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
    void parameterChanged(const juce::String& parameterID, float newValu);
    std::array<juce::dsp::IIR::Coefficients<float>::Ptr, NUM_BANDS> QuasarEQAudioProcessor::getSharedCoefficients() const;
private:
    //--------------------------------------------------------------------------------
    void updateFilters();
    std::atomic<bool> parametersChanged {true};
    std::array<juce::dsp::IIR::Coefficients<float>::Ptr, NUM_BANDS> sharedCoefficients;
    enum FilterType;
    using Gain = juce::dsp::Gain<float>;
    using FilterBand = juce::dsp::ProcessorDuplicator<juce::dsp::IIR::Filter<float>, juce::dsp::IIR::Coefficients<float>>;
    juce::dsp::ProcessorChain<Gain> outputGain;
    juce::dsp::ProcessorChain<FilterBand, FilterBand, FilterBand, FilterBand, FilterBand, FilterBand, FilterBand, FilterBand> filterChain;
    juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();
    juce::CriticalSection coefficientsLock;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(QuasarEQAudioProcessor);
    template <size_t... I>
    void updateFilterChainCoefficients(
        const std::array<juce::dsp::IIR::Coefficients<float>::Ptr, QuasarEQAudioProcessor::NUM_BANDS>& newCoefs,
        bool isBypassed,
        std::index_sequence<I...>)
    {
        ((*filterChain.get<I>().state = *newCoefs[I], filterChain.setBypassed<I>(isBypassed)), ...);
    }
};
