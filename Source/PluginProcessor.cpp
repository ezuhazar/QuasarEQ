#include "PluginProcessor.h"
#include "PluginEditor.h"
QuasarEQAudioProcessor::QuasarEQAudioProcessor()
#ifndef JucePlugin_PreferredChannelConfigurations
    : AudioProcessor(BusesProperties()
#if ! JucePlugin_IsMidiEffect
#if ! JucePlugin_IsSynth
        .withInput("Input", juce::AudioChannelSet::stereo(), true)
#endif
        .withOutput("Output", juce::AudioChannelSet::stereo(), true)
#endif
    )
#endif
    , apvts(*this, nullptr, "Parameters", createParameterLayout())
{
    apvts.addParameterListener("outGain", this);
    apvts.addParameterListener("bypass", this);

    const juce::StringArray bandParamPrefixes = {"Freq", "Gain", "Q", "Type"};
    for (int i = 0; i < NUM_BANDS; ++i)
    {
        const juce::String index = juce::String (i + 1);
        for (const auto& prefix : bandParamPrefixes)
        {
            apvts.addParameterListener (prefix + index, this);
        }
    }

    outGain = apvts.getRawParameterValue("outGain");
    bypass = apvts.getRawParameterValue("bypass");
    for (int i = 0; i < NUM_BANDS; ++i)
    {
        const juce::String idx = juce::String(i + 1);
        auto& p = bandParams[i];
        p.freq = apvts.getRawParameterValue("Freq" + idx);
        p.gain = apvts.getRawParameterValue("Gain" + idx);
        p.q = apvts.getRawParameterValue("Q" + idx);
        p.type = apvts.getRawParameterValue("Type" + idx);
    }
}
#ifndef JucePlugin_PreferredChannelConfigurations
bool QuasarEQAudioProcessor::isBusesLayoutSupported(const BusesLayout& layouts) const
{
#if JucePlugin_IsMidiEffect
    juce::ignoreUnused(layouts);
    return true;
#else
    if (layouts.getMainOutputChannelSet() != juce::AudioChannelSet::mono() && layouts.getMainOutputChannelSet() != juce::AudioChannelSet::stereo())
        return false;
#if ! JucePlugin_IsSynth
    if (layouts.getMainOutputChannelSet() != layouts.getMainInputChannelSet())
        return false;
#endif
    return true;
#endif
}
#endif
int QuasarEQAudioProcessor::getNumPrograms() { return 1; }
int QuasarEQAudioProcessor::getCurrentProgram() { return 0; }
bool QuasarEQAudioProcessor::hasEditor() const { return true; }
bool QuasarEQAudioProcessor::acceptsMidi() const { return JucePlugin_WantsMidiInput; }
bool QuasarEQAudioProcessor::isMidiEffect() const { return JucePlugin_IsMidiEffect; }
bool QuasarEQAudioProcessor::producesMidi() const { return JucePlugin_ProducesMidiOutput; }
void QuasarEQAudioProcessor::releaseResources() {}
void QuasarEQAudioProcessor::parameterChanged(const juce::String& parameterID, float newValue) { parametersChanged.store(true); }
void QuasarEQAudioProcessor::setCurrentProgram(int index) {}
void QuasarEQAudioProcessor::changeProgramName(int index, const juce::String& newName) {}
double QuasarEQAudioProcessor::getTailLengthSeconds() const { return 0.0; }
const juce::String QuasarEQAudioProcessor::getName() const { return JucePlugin_Name; }
const juce::String QuasarEQAudioProcessor::getProgramName(int index) { return {}; }
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() { return new QuasarEQAudioProcessor(); }
juce::AudioProcessorEditor* QuasarEQAudioProcessor::createEditor() { return new QuasarEQAudioProcessorEditor(*this); }
void QuasarEQAudioProcessor::prepareToPlay(double sampleRate, int samplesPerBlock)
{
    juce::dsp::ProcessSpec spec {};
    spec.sampleRate = sampleRate;
    spec.maximumBlockSize = (juce::uint32)samplesPerBlock;
    spec.numChannels = (juce::uint32)getTotalNumOutputChannels();
    leftChannelFifo.prepare(samplesPerBlock);
    rightChannelFifo.prepare(samplesPerBlock);
    filterChain.prepare(spec);
    filterChain.reset();
    outputGain.prepare(spec);
    outputGain.reset();
    updateFilters();
}
void QuasarEQAudioProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    juce::ScopedNoDenormals noDenormals;
    auto totalNumInputChannels = getTotalNumInputChannels();
    auto totalNumOutputChannels = getTotalNumOutputChannels();
    for (auto i = totalNumInputChannels; i < totalNumOutputChannels; ++i)
    {
        buffer.clear(i, 0, buffer.getNumSamples());
    }
    if (parametersChanged.load())
    {
        updateFilters();
    }
    juce::dsp::AudioBlock<float> block(buffer);
    juce::dsp::ProcessContextReplacing<float> context(block);
    filterChain.process(context);
    outputGain.process(context);
    leftChannelFifo.update(buffer);
    rightChannelFifo.update(buffer);
}
void QuasarEQAudioProcessor::getStateInformation(juce::MemoryBlock& destData)
{
    juce::MemoryOutputStream stream(destData, false);
    apvts.state.writeToStream(stream);
}
void QuasarEQAudioProcessor::setStateInformation(const void* data, int sizeInBytes)
{
    auto tree = juce::ValueTree::readFromData(data, size_t(sizeInBytes));
    if (tree.isValid())
    {
        apvts.replaceState(tree);
    }
}
juce::AudioProcessorValueTreeState::ParameterLayout QuasarEQAudioProcessor::createParameterLayout() const
{
    const float FREQ_RATIO = std::pow(MAX_FREQ / MIN_FREQ, 1.0 / static_cast<double>(NUM_BANDS + 1));
    const float CENTRE_GAIN = 0.0f;
    const float CENTRE_FREQ = std::sqrtf(MIN_FREQ * MAX_FREQ);
    const float CENTRE_Q = 1.0f / juce::MathConstants<float>::sqrt2;
    const int DEFAULT_FILTER = PeakFilter;
    juce::NormalisableRange<float> gainRange (MIN_GAIN, MAX_GAIN, GAIN_INTERVAL);
    juce::NormalisableRange<float> FreqRange (MIN_FREQ, MAX_FREQ, FREQ_INTERVAL);
    juce::NormalisableRange<float> QRange (MIN_Q, MAX_Q, Q_INTERVAL);
    FreqRange.setSkewForCentre(CENTRE_FREQ);
    QRange.setSkewForCentre(CENTRE_Q);
    juce::AudioProcessorValueTreeState::ParameterLayout layout;
    layout.add(std::make_unique<juce::AudioParameterFloat>("outGain", "Out Gain", gainRange, CENTRE_GAIN, "dB"));
    layout.add(std::make_unique<juce::AudioParameterBool>("bypass", "Bypass", false));
    float currentFrequency = MIN_FREQ;
    for (int i = 0; i < NUM_BANDS; ++i)
    {
        const juce::String index = juce::String(i + 1);
        currentFrequency *= FREQ_RATIO;
        layout.add(std::make_unique<juce::AudioParameterFloat>("Freq" + index, "Band " + index + " Freq", FreqRange, currentFrequency, "Hz"));
        layout.add(std::make_unique<juce::AudioParameterFloat>("Gain" + index, "Band " + index + " Gain", gainRange, CENTRE_GAIN, "dB"));
        layout.add(std::make_unique<juce::AudioParameterFloat>("Q" + index, "Band " + index + " Q", QRange, CENTRE_Q));
        layout.add(std::make_unique<juce::AudioParameterChoice>("Type" + index, "Band " + index + " Type", juce::StringArray {"HighPass", "HighShelf", "LowPass", "LowShelf", "PeakFilter"}, DEFAULT_FILTER));
    }
    return layout;
}
void QuasarEQAudioProcessor::updateFilters()
{
    const double sampleRate = getSampleRate();
    for (int i = 0; i < NUM_BANDS; ++i)
    {
        const auto& p = bandParams[i];
        const float freq = juce::jmin(p.freq->load(), static_cast<float>(sampleRate) * 0.49f);
        const float q = p.q->load();
        const float gain = juce::Decibels::decibelsToGain(p.gain->load());
        const FilterType typeIndex = static_cast<FilterType>((int)p.type->load());
        switch (typeIndex)
        {
        case HighPass:
            coefsBuffer[i] = juce::dsp::IIR::Coefficients<float>::makeHighPass(sampleRate, freq, q);
            break;
        case HighShelf:
            coefsBuffer[i] = juce::dsp::IIR::Coefficients<float>::makeHighShelf(sampleRate, freq, q, gain);
            break;
        case LowPass:
            coefsBuffer[i] = juce::dsp::IIR::Coefficients<float>::makeLowPass(sampleRate, freq, q);
            break;
        case LowShelf:
            coefsBuffer[i] = juce::dsp::IIR::Coefficients<float>::makeLowShelf(sampleRate, freq, q, gain);
            break;
        case PeakFilter:
            coefsBuffer[i] = juce::dsp::IIR::Coefficients<float>::makePeakFilter(sampleRate, freq, q, gain);
            break;
        default:
            coefsBuffer[i] = juce::dsp::IIR::Coefficients<float>::makePeakFilter(sampleRate, freq, q, gain);
            break;
        }
    }
    const bool isBypassed = bypass->load();
    const float gain = outGain->load();
    outputGain.setBypassed<0>(isBypassed);
    outputGain.get<0>().setGainDecibels(gain);
    updateFilterChainCoefficients(coefsBuffer, isBypassed, std::make_index_sequence<NUM_BANDS>{});
    {
        juce::ScopedLock lock (coefficientsLock);
        sharedCoefficients = coefsBuffer;
    }
    sendChangeMessage();
    parametersChanged.store(false);
}
QuasarEQAudioProcessor::CoefPtrArray QuasarEQAudioProcessor::getSharedCoefficients() const
{
    juce::ScopedLock lock (coefficientsLock);
    return sharedCoefficients;
}
