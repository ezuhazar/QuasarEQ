#pragma once
#include <JuceHeader.h>
#include "QFifo.h"
static inline const juce::String ID_BYPASS {"bypass"};
static inline const juce::String ID_GAIN {"outGain"};
static inline const juce::String ID_PREFIX_FREQ {"Freq"};
static inline const juce::String ID_PREFIX_GAIN {"Gain"};
static inline const juce::String ID_PREFIX_Q {"Q"};
static inline const juce::String ID_PREFIX_TYPE {"Type"};
static inline const juce::String ID_PREFIX_BYPASS {"Bypass"};
static inline const juce::String ID_PARAMETERS {"Parameters"};
static inline const juce::String NAME_BYPASS {"Bypass"};
static inline const juce::String NAME_GAIN {"Gain"};
static inline const juce::String NAME_PREFIX_FREQ {"Freq"};
static inline const juce::String NAME_PREFIX_GAIN {"Gain"};
static inline const juce::String NAME_PREFIX_Q {"Q"};
static inline const juce::String NAME_PREFIX_TYPE {"Type"};
static inline const juce::String NAME_PREFIX_BYPASS {"Bypass"};
static inline const juce::String NAME_PREFIX_BAND {"Band"};
static inline const juce::String UNIT_HZ {"Hz"};
static inline const juce::String UNIT_DB {"dB"};
static inline const juce::StringArray filterTags {"HighPass", "HighShelf", "LowPass", "LowShelf", "Peak"};
static inline const juce::StringArray bandParamPrefixes = {ID_PREFIX_FREQ, ID_PREFIX_GAIN, ID_PREFIX_Q, ID_PREFIX_TYPE, ID_PREFIX_BYPASS};
static constexpr int NUM_BANDS = 8;
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
    wrap<juce::dsp::IIR::Coefficients<T>::makePeakFilter>
};
template <typename T>
constexpr T constexpr_sqrt(T x)
{
    if (x < 0)
    {
        return std::numeric_limits<T>::quiet_NaN();
    }
    if (x == 0 || x == std::numeric_limits<T>::infinity())
    {
        return x;
    }
    T curr = x;
    T prev = 0;
    while (curr != prev)
    {
        prev = curr;
        curr = (curr + x / curr) * 0.5;
    }
    return curr;
}
template <size_t... I>
auto make_chain_type(std::index_sequence<I...>) -> juce::dsp::ProcessorChain < std::decay_t<decltype((void)I, juce::dsp::ProcessorDuplicator<juce::dsp::IIR::Filter<float>, juce::dsp::IIR::Coefficients<float>>{}) > ... > ;
template <size_t N>
using FilterChain = decltype(make_chain_type(std::make_index_sequence<N>{}));
static constexpr bool BYPASS_DEFAULT = false;
static constexpr int TYPE_DEFAULT = 4;
static constexpr float FREQ_START = 20.0f;
static constexpr float FREQ_END = 20000.0f;
static constexpr float FREQ_INTERVAL = 0.1f;
static constexpr float FREQ_CENTRE = constexpr_sqrt(FREQ_START * FREQ_END);
static constexpr float GAIN_START = -24.0f;
static constexpr float GAIN_END = 24.0f;
static constexpr float GAIN_INTERVAL = 0.01f;
static constexpr float GAIN_CENTRE = 0.0f;
static constexpr float QUAL_START = 0.05f;
static constexpr float QUAL_END = 12.0f;
static constexpr float QUAL_INTERVAL = 0.001f;
static constexpr float QUAL_CENTRE = 1.0f / juce::MathConstants<float>::sqrt2;
class QuasarEQAudioProcessor: public juce::AudioProcessor, public juce::AudioProcessorValueTreeState::Listener
{
public:
    QuasarEQAudioProcessor()
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
        , apvts(*this, nullptr, ID_PARAMETERS, createParameterLayout())
    {
        apvts.addParameterListener(ID_GAIN, this);
        apvts.addParameterListener(ID_BYPASS, this);
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
    bool isBusesLayoutSupported(const BusesLayout& layouts) const
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
    void prepareToPlay(double sampleRate, int samplesPerBlock) override
    {
        juce::dsp::ProcessSpec spec {};
        spec.sampleRate = sampleRate;
        spec.maximumBlockSize = (juce::uint32)samplesPerBlock;
        spec.numChannels = (juce::uint32)getTotalNumOutputChannels();
        leftChannelFifo.prepare(samplesPerBlock);
        rightChannelFifo.prepare(samplesPerBlock);
        filterChain.prepare(spec);
        filterChain.reset();
        outGain.prepare(spec);
        outGain.reset();
        updateFilters();
    }
    void processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages) override
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
        outGain.process(context);
        leftChannelFifo.update(buffer);
        rightChannelFifo.update(buffer);
    }
    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    bool hasEditor() const override { return true; }
    bool acceptsMidi() const override { return JucePlugin_WantsMidiInput; };
    bool isMidiEffect() const override { return JucePlugin_IsMidiEffect; };
    bool producesMidi() const override { return JucePlugin_ProducesMidiOutput; };
    void releaseResources() override {};
    void setCurrentProgram(int index) override {};
    void changeProgramName(int index, const juce::String& newName) override {};
    double getTailLengthSeconds() const override { return 0.0; };
    const juce::String getName() const override { return JucePlugin_Name; }
    const juce::String getProgramName(int index) override { return {}; }
    void getStateInformation(juce::MemoryBlock& destData) override
    {
        juce::MemoryOutputStream stream(destData, false);
        apvts.state.writeToStream(stream);
    };
    void setStateInformation(const void* data, int sizeInBytes) override
    {
        auto tree = juce::ValueTree::readFromData(data, size_t(sizeInBytes));
        if (tree.isValid())
        {
            apvts.replaceState(tree);
        }
    };
    juce::AudioProcessorEditor* createEditor() override;
    void parameterChanged(const juce::String& parameterID, float newValue)
    {
        parametersChanged.store(true);
    };
    SingleChannelSampleFifo leftChannelFifo {Channel::Left};
    SingleChannelSampleFifo rightChannelFifo {Channel::Right};
    juce::AudioProcessorValueTreeState apvts;
