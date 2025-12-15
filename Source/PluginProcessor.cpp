#include "PluginProcessor.h"
#include "PluginEditor.h"
#include "QuasarHeader.h"

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
    for (int i = 0; i < NUM_BANDS; ++i)
    {
        juce::String index = juce::String(i + 1);
        apvts.addParameterListener("Freq" + index, this);
        apvts.addParameterListener("Gain" + index, this);
        apvts.addParameterListener("Q" + index, this);
        apvts.addParameterListener("Type" + index, this);
    }
    apvts.addParameterListener("outGain", this);
    apvts.addParameterListener("bypass", this);
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
    return 1;   // NB: some hosts don't cope very well if you tell them there are 0 programs,
    // so this should be at least 1, even if you're not really implementing programs.
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

    for (int i = 0; i < NUM_BANDS; ++i)
    {
        leftFilters[i].prepare(spec);
        rightFilters[i].prepare(spec);
        leftFilters[i].reset();
        rightFilters[i].reset();
    }
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
    // ***********************************************************************************************
    // !!! IMPORTANT !!!
    // DO NOT CREATE ProcessContextReplacing DIRECTLY FROM block.getSingleChannelBlock()
    // IN RELEASE BUILDS THE COMPILER MAY OPTIMIZE AWAY THE TEMPORARY BLOCK,
    // CAUSING UNDEFINED BEHAVIOR WHERE THE RIGHT CHANNEL BECOMES SILENT.
    //
    // ALWAYS STORE THE SINGLE-CHANNEL BLOCKS IN LOCAL VARIABLES FIRST.
    // THIS GUARANTEES THEIR LIFETIME AND PREVENTS AGGRESSIVE OPTIMIZATION BUGS.
    //
    // DEBUG BUILD WORKS BY ACCIDENT - RELEASE WILL BREAK IF YOU CHANGE THIS.
    // ***********************************************************************************************
    bool isBypassed = apvts.getRawParameterValue("bypass")->load();
    if (!isBypassed)
    {
        juce::dsp::AudioBlock<float> block(buffer);
        auto leftBlock = block.getSingleChannelBlock(0); // MUST STORE LOCALLY
        auto rightBlock = block.getSingleChannelBlock(1); // MUST STORE LOCALLY
        juce::dsp::ProcessContextReplacing<float> leftContext(leftBlock);
        juce::dsp::ProcessContextReplacing<float> rightContext(rightBlock);
        for (int i = 0; i < NUM_BANDS; ++i)
        {
            leftFilters[i].process(leftContext);
            rightFilters[i].process(rightContext);
        }
        float currentGainDB = apvts.getRawParameterValue("outGain")->load();
        const float gainLinear = juce::Decibels::decibelsToGain(currentGainDB);
        buffer.applyGain(gainLinear); 
    }
    leftChannelFifo.update(buffer);
    rightChannelFifo.update(buffer);
}
bool QuasarEQAudioProcessor::hasEditor() const
{
    return true; // (change this to false if you choose to not supply an editor)
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
    juce::NormalisableRange<float> QRange (0.05f, 12.0f, 0.001f);
    juce::NormalisableRange<float> FreqRange (min, max, 0.1f);
    const float inverseRootTwo = 1.0f / juce::MathConstants<float>::sqrt2;
    QRange.setSkewForCentre(inverseRootTwo);
    FreqRange.setSkewForCentre(std::sqrtf(min * max));
    juce::AudioProcessorValueTreeState::ParameterLayout layout;
    layout.add(std::make_unique<juce::AudioParameterFloat>("outGain", "Out Gain", juce::NormalisableRange<float>(-24.0f, 24.0f, 0.001f), 0.0f, "dB"));
    layout.add(std::make_unique<juce::AudioParameterBool>("bypass", "Bypass", false));

    const std::vector<float> aad = {
        50.0f, 100.0f, 200.0f, 500.0f, 1000.0f, 2000.0f, 5000.0f, 10000.0f
    };

    for (int i = 0; i < NUM_BANDS; ++i)
    {
        juce::String index = juce::String(i + 1);
        layout.add(std::make_unique<juce::AudioParameterFloat>("Freq" + index, "Band " + index + " Freq", FreqRange, aad[i], "Hz"));
        layout.add(std::make_unique<juce::AudioParameterFloat>("Gain" + index, "Band " + index + " Gain", juce::NormalisableRange<float>(-24.0f, 24.0f, 0.01f), 0.0f, "dB"));
        layout.add(std::make_unique<juce::AudioParameterFloat>("Q" + index, "Band " + index + " Q", QRange, inverseRootTwo));
        layout.add(std::make_unique<juce::AudioParameterChoice>("Type" + index, "Band " + index + " Type", 
            juce::StringArray {"HighPass", "HighShelf", "LowPass", "LowShelf", "PeakFilter"}, 4));
    }
    return layout;
}
void QuasarEQAudioProcessor::updateFilters()
{
    for (int i = 0; i < NUM_BANDS; ++i)
    {
        juce::String index = juce::String(i + 1);
        const double sampleRate = getSampleRate();
        float freq = apvts.getRawParameterValue("Freq" + index)->load();
        float gainDB = apvts.getRawParameterValue("Gain" + index)->load();
        float q = apvts.getRawParameterValue("Q" + index)->load();
        int typeIndex = static_cast<int>(apvts.getRawParameterValue("Type" + index)->load());
        float gainLinear = juce::Decibels::decibelsToGain(gainDB);
        juce::dsp::IIR::Coefficients<float>::Ptr coefs;
        switch (typeIndex)
        {
        case HighPass:
            coefs = juce::dsp::IIR::Coefficients<float>::makeHighPass(sampleRate, freq, q);
            break;
        case HighShelf:
            coefs = juce::dsp::IIR::Coefficients<float>::makeHighShelf(sampleRate, freq, q, gainLinear);
            break;
        case LowPass:
            coefs = juce::dsp::IIR::Coefficients<float>::makeLowPass(sampleRate, freq, q);
            break;
        case LowShelf:
            coefs = juce::dsp::IIR::Coefficients<float>::makeLowShelf(sampleRate, freq, q, gainLinear);
            break;
        case PeakFilter:
            coefs = juce::dsp::IIR::Coefficients<float>::makePeakFilter(sampleRate, freq, q, gainLinear);
            break;
        default:
            coefs = juce::dsp::IIR::Coefficients<float>::makePeakFilter(sampleRate, freq, q, gainLinear);
            break;
        }
        leftFilters[i].coefficients = coefs;
        rightFilters[i].coefficients = coefs;
    }
    parametersChanged.store(false);
}
void QuasarEQAudioProcessor::parameterChanged(const juce::String& parameterID, float newValue)
{
    parametersChanged.store(true);
}
