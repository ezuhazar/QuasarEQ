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
    SingleChannelSampleFifo leftChannelFifo {Channel::Left};
    SingleChannelSampleFifo rightChannelFifo {Channel::Right};
    juce::AudioProcessorValueTreeState apvts;
    static constexpr int NUM_BANDS = 8;
    void parameterChanged(const juce::String& parameterID, float newValue);
    using CoefPtrArray = std::array<juce::dsp::IIR::Coefficients<float>::Ptr, NUM_BANDS>;
    CoefPtrArray getSharedCoefficients() const;
private:
    static constexpr float MIN_FREQ = 20.0f;
    static constexpr float MAX_FREQ = 20000.0f;
    static constexpr float MIN_GAIN = -24.0f;
    static constexpr float MAX_GAIN = 24.0f;
    static constexpr float MIN_Q = 0.05f;
    static constexpr float MAX_Q = 12.0f;
    static constexpr float GAIN_INTERVAL = 0.01f;
    static constexpr float FREQ_INTERVAL = 0.1f;
    static constexpr float Q_INTERVAL = 0.001f;
    const float FREQ_RATIO = std::pow(MAX_FREQ / MIN_FREQ, 1.0 / static_cast<double>(NUM_BANDS + 1));
    const float CENTRE_GAIN = 0.0f;
    const float CENTRE_FREQ = std::sqrtf(MIN_FREQ * MAX_FREQ);
    const float CENTRE_Q = 1.0f / juce::MathConstants<float>::sqrt2;
    const int DEFAULT_FILTER = PeakFilter;
    CoefPtrArray coefsBuffer;
    CoefPtrArray sharedCoefficients;
    std::atomic<bool> parametersChanged {true};
    void updateFilters();
    enum FilterType { HighPass, HighShelf, LowPass, LowShelf, PeakFilter };
    juce::dsp::ProcessorChain<juce::dsp::Gain<float>> outputGain;
    template <typename T, size_t N, typename... Args> struct RepeatTypeHelper: RepeatTypeHelper<T, N - 1, T, Args...> {};
    template <typename T, typename... Args> struct RepeatTypeHelper<T, 0, Args...> { using Type = juce::dsp::ProcessorChain<Args...>; };
    typename RepeatTypeHelper<juce::dsp::ProcessorDuplicator<juce::dsp::IIR::Filter<float>, juce::dsp::IIR::Coefficients<float>>, NUM_BANDS>::Type filterChain;
    juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout() const;
    juce::CriticalSection coefficientsLock;
    template <size_t... I>
    void updateFilterChainCoefficients(const CoefPtrArray& newCoefs, bool isBypassed, std::index_sequence<I...>)
    {
        ((*filterChain.get<I>().state = *newCoefs[I], filterChain.setBypassed<I>(isBypassed)), ...);
    }
};