private:
    std::array<juce::dsp::IIR::Coefficients<T>::Ptr, NUM_BANDS> coefsBuffer;
    std::array<bool, NUM_BANDS> bandBypassStates;
    std::atomic<bool> parametersChanged {true};
    void updateFilters()
    {
        const auto sr = getSampleRate();
        const auto globalBypass = static_cast<bool>(apvts.getRawParameterValue(ID_BYPASS)->load());
        for (size_t i = 0; i < NUM_BANDS; ++i)
        {
            const juce::String idx = juce::String(i + 1);
            const auto bandF = juce::jmin(apvts.getRawParameterValue(ID_PREFIX_FREQ + idx)->load(), static_cast<float>(sr * 0.49));
            const auto bandQ = apvts.getRawParameterValue(ID_PREFIX_Q + idx)->load();
            const auto bandG = juce::Decibels::decibelsToGain(apvts.getRawParameterValue(ID_PREFIX_GAIN + idx)->load());
            const auto bandT = static_cast<int>(apvts.getRawParameterValue(ID_PREFIX_TYPE + idx)->load());
            coefsBuffer[i] = filterFactories[bandT](sr, bandF, bandQ, bandG);
            const bool individualBypass = static_cast<bool>(apvts.getRawParameterValue(ID_PREFIX_BYPASS + idx)->load());
            bandBypassStates[i] = globalBypass || individualBypass;
        }
        const auto g = apvts.getRawParameterValue(ID_GAIN)->load();
        outGain.setBypassed<0>(globalBypass);
        outGain.get<0>().setGainDecibels(g);
        updateFilterChainCoefficients(coefsBuffer, bandBypassStates, std::make_index_sequence<NUM_BANDS> {});
        parametersChanged.store(false);
    };
    template <size_t... I>
    void updateFilterChainCoefficients(const std::array<juce::dsp::IIR::Coefficients<T>::Ptr, NUM_BANDS>& newCoefs,
        const std::array<bool, NUM_BANDS>& bypassStates,
        std::index_sequence<I...>)
    {
        ((*filterChain.get<I>().state = *newCoefs[I], filterChain.setBypassed<I>(bypassStates[I])), ...);
    }
    FilterChain<NUM_BANDS> filterChain;
    juce::dsp::ProcessorChain<juce::dsp::Gain<T>> outGain;
    juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout() const
    {
        juce::NormalisableRange<float> gainRange {GAIN_START, GAIN_END, GAIN_INTERVAL};
        juce::NormalisableRange<float> freqRange {FREQ_START, FREQ_END, FREQ_INTERVAL};
        juce::NormalisableRange<float> qualRange {QUAL_START, QUAL_END, QUAL_INTERVAL};
        freqRange.setSkewForCentre(freqRange.snapToLegalValue(FREQ_CENTRE));
        qualRange.setSkewForCentre(qualRange.snapToLegalValue(QUAL_CENTRE));
        juce::AudioProcessorValueTreeState::ParameterLayout layout;
        layout.add(std::make_unique<juce::AudioParameterBool>(ID_BYPASS, NAME_BYPASS, BYPASS_DEFAULT));
        layout.add(std::make_unique<juce::AudioParameterFloat>(ID_GAIN, NAME_GAIN, gainRange, GAIN_CENTRE, UNIT_DB));
        for (int i = 0; i < NUM_BANDS; ++i)
        {
            const juce::String index = juce::String(i + 1);
            layout.add(std::make_unique<juce::AudioParameterBool>(ID_PREFIX_BYPASS + index, NAME_PREFIX_BAND + index + NAME_PREFIX_BYPASS, BYPASS_DEFAULT));
            layout.add(std::make_unique<juce::AudioParameterFloat>(ID_PREFIX_FREQ + index, NAME_PREFIX_BAND + index + NAME_PREFIX_FREQ, freqRange, freqRange.snapToLegalValue(FREQ_CENTRE), UNIT_HZ));
            layout.add(std::make_unique<juce::AudioParameterFloat>(ID_PREFIX_GAIN + index, NAME_PREFIX_BAND + index + NAME_PREFIX_GAIN, gainRange, GAIN_CENTRE, UNIT_DB));
            layout.add(std::make_unique<juce::AudioParameterFloat>(ID_PREFIX_Q + index, NAME_PREFIX_BAND + index + NAME_PREFIX_Q, qualRange, qualRange.snapToLegalValue(QUAL_CENTRE)));
            layout.add(std::make_unique<juce::AudioParameterChoice>(ID_PREFIX_TYPE + index, NAME_PREFIX_BAND + index + NAME_PREFIX_TYPE, filterTags, TYPE_DEFAULT));
        }
        return layout;
    };
};
