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

    using T = float;
    template <juce::dsp::IIR::Coefficients<T>::Ptr (*F)(double, T, T, T)>
    static constexpr juce::dsp::IIR::Coefficients<T>::Ptr wrap(double sr, T f, T q, T g) { return F(sr, f, q, g); }
    template <juce::dsp::IIR::Coefficients<T>::Ptr (*F)(double, T, T)>
    static constexpr juce::dsp::IIR::Coefficients<T>::Ptr wrap(double sr, T f, T q, T) { return F(sr, f, q); }
    static constexpr juce::dsp::IIR::Coefficients<T>::Ptr (*filterFactories[])(double, T, T, T) = {
        wrap<juce::dsp::IIR::Coefficients<T>::makeHighPass>,
        wrap<juce::dsp::IIR::Coefficients<T>::makeHighShelf>,
        wrap<juce::dsp::IIR::Coefficients<T>::makeLowPass>,
        wrap<juce::dsp::IIR::Coefficients<T>::makeLowShelf>,
        wrap<juce::dsp::IIR::Coefficients<T>::makePeakFilter>,
        wrap<juce::dsp::IIR::Coefficients<T>::makeHighShelf>
    };

    juce::dsp::ProcessorChain<juce::dsp::Gain<T>> outGain;
    using Param = std::remove_pointer_t<decltype(std::declval<juce::AudioProcessorValueTreeState>().getRawParameterValue(""))>;
    Param* outGainParam = nullptr;
    Param* bypassParam = nullptr;
    struct BandParamCache
    {
        Param* f = nullptr;
        Param* g = nullptr;
        Param* q = nullptr;
        Param* t = nullptr;
    };
    std::array<BandParamCache, NUM_BANDS> bandParams;
    template <std::size_t... Is>
    void updateAllBands(const double sr, std::index_sequence<Is...>)
    {
        (([this, sr](size_t i)
            {
                const auto& bandP = bandParams[i];
                const auto bandF = juce::jmin(bandP.f->load(), static_cast<float>(sr * 0.49));
                const auto bandQ = bandP.q->load();
                const auto bandG = juce::Decibels::decibelsToGain(bandP.g->load());
                const auto bandT = static_cast<int>(bandP.t->load());
                coefsBuffer[i] = filterFactories[bandT](sr, bandF, bandQ, bandG);
            }(Is)), ...);
    }
};
