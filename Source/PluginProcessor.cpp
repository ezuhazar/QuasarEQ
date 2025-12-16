#include "PluginProcessor.h"
#include "PluginEditor.h"
#include "QuasarHeader.h"

enum FilterType
{
    HighPass, HighShelf, LowPass, LowShelf, PeakFilter
};
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
    for (int i = 0; i < NUM_BANDS; ++i)
    {
        juce::String index = juce::String(i + 1);
        apvts.addParameterListener("Freq" + index, this);
        apvts.addParameterListener("Gain" + index, this);
        apvts.addParameterListener("Q" + index, this);
        apvts.addParameterListener("Type" + index, this);
    }
}
const juce::String QuasarEQAudioProcessor::getName() const
{
    return JucePlugin_Name;
}
bool QuasarEQAudioProcessor::acceptsMidi() const
{
    return JucePlugin_WantsMidiInput;
}
bool QuasarEQAudioProcessor::producesMidi() const
{
    return JucePlugin_ProducesMidiOutput;
}
bool QuasarEQAudioProcessor::isMidiEffect() const
{
    return JucePlugin_IsMidiEffect;
}
double QuasarEQAudioProcessor::getTailLengthSeconds() const
{
    return 0.0;
}
int QuasarEQAudioProcessor::getNumPrograms()
{
    // NB: some hosts don't cope very well if you tell them there are 0 programs, so this should be at least 1, even if you're not really implementing programs.
    return 1;
}
int QuasarEQAudioProcessor::getCurrentProgram()
{
    return 0;
}
void QuasarEQAudioProcessor::setCurrentProgram(int index)
{
}
const juce::String QuasarEQAudioProcessor::getProgramName(int index)
{
    return {};
}
void QuasarEQAudioProcessor::changeProgramName(int index, const juce::String& newName)
{
}
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
    updateFilters();
}
void QuasarEQAudioProcessor::releaseResources()
{
    // When playback stops, you can use this as an opportunity to free up any
    // spare memory, etc.
}
#ifndef JucePlugin_PreferredChannelConfigurations
bool QuasarEQAudioProcessor::isBusesLayoutSupported(const BusesLayout& layouts) const
{
#if JucePlugin_IsMidiEffect
    juce::ignoreUnused(layouts);
    return true;
#else
    // This is the place where you check if the layout is supported.
    // In this template code we only support mono or stereo.
    // Some plugin hosts, such as certain GarageBand versions, will only
    // load plugins that support stereo bus layouts.
    if (layouts.getMainOutputChannelSet() != juce::AudioChannelSet::mono()
        && layouts.getMainOutputChannelSet() != juce::AudioChannelSet::stereo())
        return false;

    // This checks if the input layout matches the output layout
#if ! JucePlugin_IsSynth
    if (layouts.getMainOutputChannelSet() != layouts.getMainInputChannelSet())
        return false;
#endif

    return true;
#endif
}
#endif
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
    leftChannelFifo.update(buffer);
    rightChannelFifo.update(buffer);
}
bool QuasarEQAudioProcessor::hasEditor() const
{
    // (change this to false if you choose to not supply an editor)
    return true;
}
juce::AudioProcessorEditor* QuasarEQAudioProcessor::createEditor()
{
    return new QuasarEQAudioProcessorEditor(*this);
}
void QuasarEQAudioProcessor::getStateInformation(juce::MemoryBlock& destData)
{
    juce::MemoryOutputStream stream(destData, false);
    apvts.state.writeToStream(stream);
    // You should use this method to store your parameters in the memory block.
    // You could do that either as raw data, or use the XML or ValueTree classes
    // as intermediaries to make it easy to save and load complex data.
}
void QuasarEQAudioProcessor::setStateInformation(const void* data, int sizeInBytes)
{
    auto tree = juce::ValueTree::readFromData(data, size_t(sizeInBytes));
    if (tree.isValid())
    {
        apvts.replaceState(tree);
    }
    // You should use this method to restore your parameters from this memory block,
    // whose contents will have been created by the getStateInformation() call.
}
// This creates new instances of the plugin..
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new QuasarEQAudioProcessor();
}

//------------------------------------------------------------------------------------------

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
    layout.add(std::make_unique<juce::AudioParameterFloat>("outGain", "Out Gain", juce::NormalisableRange<float>(-24.0f, 24.0f, 0.001f), 0.0f, "dB"));
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
    {
        juce::ScopedLock lock (coefficientsLock);
        for (int i = 0; i < NUM_BANDS; ++i)
        {
            sharedCoefficients[i] = newCoefs[i];
        }
        const bool isBypassed = apvts.getRawParameterValue("bypass")->load();
        const float currentGainDB = apvts.getRawParameterValue("outGain")->load();
        const float gainLinear = juce::Decibels::decibelsToGain(currentGainDB);
        *filterChain.get<0>().state = *newCoefs[0];
        *filterChain.get<1>().state = *newCoefs[1];
        *filterChain.get<2>().state = *newCoefs[2];
        *filterChain.get<3>().state = *newCoefs[3];
        *filterChain.get<4>().state = *newCoefs[4];
        *filterChain.get<5>().state = *newCoefs[5];
        *filterChain.get<6>().state = *newCoefs[6];
        *filterChain.get<7>().state = *newCoefs[7];
        filterChain.get<8>().setGainLinear(gainLinear);
        filterChain.setBypassed<0>(isBypassed);
        filterChain.setBypassed<1>(isBypassed);
        filterChain.setBypassed<2>(isBypassed);
        filterChain.setBypassed<3>(isBypassed);
        filterChain.setBypassed<4>(isBypassed);
        filterChain.setBypassed<5>(isBypassed);
        filterChain.setBypassed<6>(isBypassed);
        filterChain.setBypassed<7>(isBypassed);
        filterChain.setBypassed<8>(isBypassed);
    }
    sendChangeMessage();
    parametersChanged.store(false);
}
void QuasarEQAudioProcessor::parameterChanged(const juce::String& parameterID, float newValue)
{
    parametersChanged.store(true);
}
std::array<juce::dsp::IIR::Coefficients<float>::Ptr, QuasarEQAudioProcessor::NUM_BANDS> QuasarEQAudioProcessor::getSharedCoefficients() const
{
    juce::ScopedLock lock (coefficientsLock);
    return sharedCoefficients;
}
