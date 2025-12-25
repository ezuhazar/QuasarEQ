#pragma once

#include <JuceHeader.h>
#include "QFifo.h"

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
    void parameterChanged(const juce::String& parameterID, float newValue) { parametersChanged.store(true); };

    juce::AudioProcessorEditor* createEditor() override;

    using T = float;

    SingleChannelSampleFifo leftChannelFifo {Channel::Left};
    SingleChannelSampleFifo rightChannelFifo {Channel::Right};
    juce::AudioProcessorValueTreeState apvts;
    static constexpr int NUM_BANDS = 8;

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

    std::array<juce::dsp::IIR::Coefficients<T>::Ptr, NUM_BANDS> coefsBuffer;
    std::atomic<bool> parametersChanged {true};

    void updateFilters()
    {
        const auto sr = getSampleRate();
        for (size_t i = 0; i < NUM_BANDS; ++i)
        {
            const juce::String idx = juce::String(i + 1);
            const auto bandF = juce::jmin(apvts.getRawParameterValue("Freq" + idx)->load(), static_cast<float>(sr * 0.49));
            const auto bandQ = apvts.getRawParameterValue("Q" + idx)->load();
            const auto bandG = juce::Decibels::decibelsToGain(apvts.getRawParameterValue("Gain" + idx)->load());
            const auto bandT = static_cast<int>(apvts.getRawParameterValue("Type" + idx)->load());
            coefsBuffer[i] = filterFactories[bandT](sr, bandF, bandQ, bandG);
        }
        const auto isBypass = static_cast<bool>(apvts.getRawParameterValue("bypass")->load());
        const auto g = apvts.getRawParameterValue("outGain")->load();
        outGain.setBypassed<0>(isBypass);
        outGain.get<0>().setGainDecibels(g);
        updateFilterChainCoefficients(coefsBuffer, isBypass, std::make_index_sequence<NUM_BANDS> {});
        parametersChanged.store(false);
    };

    template <size_t... I>
    void updateFilterChainCoefficients(const std::array<juce::dsp::IIR::Coefficients<T>::Ptr, NUM_BANDS>& newCoefs, bool isBypassed, std::index_sequence<I...>)
    {
        ((*filterChain.get<I>().state = *newCoefs[I], filterChain.setBypassed<I>(isBypassed)), ...);
    }

    template <typename T, size_t N, typename... Args> struct RepeatTypeHelper: RepeatTypeHelper<T, N - 1, T, Args...> {};
    template <typename T, typename... Args> struct RepeatTypeHelper<T, 0, Args...> { using Type = juce::dsp::ProcessorChain<Args...>; };
    typename RepeatTypeHelper<juce::dsp::ProcessorDuplicator<juce::dsp::IIR::Filter<float>, juce::dsp::IIR::Coefficients<float>>, NUM_BANDS>::Type filterChain;
    juce::dsp::ProcessorChain<juce::dsp::Gain<T>> outGain;

    juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout() const
    {
        const float FREQ_RATIO = std::pow(MAX_FREQ / MIN_FREQ, 1.0 / static_cast<double>(NUM_BANDS + 1));
        const float CENTRE_GAIN = 0.0f;
        const float CENTRE_FREQ = std::sqrtf(MIN_FREQ * MAX_FREQ);
        const float CENTRE_Q = 1.0f / juce::MathConstants<float>::sqrt2;
        const int DEFAULT_FILTER = 4;
        const juce::StringArray filterTags {"HighPass", "HighShelf", "LowPass", "LowShelf", "PeakFilter"};
        juce::NormalisableRange<float> gainRange (MIN_GAIN, MAX_GAIN, GAIN_INTERVAL);
        juce::NormalisableRange<float> FreqRange (MIN_FREQ, MAX_FREQ, FREQ_INTERVAL);
        juce::NormalisableRange<float> QRange (MIN_Q, MAX_Q, Q_INTERVAL);
        FreqRange.setSkewForCentre(CENTRE_FREQ);
        QRange.setSkewForCentre(CENTRE_Q);
        juce::AudioProcessorValueTreeState::ParameterLayout layout;
        layout.add(std::make_unique<juce::AudioParameterFloat>("outGain", "Out Gain", gainRange, CENTRE_GAIN, "dB"));
        layout.add(std::make_unique<juce::AudioParameterBool>("bypass", "Bypass", false));
        float currentFrequency = MIN_FREQ;
        for (int i = 0; i < NUM_BANDS; ++i)
        {
            const juce::String index = juce::String(i + 1);
            currentFrequency *= FREQ_RATIO;
            layout.add(std::make_unique<juce::AudioParameterFloat>("Freq" + index, "Band " + index + " Freq", FreqRange, currentFrequency, "Hz"));
            layout.add(std::make_unique<juce::AudioParameterFloat>("Gain" + index, "Band " + index + " Gain", gainRange, CENTRE_GAIN, "dB"));
            layout.add(std::make_unique<juce::AudioParameterFloat>("Q" + index, "Band " + index + " Q", QRange, CENTRE_Q));
            layout.add(std::make_unique<juce::AudioParameterChoice>("Type" + index, "Band " + index + " Type", filterTags, DEFAULT_FILTER));
        }
        return layout;
    };
};
