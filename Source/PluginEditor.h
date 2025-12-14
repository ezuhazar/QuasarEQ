#pragma once

#include <JuceHeader.h>
#include "PluginProcessor.h"
#include "ResponseCurveComponent.h"
#include "PowerButton.h"
#include "PluginInfoComponent.h"
#include "MyGainSlider.h"
#include "FilterBandControl.h"

class QuasarEQAudioProcessorEditor: public juce::AudioProcessorEditor, public juce::ChangeListener
{
public:
    QuasarEQAudioProcessorEditor(QuasarEQAudioProcessor&);
    void paint(juce::Graphics&) override;
    void resized() override;
    //-----------------------------------------------------
    void changeListenerCallback(juce::ChangeBroadcaster* source) override;
private:
    QuasarEQAudioProcessor& audioProcessor;
    //-----------------------------------------------------
    const juce::Colour BACKGROUND_COLOR = juce::Colour(juce::uint8(40), juce::uint8(42), juce::uint8(50));
    PowerButton bypathButton;
    CustomGainSlider gainSlider;
    VisualizerComponent visualizerComponent;
    PluginInfoComponent pluginInfoComponent;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> bypassAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> outGainAttachment;
    std::vector<std::unique_ptr<FilterBandControl>> bandControls;
};
