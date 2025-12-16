#pragma once

#include <JuceHeader.h>
#include "PluginProcessor.h" 
#include "PathProducer.h" 

class VisualizerComponent: public juce::Component, private juce::AsyncUpdater
{
public:
	VisualizerComponent(QuasarEQAudioProcessor&);
	void paint(juce::Graphics& g) override;
	bool parametersNeedUpdate = true;
private:
	void handleAsyncUpdate() override;
	void resized() override;
	juce::Rectangle<int> getLevelMeterArea();
	juce::Rectangle<int> getCurveArea();
	juce::Path createBezierPath(const std::vector<juce::Point<float>>& points);
	static constexpr float MIN_HZ = 20.0f;
	static constexpr float MAX_HZ = 20000.0f;
	static constexpr float MIN_DBFS = -90.0f;
	static constexpr float MAX_DBFS = 30.0f;
	static constexpr int HALF_FONT_HEIGHT = 5;
	static constexpr int FONT_HEIGHT = HALF_FONT_HEIGHT * 2;
	static constexpr int margin = 10;
	static constexpr int THREAD_SLEEP_TIME = 10;
	const std::vector<float> gridFrequencies = {
		20.0f, 50.0f, 100.0f, 200.0f, 500.0f, 1000.0f, 2000.0f, 5000.0f, 10000.0f, 20000.0f
	};
	const std::vector<juce::String> frequencyTags = {
		"20", "50", "100", "200", "500", "1k", "2k", "5k", "10k", "20k"
	};
	const std::vector<juce::String> dbTags = {
		"+24", "+18", "+12", "+6", "0", "-6", "-12", "-18", "-24"
	};
	const std::vector<juce::String> meterTags = {
		"+6", "+3", "0", "-3", "-6", "-9", "-12", "-15", "-18"
	};
	QuasarEQAudioProcessor& audioProcessor;
	PathProducer pathProducer;
	SpectrumRenderData channelPathToDraw;
	std::vector<juce::Point<float>> spectrumPoints;
	std::vector<juce::Point<float>> peakHoldPoints;
	std::vector<float> freqLUT;
	juce::ColourGradient gradientLUT;
	juce::CriticalSection pathLock;
	juce::CriticalSection freqLUTLock;
	juce::Image gridCache;
	juce::Path responseCurvePath;
	class AnalyzerThread: public juce::Thread
	{
	public:
		AnalyzerThread(PathProducer& producer, VisualizerComponent& comp);
		~AnalyzerThread() override;
		void run() override;
	private:
		PathProducer& producer;
		VisualizerComponent& responseCurveComponent;
	};
	AnalyzerThread analyzerThread;

	void calculateResponseCurve();
	std::vector<float> responseCurveMagnitude;
};
