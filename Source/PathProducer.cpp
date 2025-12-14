#include "PathProducer.h"

PathProducer::PathProducer(SingleChannelSampleFifo& leftScsf, SingleChannelSampleFifo& rightScsf): leftChannelFifo(&leftScsf), rightChannelFifo(&rightScsf)
{
    fftBuffer.setSize(1, FFT_OUT_SIZE, false, true, true);
    monoBufferL.setSize(1, FFT_SIZE, false, true, true);
    monoBufferR.setSize(1, FFT_SIZE, false, true, true);
    monoAverageBuffer.setSize(1, FFT_SIZE, false, true, true);
    peakFallVelocity.assign(RENDER_OUT_SIZE, 0.0f);
    peakHoldDecibels.assign(RENDER_OUT_SIZE, -std::numeric_limits<float>::infinity());
    currentDecibels.assign(RENDER_OUT_SIZE, -std::numeric_limits<float>::infinity());
    Gains.assign(RENDER_OUT_SIZE, 0.0f);
    SmoothGains.assign(RENDER_OUT_SIZE, 0.0f);
}
void PathProducer::process(float minDB, double sampleRate)
{
    juce::AudioBuffer<float> leftIncomingBuffer, rightIncomingBuffer;

    bool aaa = false;
    while (leftChannelFifo->getNumCompleteBuffersAvailable() > 0 && rightChannelFifo->getNumCompleteBuffersAvailable() > 0)
    {
        if (leftChannelFifo->getAudioBuffer(leftIncomingBuffer) && rightChannelFifo->getAudioBuffer(rightIncomingBuffer))
        {
            aaa = true;
            const int incomingSize = leftIncomingBuffer.getNumSamples();

            currentLeftGain = leftIncomingBuffer.getMagnitude(0, 0, incomingSize);
            currentRightGain = rightIncomingBuffer.getMagnitude(0, 0, incomingSize);

            const int copySize = FFT_SIZE - incomingSize;
            monoBufferL.copyFrom(0, 0, monoBufferL.getReadPointer(0, incomingSize), copySize);
            monoBufferR.copyFrom(0, 0, monoBufferR.getReadPointer(0, incomingSize), copySize);
            monoBufferL.copyFrom(0, copySize, leftIncomingBuffer.getReadPointer(0), incomingSize);
            monoBufferR.copyFrom(0, copySize, rightIncomingBuffer.getReadPointer(0), incomingSize);
            auto* destData = monoAverageBuffer.getWritePointer(0);
            juce::FloatVectorOperations::copy(destData, monoBufferL.getReadPointer(0), FFT_SIZE);
            juce::FloatVectorOperations::add(destData, monoBufferR.getReadPointer(0), FFT_SIZE);
            juce::FloatVectorOperations::multiply(destData, 0.5f, FFT_SIZE);
            auto* fftDataWritePointer = fftBuffer.getWritePointer(0);
            juce::FloatVectorOperations::clear(fftDataWritePointer, FFT_OUT_SIZE);
            juce::FloatVectorOperations::copy(fftDataWritePointer, monoAverageBuffer.getReadPointer(0), FFT_SIZE);
            windowing.multiplyWithWindowingTable(fftDataWritePointer, FFT_SIZE);
            fft.performFrequencyOnlyForwardTransform(fftDataWritePointer);
            juce::FloatVectorOperations::multiply(fftDataWritePointer, fftDataWritePointer, INVERSE_NUM_BINS, NUM_BINS);
            generatePath(fftDataWritePointer, minDB, static_cast<float>(incomingSize) / sampleRate);
        }
    }
    if (aaa)
    {

        pathFifo.push({currentDecibels, peakHoldDecibels, juce::Decibels::gainToDecibels(smoothedLeftGain, minDB), juce::Decibels::gainToDecibels(smoothedRightGain, minDB)});

    }
}
std::vector<float> PathProducer::makeFreqLUT(const double sampleRate, const float minHz, const float maxHz) const
{
    std::vector<float> frequencyLUT;
    frequencyLUT.reserve(RENDER_OUT_SIZE);
    const float binWidth = static_cast<float>(sampleRate / FFT_SIZE);
    for (int levelIndex = 0, sourceDataIndex = 0, outputIndex = 0; levelIndex < NUM_SECTIONS; ++levelIndex)
    {
        const int windowSize = 1 << levelIndex;
        const int nextOutputStart = outputIndex + (SECTION_SIZE >> levelIndex);
        for (; outputIndex < nextOutputStart; ++outputIndex)
        {

            frequencyLUT.push_back(juce::mapFromLog10((binWidth * sourceDataIndex), minHz, maxHz));
            sourceDataIndex += windowSize;
        }
    }
    return frequencyLUT;
}
void PathProducer::generatePath(const float* renderData, const float minDB, const float deltaTime)
{
    for (int levelIndex = 0, sourceDataIndex = 0, outputIndex = 0; levelIndex < NUM_SECTIONS; ++levelIndex)
    {
        const int windowSize = 1 << levelIndex;
        const int nextOutputStart = outputIndex + (SECTION_SIZE >> levelIndex);
        for (; outputIndex < nextOutputStart; ++outputIndex)
        {
            Gains[outputIndex] = *std::max_element(renderData + sourceDataIndex, renderData + sourceDataIndex + windowSize);
            sourceDataIndex += windowSize;
        }
    }
    const float peakFallRate = PEAK_DECAY_RATE * deltaTime;
    const float alphaSmooth = 1.0f - std::exp(-deltaTime / SMOOTHING_TIME_CONSTANT);
    const float oneMinusAlpha = 1.0f - alphaSmooth;
    for (size_t i = 0; i < RENDER_OUT_SIZE; ++i)
    {
        if (SmoothGains[i] > Gains[i])
        {
            SmoothGains[i] = alphaSmooth * Gains[i] + oneMinusAlpha * SmoothGains[i];
        }
        else
        {
            SmoothGains[i] = Gains[i];
        }
        currentDecibels[i] = juce::Decibels::gainToDecibels(SmoothGains[i], minDB);
        peakFallVelocity[i] += peakFallRate;
        peakHoldDecibels[i] -= peakFallVelocity[i] * deltaTime;
        if (currentDecibels[i] >= peakHoldDecibels[i])
        {
            peakFallVelocity[i] = 0.0f;
            peakHoldDecibels[i] = currentDecibels[i];
        }
    }
    const float levelMeterAlphaSmooth = 1.0f - std::exp(-deltaTime / (LEVEL_METER_SMOOTHING_TIME_CONSTANT));
    const float levelMeterOneMinusAlpha = 1.0f - levelMeterAlphaSmooth;
    if (currentLeftGain < smoothedLeftGain)
    {
        smoothedLeftGain = levelMeterAlphaSmooth * currentLeftGain + levelMeterOneMinusAlpha * smoothedLeftGain;
    }
    else
    {
        smoothedLeftGain = currentLeftGain;
    }
    if (currentRightGain  < smoothedRightGain)
    {
        smoothedRightGain = levelMeterAlphaSmooth * currentRightGain + levelMeterOneMinusAlpha * smoothedRightGain;
    }
    else
    {
        smoothedRightGain = currentRightGain;
    }
}
bool PathProducer::getPath(SpectrumRenderData& path)
{
    return pathFifo.pull(path);
}
int PathProducer::getNumPathsAvailable() const
{
    return pathFifo.getNumAvailableForReading();
}
