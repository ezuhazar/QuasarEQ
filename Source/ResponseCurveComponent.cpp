#include "ResponseCurveComponent.h"
#include "QuasarHeader.h"


VisualizerComponent::VisualizerComponent(QuasarEQAudioProcessor& p):
    audioProcessor(p),
    pathProducer(audioProcessor.leftChannelFifo, audioProcessor.rightChannelFifo),
    analyzerThread(pathProducer, *this)
{
    freqLUT = pathProducer.makeFreqLUT(audioProcessor.getSampleRate(), MIN_HZ, MAX_HZ);
}
void VisualizerComponent::parameterChanged(const juce::String& parameterID, float newValue)
{
}

juce::Rectangle<int> VisualizerComponent::getCurveArea()
{
    auto a = getLocalBounds().reduced(margin << 1);
    a.removeFromRight(margin * 6);
    return a;
}
juce::Rectangle<int> VisualizerComponent::getLevelMeterArea()
{
    auto a = getLocalBounds().reduced(margin << 1);
    return a.removeFromRight(margin << 1);
}
void VisualizerComponent::paint(juce::Graphics& g)
{
    g.drawImageAt(gridCache, 0, 0);
    SpectrumRenderData localPath;
    {
        juce::ScopedLock lock(pathLock);
        localPath = channelPathToDraw;
    }
    g.saveState();
    g.reduceClipRegion(getCurveArea());
    spectrumPoints.clear();
    peakHoldPoints.clear();
    spectrumPoints.reserve(localPath.spectrumPath.size());
    peakHoldPoints.reserve(localPath.spectrumPath.size());
    if (localPath.spectrumPath.size() != 0)
    {
        for (size_t i = 1; i < localPath.spectrumPath.size(); ++i)
        {
            const float freq = freqLUT[i] * getCurveArea().getWidth() + getCurveArea().getX();
            spectrumPoints.emplace_back(freq, juce::jmap(localPath.spectrumPath[i], MIN_DBFS, MAX_DBFS, getCurveArea().toFloat().getBottom(), getCurveArea().toFloat().getY()));
            peakHoldPoints.emplace_back(freq, juce::jmap(localPath.peakHoldPath[i], MIN_DBFS, MAX_DBFS, getCurveArea().toFloat().getBottom(), getCurveArea().toFloat().getY()));
        }
    }
    if (spectrumPoints.size() != 0)
    {
        juce::Path curvePathPeak = createBezierPath(spectrumPoints);
        curvePathPeak.lineTo(spectrumPoints.back().x, getCurveArea().toFloat().getBottom());
        curvePathPeak.lineTo(spectrumPoints[0].x, getCurveArea().toFloat().getBottom());
        curvePathPeak.closeSubPath();
        g.setFillType(juce::FillType(gradientLUT));
        g.fillPath(curvePathPeak);
    }
    if (peakHoldPoints.size() != 0)
    {
        juce::Path curvePathPeak = createBezierPath(peakHoldPoints);
        g.setColour(quasar::colours::audioSignal);
        g.strokePath(curvePathPeak, juce::PathStrokeType(1.3f));
    }
    g.setColour(quasar::colours::enabled);
    g.strokePath(responseCurvePath, juce::PathStrokeType(2.5f));
    g.setFillType(juce::FillType(quasar::colours::audioSignal.withAlpha(0.25f)));
    g.fillPath(responseCurvePath);
    g.restoreState();
    const float high = 6.0f;
    const float low = -18.0f;
    const int leftY = juce::roundToInt(juce::jmap(localPath.leftDB, low, high, getLevelMeterArea().toFloat().getBottom(), getLevelMeterArea().toFloat().getY()));
    const int rightY = juce::roundToInt(juce::jmap(localPath.rightDB, low, high, getLevelMeterArea().toFloat().getBottom(), getLevelMeterArea().toFloat().getY()));
    g.setFillType(juce::FillType(gradientLUT));
    g.fillRect(juce::Rectangle<int>::leftTopRightBottom(getLevelMeterArea().getX(), leftY, getLevelMeterArea().getX() + (getLevelMeterArea().getWidth() >> 1), getLevelMeterArea().getBottom()));
    g.fillRect(juce::Rectangle<int>::leftTopRightBottom(getLevelMeterArea().getX() + (getLevelMeterArea().getWidth() >> 1), rightY, getLevelMeterArea().getRight(), getLevelMeterArea().getBottom()));

    auto bounds = getCurveArea().toFloat();
    const float minDb = -24.0f;
    const float maxDb = 24.0f;
    auto& apvts = audioProcessor.apvts;


    for (int i = 0; i < audioProcessor.NUM_BANDS; ++i)
    {
        juce::String index = juce::String(i + 1);
        float freqHz = apvts.getRawParameterValue("Freq" + index)->load();
        float gainDb = apvts.getRawParameterValue("Gain" + index)->load();
        float x = bounds.getX() + bounds.getWidth() * juce::mapFromLog10(freqHz, MIN_HZ, MAX_HZ);
        float y = juce::jmap(gainDb, minDb, maxDb, bounds.getBottom(), bounds.getY());
        g.setColour(quasar::colours::labelBackground);
        const int pointSize = 14;
        g.fillEllipse(x - (pointSize >> 1), y - pointSize * 0.5f, pointSize, pointSize);
        g.setColour(quasar::colours::staticText);
        g.drawEllipse(x - (pointSize >> 1), y - pointSize * 0.5f, pointSize, pointSize, 1.5f);
        juce::String bandNumber = juce::String(i + 1);
        const int textHeight = 12;
        g.setFont(textHeight);
        const int textWidth = g.getCurrentFont().getStringWidth(bandNumber);
        juce::Rectangle<int> textBounds(
            juce::roundToInt(x - textWidth * 0.5f),
            y - 6,
            textWidth,
            textHeight
        );
        g.drawText(
            bandNumber,
            textBounds,
            juce::Justification::centred,
            false
        );
    }
}

