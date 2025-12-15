#pragma once
#include <JuceHeader.h>
#include <algorithm>
#include <vector>
#include <iterator>
#include "PluginProcessor.h"

struct SpectrumRenderData
{
	std::vector<float> spectrumPath;
	std::vector<float> peakHoldPath;
	float leftDB = -100.0f;
	float rightDB = -100.0f;
};
class PathProducer
{
public:
	PathProducer(SingleChannelSampleFifo& leftScsf, SingleChannelSampleFifo& rightScsf);
	void process(float minDB, double sampleRate);
	int getNumPathsAvailable() const;
	bool getPath(SpectrumRenderData& path);
	std::vector<float> makeFreqLUT(const double sampleRate, const float minHz, const float maxHz) const;
private:
	// DO NOT CHANGE THESE VALUES UNLESS YOU KNOW EXACTLY WHAT THEY DO
	static constexpr int FFT_ORDER = 12;
	static constexpr int NUM_SECTIONS = 1 << 3;
	static constexpr int SECTION_SIZE = 1 << 8;
	static constexpr int NUM_BINS = 1 << (FFT_ORDER - 1);
	static constexpr int FFT_SIZE = 1 << FFT_ORDER;
	static constexpr int FFT_OUT_SIZE = 1 << (FFT_ORDER + 1);
	static constexpr int RENDER_OUT_SIZE = 510;
	static constexpr float INVERSE_NUM_BINS = 1.0f / (1 << (FFT_ORDER - 1));
	static constexpr float SMOOTHING_TIME_CONSTANT = 0.02f;
	static constexpr float PEAK_DECAY_RATE = 80.0f;
	static constexpr float LEVEL_METER_SMOOTHING_TIME_CONSTANT = SMOOTHING_TIME_CONSTANT * 5.0f;
	//----------------------------------------------------------------
	SingleChannelSampleFifo* leftChannelFifo;
	SingleChannelSampleFifo* rightChannelFifo;
	juce::AudioBuffer<float> monoBufferL;
	juce::AudioBuffer<float> monoBufferR;
	juce::AudioBuffer<float> fftBuffer;
	juce::AudioBuffer<float> monoAverageBuffer;
	juce::dsp::FFT fft {FFT_ORDER};
	juce::dsp::WindowingFunction<float> windowing {size_t(FFT_SIZE), juce::dsp::WindowingFunction<float>::blackmanHarris, true};
	void generatePath(const float* renderData, const float deltaTime);
	std::vector<float> peakFallVelocity;
	std::vector<float> peakHoldDecibels;
	std::vector<float> Gains;
	std::vector<float> SmoothGains;
	std::vector<float> currentDecibels;
	float currentLeftGain = 0.0f;
	float currentRightGain = 0.0f;
	float smoothedLeftGain = 0.0f;
	float smoothedRightGain = 0.0f;
	Fifo<SpectrumRenderData> pathFifo;
};