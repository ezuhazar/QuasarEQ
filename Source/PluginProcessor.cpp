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
void QuasarEQAudioProcessor::changeProgramName(int index, const juce::String& newName) {}
void QuasarEQAudioProcessor::releaseResources() {}
void QuasarEQAudioProcessor::setCurrentProgram(int index) {}
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
enum QuasarEQAudioProcessor::FilterType { HighPass, HighShelf, LowPass, LowShelf, PeakFilter };
void QuasarEQAudioProcessor::parameterChanged(const juce::String& parameterID, float newValue) { parametersChanged.store(true); }
juce::AudioProcessorValueTreeState::ParameterLayout QuasarEQAudioProcessor::createParameterLayout()
{
    const float min = 20.0f;
    const float max = 20000.0f;
    const float defaultQValue = 1.0f / juce::MathConstants<float>::sqrt2;
    juce::NormalisableRange<float> QRange (0.05f, 12.0f, 0.001f);
    juce::NormalisableRange<float> FreqRange (min, max, 0.1f);
    QRange.setSkewForCentre(defaultQValue);
    FreqRange.setSkewForCentre(std::sqrtf(min * max));
    juce::AudioProcessorValueTreeState::ParameterLayout layout;
    layout.add(std::make_unique<juce::AudioParameterFloat>("outGain", "Out Gain", juce::NormalisableRange<float>(-24.0f, 24.0f, 0.01f), 0.0f, "dB"));
    layout.add(std::make_unique<juce::AudioParameterBool>("bypass", "Bypass", false));
    const std::array<float, NUM_BANDS> initialFrequencies = {
        50.0f, 100.0f, 200.0f, 500.0f, 1000.0f, 2000.0f, 5000.0f, 10000.0f
    };
    for (int i = 0; i < NUM_BANDS; ++i)
    {
        const juce::String index = juce::String(i + 1);
        layout.add(std::make_unique<juce::AudioParameterFloat>("Freq" + index, "Band " + index + " Freq", FreqRange, initialFrequencies[i], "Hz"));
        layout.add(std::make_unique<juce::AudioParameterFloat>("Gain" + index, "Band " + index + " Gain", juce::NormalisableRange<float>(-24.0f, 24.0f, 0.01f), 0.0f, "dB"));
        layout.add(std::make_unique<juce::AudioParameterFloat>("Q" + index, "Band " + index + " Q", QRange, defaultQValue));
        layout.add(std::make_unique<juce::AudioParameterChoice>("Type" + index, "Band " + index + " Type", juce::StringArray {"HighPass", "HighShelf", "LowPass", "LowShelf", "PeakFilter"}, PeakFilter));
    }
    return layout;
}
void QuasarEQAudioProcessor::updateFilters()
{
    const double sampleRate = getSampleRate();
    std::array<juce::dsp::IIR::Coefficients<float>::Ptr, NUM_BANDS> newCoefs;
    for (int i = 0; i < NUM_BANDS; ++i)
    {
        const juce::String index = juce::String(i + 1);
        const float freq = apvts.getRawParameterValue("Freq" + index)->load();
        const float gainDB = apvts.getRawParameterValue("Gain" + index)->load();
        const float gainLinear = juce::Decibels::decibelsToGain(gainDB);
        const float q = apvts.getRawParameterValue("Q" + index)->load();
        const auto typeIndex = static_cast<FilterType>(static_cast<int>(apvts.getRawParameterValue("Type" + index)->load()));
        switch (typeIndex)
        {
        case HighPass:
            newCoefs[i] = juce::dsp::IIR::Coefficients<float>::makeHighPass(sampleRate, freq, q);
            break;
        case HighShelf:
            newCoefs[i] = juce::dsp::IIR::Coefficients<float>::makeHighShelf(sampleRate, freq, q, gainLinear);
            break;
        case LowPass:
            newCoefs[i] = juce::dsp::IIR::Coefficients<float>::makeLowPass(sampleRate, freq, q);
            break;
        case LowShelf:
            newCoefs[i] = juce::dsp::IIR::Coefficients<float>::makeLowShelf(sampleRate, freq, q, gainLinear);
            break;
        case PeakFilter:
            newCoefs[i] = juce::dsp::IIR::Coefficients<float>::makePeakFilter(sampleRate, freq, q, gainLinear);
            break;
        default:
            newCoefs[i] = juce::dsp::IIR::Coefficients<float>::makePeakFilter(sampleRate, freq, q, gainLinear);
            break;
        }
    }
    const bool isBypassed = apvts.getRawParameterValue("bypass")->load();
    const float currentGainDB = apvts.getRawParameterValue("outGain")->load();
    const float gainLinear = juce::Decibels::decibelsToGain(currentGainDB);
    updateFilterChainCoefficients(newCoefs, isBypassed, std::make_index_sequence<NUM_BANDS>{});
    outputGain.setBypassed<0>(isBypassed);
    outputGain.get<0>().setGainLinear(gainLinear);
    {
        juce::ScopedLock lock (coefficientsLock);
        for (int i = 0; i < NUM_BANDS; ++i)
        {
            sharedCoefficients[i] = newCoefs[i];
        }
    }
    sendChangeMessage();
    parametersChanged.store(false);
}
std::array<juce::dsp::IIR::Coefficients<float>::Ptr, QuasarEQAudioProcessor::NUM_BANDS> QuasarEQAudioProcessor::getSharedCoefficients() const
{
    juce::ScopedLock lock (coefficientsLock);
    return sharedCoefficients;
}