void VisualizerComponent::calculateResponseCurve()
{
    const int curveSize = getCurveArea().getWidth();
    const float minHz = MIN_HZ;
    const float maxHz = MAX_HZ;
    double sampleRate = audioProcessor.getSampleRate();
    responseCurveMagnitude.clear();
    responseCurveMagnitude.resize(curveSize, 0.0f);
    auto allCoefficients = audioProcessor.getSharedCoefficients();
    for (int i = 0; i < curveSize; ++i)
    {
        float normalizedX = (float)i / (float)(curveSize - 1);
        float freqHz = juce::mapToLog10(normalizedX, minHz, maxHz);
        float totalGainLinear = 1.0f;
        for (const auto& coefs : allCoefficients)
        {
            totalGainLinear *= coefs->getMagnitudeForFrequency(freqHz, sampleRate);
        }
        responseCurveMagnitude[i] = juce::Decibels::gainToDecibels(totalGainLinear);
    }
    auto bounds = getCurveArea().toFloat();
    const float minDb = -24.0f;
    const float maxDb = 24.0f;
    responseCurvePath.clear();
    responseCurvePath.startNewSubPath(getLocalBounds().toFloat().getX(), getLocalBounds().toFloat().getCentreY());
    for (size_t i = 0; i < responseCurveMagnitude.size(); ++i)
    {

        float magnitudeDb = responseCurveMagnitude[i];
        float normalizedX = (float)i / (float)(responseCurveMagnitude.size() - 1);
        float x = bounds.getX() + bounds.getWidth() * normalizedX;
        float y = juce::jmap(magnitudeDb, minDb, maxDb, bounds.getBottom(), bounds.getY());

        responseCurvePath.lineTo(x, y);
    }
    responseCurvePath.lineTo(getLocalBounds().toFloat().getRight(), getLocalBounds().toFloat().getCentreY());
}

