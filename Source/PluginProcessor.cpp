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
    if constexpr (JucePlugin_IsMidiEffect)
    {
        return true;
    }
    const auto mainOutput = layouts.getMainOutputChannelSet();
    if (mainOutput != juce::AudioChannelSet::mono() && mainOutput != juce::AudioChannelSet::stereo())
    {
        return false;
    }
    if constexpr (!JucePlugin_IsSynth)
    {
        return mainOutput == layouts.getMainInputChannelSet();
    }
    return true;
}
#endif
int QuasarEQAudioProcessor::getNumPrograms() { return 1; }
int QuasarEQAudioProcessor::getCurrentProgram() { return 0; }
bool QuasarEQAudioProcessor::hasEditor() const { return true; }
bool QuasarEQAudioProcessor::acceptsMidi() const { return JucePlugin_WantsMidiInput; }
bool QuasarEQAudioProcessor::isMidiEffect() const { return JucePlugin_IsMidiEffect; }
bool QuasarEQAudioProcessor::producesMidi() const { return JucePlugin_ProducesMidiOutput; }
void QuasarEQAudioProcessor::releaseResources() {}
void QuasarEQAudioProcessor::setCurrentProgram(int index) {}
void QuasarEQAudioProcessor::changeProgramName(int index, const juce::String& newName) {}
double QuasarEQAudioProcessor::getTailLengthSeconds() const { return 0.0; }
const juce::String QuasarEQAudioProcessor::getName() const { return JucePlugin_Name; }
const juce::String QuasarEQAudioProcessor::getProgramName(int index) { return {}; }
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() { return new QuasarEQAudioProcessor(); }
juce::AudioProcessorEditor* QuasarEQAudioProcessor::createEditor() { return new QuasarEQAudioProcessorEditor(*this); }
