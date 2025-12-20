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

    std::atomic<float>* outGainParam = nullptr;
    std::atomic<float>* bypassParam = nullptr;
    struct BandParamCache
    {
        std::atomic<float>* f = nullptr;
        std::atomic<float>* g = nullptr;
        std::atomic<float>* q = nullptr;
        std::atomic<float>* t = nullptr;
    };
    std::array<BandParamCache, NUM_BANDS> bandParams;

    CoefPtrArray coefsBuffer;
    CoefPtrArray sharedCoefficients;
    std::atomic<bool> parametersChanged {true};
    void updateFilters();
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

    using NumericType = float;
    using CoefCreator = juce::dsp::IIR::Coefficients<NumericType>::Ptr(*)(double, NumericType, NumericType, NumericType);
    static constexpr CoefCreator coefCreators[] = {
        [](auto sr, auto f, auto q, auto g) { return juce::dsp::IIR::Coefficients<NumericType>::makeHighPass(sr, f, q); },
        [](auto sr, auto f, auto q, auto g) { return juce::dsp::IIR::Coefficients<NumericType>::makeHighShelf(sr, f, q, g); },
        [](auto sr, auto f, auto q, auto g) { return juce::dsp::IIR::Coefficients<NumericType>::makeLowPass(sr, f, q); },
        [](auto sr, auto f, auto q, auto g) { return juce::dsp::IIR::Coefficients<NumericType>::makeLowShelf(sr, f, q, g); },
        [](auto sr, auto f, auto q, auto g) { return juce::dsp::IIR::Coefficients<NumericType>::makePeakFilter(sr, f, q, g); },
        [](auto sr, auto f, auto q, auto g) { return juce::dsp::IIR::Coefficients<NumericType>::makeHighShelf(sr, f, q, g); }
    };
    juce::dsp::ProcessorChain<juce::dsp::Gain<NumericType>> outGain;
};
