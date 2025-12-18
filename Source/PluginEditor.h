#pragma once

#include <JuceHeader.h>
#include "PluginProcessor.h"
#include "ResponseCurveComponent.h"
#include "FilterBandControl.h"
#include "QuasarHeader.h"

class PluginInfoComponent: public juce::Component
{
public:
    PluginInfoComponent()
    {
        infoText = juce::String(JucePlugin_Name) + " v" + juce::String(JucePlugin_VersionString);
    };
    void paint(juce::Graphics& g) override
    {
        g.setColour(quasar::colours::staticText);
        g.setFont(FONT_HEIGHT);
        g.drawText(infoText, getLocalBounds(), juce::Justification::centredLeft, false);
    };
private:
    static constexpr int FONT_HEIGHT = 20;
    juce::String infoText;
};
class PowerButton: public juce::Button
{
public:
    PowerButton(): juce::Button("PowerButton")
    {
        setClickingTogglesState(true);
    };
    void paintButton(juce::Graphics& g, bool isMouseOverButton, bool isButtonDown) override
    {
        g.setColour(getToggleState() ? quasar::colours::enabled : quasar::colours::disabled.withAlpha(0.3f));
        g.fillRoundedRectangle(getLocalBounds().toFloat(), 2.5f);
    };
    void mouseEnter(const juce::MouseEvent& event) override
    {
        setMouseCursor(juce::MouseCursor::PointingHandCursor);
    };
    void mouseExit(const juce::MouseEvent& event) override
    {
        setMouseCursor(juce::MouseCursor::NormalCursor);
    };
};
class CustomGainSlider: public juce::Slider
{
public:
    CustomGainSlider()
    {
        setSliderStyle(juce::Slider::SliderStyle::LinearVertical);
        setTextBoxStyle(juce::Slider::TextBoxBelow, false, 50, 20);
    };
    void mouseDoubleClick (const juce::MouseEvent& event) override
    {
        // Please keep the empty override. I want to disable the double-click parameter reset.
    };
};
class QuasarEQAudioProcessorEditor: public juce::AudioProcessorEditor, public juce::ChangeListener
{
public:
    QuasarEQAudioProcessorEditor(QuasarEQAudioProcessor&);
    ~QuasarEQAudioProcessorEditor();
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
