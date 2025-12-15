#pragma once

#include <JuceHeader.h>
#include "QFifo.h"

class QuasarEQAudioProcessor: public juce::AudioProcessor, public juce::ChangeBroadcaster, public juce::AudioProcessorValueTreeState::Listener
{
public:
    QuasarEQAudioProcessor();
    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;
#ifndef JucePlugin_PreferredChannelConfigurations
    bool isBusesLayoutSupported(const BusesLayout& layouts) const override;
#endif
    void processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer&) override;
    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override;
    const juce::String getName() const override;
    bool acceptsMidi() const override;
    bool producesMidi() const override;
    bool isMidiEffect() const override;
    double getTailLengthSeconds() const override;
    int getNumPrograms() override;
    int getCurrentProgram() override;
    void setCurrentProgram(int index) override;
    const juce::String getProgramName(int index) override;
    void changeProgramName(int index, const juce::String& newName) override;
    void getStateInformation(juce::MemoryBlock& destData) override;
    void setStateInformation(const void* data, int sizeInBytes) override;
    //--------------------------------------------------------------------------------
    SingleChannelSampleFifo leftChannelFifo {Channel::Left};
    SingleChannelSampleFifo rightChannelFifo {Channel::Right};
    juce::AudioProcessorValueTreeState apvts;
    static constexpr int NUM_BANDS = 8;
    using Filter = juce::dsp::IIR::Filter<float>;
    std::array<Filter, NUM_BANDS> leftFilters;
    std::array<Filter, NUM_BANDS> rightFilters;
    std::atomic<bool> parametersChanged {true};
    void updateFilters();
    void parameterChanged(const juce::String& parameterID, float newValu);
    const std::array<juce::dsp::IIR::Coefficients<float>::Ptr, NUM_BANDS>& getSharedCoefficients() const
    {
        return sharedCoefficients;
    }
private:
    //--------------------------------------------------------------------------------
    juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();

    /*
    using FilterBand = juce::dsp::ProcessorDuplicator<juce::dsp::IIR::Filter<float>, juce::dsp::IIR::Coefficients<float>>;
    using Gain = juce::dsp::Gain<float>;
    juce::dsp::ProcessorChain<FilterBand, FilterBand, FilterBand, FilterBand, FilterBand, FilterBand, FilterBand, FilterBand, Gain> filter;
    */
    juce::CriticalSection coefficientsLock;
    std::array<juce::dsp::IIR::Coefficients<float>::Ptr, NUM_BANDS> sharedCoefficients;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(QuasarEQAudioProcessor);
};