juce::Path VisualizerComponent::createBezierPath(const std::vector<juce::Point<float>>& points)
{
    const size_t numPoints = points.size();
    const size_t a = 255;

    const size_t b = 0;
    if ((numPoints < 2))
    {
        return juce::Path {};
    }
    static constexpr float BEZIER_SCALE = 1.0f / 6.0f;
    juce::Path p {};
    p.startNewSubPath(points[b]);
    p.lineTo(points[b + 1]);
    for (size_t i = b + 1; i < a - 2; ++i)
    {
        const auto& p0 = points[i - 1];
        const auto& p1 = points[i];
        const auto& p2 = points[i + 1];
        const auto& p3 = points[i + 2];
        const juce::Point<float> cp1 = p1 + (p2 - p0) * BEZIER_SCALE;
        const juce::Point<float> cp2 = p2 - (p3 - p1) * BEZIER_SCALE;
        p.cubicTo(cp1, cp2, p2);
    }
    p.lineTo(points[a - 1]);
    for (size_t i = a; i < 509; ++i)
    {
        p.lineTo(points[i]);
    }
    return p;
}
void VisualizerComponent::handleAsyncUpdate()
{
    SpectrumRenderData path;
    bool newPathAvailable = false;
    while (pathProducer.getNumPathsAvailable() > 0)
    {
        if (pathProducer.getPath(path))
        {
            newPathAvailable = true;
        }
    }
    if (aaa)
    {
        calculateResponseCurve();
        aaa = false;
    }
    if (newPathAvailable)
    {
        {
            juce::ScopedLock lock(pathLock);
            channelPathToDraw = path;
        }
        repaint();
    }
}
void VisualizerComponent::resized()
{
    gradientLUT = juce::ColourGradient::vertical(quasar::colours::audioSignal, getCurveArea().toFloat().getY(), quasar::colours::audioSignal.withAlpha(0.3f), getCurveArea().toFloat().getBottom());
    //-----------------------------------------------------------------------------------------------
    gridCache = juce::Image(juce::Image::ARGB, getWidth(), getHeight(), true);
    juce::Graphics g(gridCache);
    g.setColour(quasar::colours::labelBackground);
    g.fillRoundedRectangle(getLocalBounds().toFloat(), 5.0f);
    g.setColour(juce::Colours::black);
    g.fillRect(getCurveArea());
    g.fillRect(getLevelMeterArea());
    g.setColour(juce::Colours::dimgrey.withAlpha(0.5f));
    g.drawVerticalLine(getLevelMeterArea().getX(), getLevelMeterArea().getY(), getLevelMeterArea().getBottom());
    g.drawVerticalLine(getLevelMeterArea().getRight(), getLevelMeterArea().getY(), getLevelMeterArea().getBottom());
    g.drawVerticalLine(getLevelMeterArea().getCentreX(), getLevelMeterArea().getY(), getLevelMeterArea().getBottom());
    g.drawHorizontalLine(getLevelMeterArea().getY(), getLevelMeterArea().getX(), getLevelMeterArea().getRight());
    g.drawHorizontalLine(getLevelMeterArea().getBottom(), getLevelMeterArea().getX(), getLevelMeterArea().getRight());
    g.drawHorizontalLine(getLevelMeterArea().getY() + getLevelMeterArea().getHeight() * (0.25f), getLevelMeterArea().getX(), getLevelMeterArea().getRight());
    std::vector<float> gridLUTX;
    std::vector<float> gridLUTY;
    for (float freq : gridFrequencies)
    {
        gridLUTX.push_back(getCurveArea().getX() + getCurveArea().getWidth() * juce::mapFromLog10(freq, MIN_HZ, MAX_HZ));
    }
    for (size_t i = 0; i < 9; ++i)
    {
        gridLUTY.push_back(getCurveArea().getY() + getCurveArea().getHeight() * 0.125f * i);
    }
    for (float x : gridLUTX)
    {
        g.drawVerticalLine(x, getCurveArea().toFloat().getY(), getCurveArea().toFloat().getBottom());
    }
    for (float y : gridLUTY)
    {
        g.drawHorizontalLine(y, getCurveArea().toFloat().getX(), getCurveArea().toFloat().getRight());
    }
    g.setColour(quasar::colours::staticText);
    g.setFont(FONT_HEIGHT);
    const int freqLabelY = getCurveArea().getBottom() + margin - HALF_FONT_HEIGHT;
    for (size_t i = 0; i < gridLUTX.size(); ++i)
    {
        const int stringWidth = g.getCurrentFont().getStringWidth(frequencyTags[i]);
        const int labelX = static_cast<int>(std::roundf(gridLUTX[i] - stringWidth * 0.5f));
        g.drawText(frequencyTags[i], labelX, freqLabelY, stringWidth, FONT_HEIGHT, juce::Justification::centred, false);
    }
    for (size_t i = 0; i < gridLUTY.size(); ++i)
    {
        const int dbLabelY = static_cast<int>(std::roundf(gridLUTY[i] - HALF_FONT_HEIGHT));
        const int curveLabelWidth = g.getCurrentFont().getStringWidth(dbTags[i]);
        const int curveLabelY = getCurveArea().getX() - margin - static_cast<int>(std::roundf(curveLabelWidth * 0.5f));
        g.drawText(dbTags[i], curveLabelY, dbLabelY, curveLabelWidth, FONT_HEIGHT, juce::Justification::centred, false);
        const int meterLabelWidth = g.getCurrentFont().getStringWidth(meterTags[i]);
        const int meterLabelY = getLevelMeterArea().getX() - margin - static_cast<int>(std::roundf(meterLabelWidth * 0.5f));
        g.drawText(meterTags[i], meterLabelY, dbLabelY, meterLabelWidth, FONT_HEIGHT, juce::Justification::centred, false);
    }
}
VisualizerComponent::AnalyzerThread::AnalyzerThread(PathProducer& producer, VisualizerComponent& comp): juce::Thread("FFT Analyzer Thread"), producer(producer), responseCurveComponent(comp)
{
    startThread();
}
VisualizerComponent::AnalyzerThread::~AnalyzerThread()
{
    stopThread(1000);
}
void VisualizerComponent::AnalyzerThread::run()
{
    while (!threadShouldExit())
    {
        producer.process(responseCurveComponent.audioProcessor.getSampleRate());
        if (!responseCurveComponent.isUpdatePending() && producer.getNumPathsAvailable() > 0)
        {
            responseCurveComponent.triggerAsyncUpdate();
        }
        juce::Thread::sleep(THREAD_SLEEP_TIME);
    }
}
