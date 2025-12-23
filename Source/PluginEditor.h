#pragma once

#include <JuceHeader.h>
#include "PluginProcessor.h"
#include "ResponseCurveComponent.h"

class CustomLNF: public juce::LookAndFeel_V4
{
public:

    CustomLNF()
    {
        setColour(juce::Slider::textBoxBackgroundColourId, juce::Colours::black);
    }
    void drawRotarySlider(juce::Graphics& g, int x, int y, int width, int height, float sliderPosProportional, float rotaryStartAngle, float rotaryEndAngle, juce::Slider& slider) override
    {
        auto area = juce::Rectangle<float>(x, y, width, height);
        auto size = juce::jmin(area.getWidth(), area.getHeight());
        auto sliderBounds = juce::Rectangle<float>(size, size);
        sliderBounds.setCentre(area.getCentre());
        sliderBounds = sliderBounds.reduced(5.0f);
        auto centerX = sliderBounds.getCentreX();
        auto centerY = sliderBounds.getCentreY();
        auto radius = sliderBounds.getWidth() / 2.0f;
        auto lineThickness = 3.5f;
        auto toAngle = rotaryStartAngle + sliderPosProportional * (rotaryEndAngle - rotaryStartAngle);
        auto centerAngle = rotaryStartAngle + (rotaryEndAngle - rotaryStartAngle) * 0.5f;
        g.setColour(quasar::colours::labelBackground);
        auto knobRadius = radius - lineThickness - 2.0f;
        g.fillEllipse(sliderBounds.reduced(lineThickness + 2.0f));
        juce::Path backgroundArc;
        backgroundArc.addCentredArc(centerX, centerY, radius, radius, 0.0f, rotaryStartAngle, rotaryEndAngle, true);
        g.setColour(quasar::colours::groove.withAlpha(0.3f));
        g.strokePath(backgroundArc, juce::PathStrokeType(lineThickness, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
        juce::Path valueArc;
        valueArc.addCentredArc(centerX, centerY, radius, radius, 0.0f, centerAngle, toAngle, true);
        g.setColour(quasar::colours::enabled);
        g.strokePath(valueArc, juce::PathStrokeType(lineThickness, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
        g.setColour(juce::Colours::white);
        juce::Path pointer;
        auto pointerWidth = 2.0f;
        auto pointerLength = 6.0f;
        pointer.addRoundedRectangle(-pointerWidth * 0.5f, -knobRadius, pointerWidth, pointerLength, 1.0f);
        pointer.applyTransform(juce::AffineTransform::rotation(toAngle).translated(centerX, centerY));
        g.fillPath(pointer);
    }
    void drawComboBox(juce::Graphics& g, int width, int height, bool isButtonDown,
        int buttonX, int buttonY, int buttonW, int buttonH,
        juce::ComboBox& box) override
    {
        auto cornerSize = 3.0f;
        auto bounds = juce::Rectangle<int>(0, 0, width, height).toFloat();
        g.setColour(quasar::colours::labelBackground);
        g.fillRoundedRectangle(bounds, cornerSize);
    }
    void positionComboBoxText(juce::ComboBox& box, juce::Label& label) override
    {
        label.setBounds(1, 1, box.getWidth() - 2, box.getHeight() - 2);
        label.setFont(getComboBoxFont(box));
        label.setJustificationType(juce::Justification::centred);
    }
    void drawTextEditorOutline(juce::Graphics& g, int width, int height, juce::TextEditor& te) override
    {
    }
    void drawLinearSlider(juce::Graphics& g, int x, int y, int width, int height,
        float sliderPos, float minSliderPos, float maxSliderPos,
        const juce::Slider::SliderStyle style, juce::Slider& slider) override
    {
        auto bounds = juce::Rectangle<float>(x, y, width, height).reduced(10.0f, 5.0f);
        float trackWidth = 6.0f;
        auto track = bounds.withSizeKeepingCentre(trackWidth, bounds.getHeight());
        g.setColour(juce::Colours::black.withAlpha(0.3f));
        g.fillRoundedRectangle(track, trackWidth * 0.5f);
        float zeroPos = (minSliderPos + maxSliderPos) * 0.5f;
        auto top = juce::jmin(zeroPos, sliderPos);
        auto bottom = juce::jmax(zeroPos, sliderPos);
        auto valueRect = track.withTop(top).withBottom(bottom);
        g.setColour(quasar::colours::enabled);
        g.fillRoundedRectangle(valueRect, trackWidth * 0.5f);
        auto thumbHeight = 12.0f;
        auto thumbWidth = 20.0f;
        auto thumbRect = juce::Rectangle<float>(thumbWidth, thumbHeight);
        thumbRect.setCentre(track.getCentreX(), sliderPos);
        g.setColour(quasar::colours::labelBackground);
        g.fillRoundedRectangle(thumbRect, 2.0f);
        g.setColour(juce::Colours::white.withAlpha(0.8f));
        g.drawRoundedRectangle(thumbRect, 2.0f, 1.0f);
        g.setColour(juce::Colours::white);
        g.fillRect(thumbRect.withSizeKeepingCentre(thumbWidth * 0.6f, 1.5f));
    }
};



class FilterBandControl: public juce::Component
{
public:
    FilterBandControl(juce::AudioProcessorValueTreeState& apvts, int bandIndex)
    {
        freqSlider.setLookAndFeel(&customLNF);
        gainSlider.setLookAndFeel(&customLNF);
        typeComboBox.setLookAndFeel(&customLNF);
        typeComboBox.setJustificationType(juce::Justification::centred);
        qSlider.setLookAndFeel(&customLNF);
        juce::String index = juce::String(bandIndex + 1);
        freqSlider.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
        freqSlider.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 50, 20);
        addAndMakeVisible(freqSlider);
        freqAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(apvts, "Freq" + index, freqSlider);
        gainSlider.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
        gainSlider.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 50, 20);
        addAndMakeVisible(gainSlider);
        gainAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(apvts, "Gain" + index, gainSlider);
        qSlider.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
        qSlider.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 50, 20);
        addAndMakeVisible(qSlider);
        qAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(apvts, "Q" + index, qSlider);
        typeComboBox.addItem("HPF", 1);
        typeComboBox.addItem("HSF", 2);
        typeComboBox.addItem("LPF", 3);
        typeComboBox.addItem("LSF", 4);
        typeComboBox.addItem("PF", 5);
        addAndMakeVisible(typeComboBox);
        typeAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(apvts, "Type" + index, typeComboBox);
    };
    ~FilterBandControl() override
    {
        freqSlider.setLookAndFeel(nullptr);
        gainSlider.setLookAndFeel(nullptr);
        qSlider.setLookAndFeel(nullptr);
    };
    void resized() override
    {
        auto bounds = getLocalBounds().reduced(6);
        typeComboBox.setBounds(bounds.removeFromTop(30).reduced(2));
        int controlHeight = bounds.getHeight() / 3;
        freqSlider.setBounds(bounds.removeFromTop(controlHeight).reduced(2));
        gainSlider.setBounds(bounds.removeFromTop(controlHeight).reduced(2));
        qSlider.setBounds(bounds.reduced(2));
    };
private:
    CustomLNF customLNF;
    juce::Slider freqSlider;
    juce::Slider gainSlider;
    juce::Slider qSlider;
    juce::ComboBox typeComboBox;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> freqAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> gainAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> qAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> typeAttachment;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(FilterBandControl)
};

class PluginInfoComponent: public juce::Component
{
public:
    void paint(juce::Graphics& g) override
    {
        const auto textColour = quasar::colours::staticText;
        const auto textHeight = getLocalBounds().getHeight();
        const auto text = juce::String(JucePlugin_Name) + " v" + juce::String(JucePlugin_VersionString);
        g.setColour(textColour);
        g.setFont(textHeight);
        g.drawText(text, getLocalBounds(), juce::Justification::centredLeft);
    };
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
        g.setColour(getToggleState() ? quasar::colours::enabled : quasar::colours::disabled);
        g.fillRect(getLocalBounds());
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
    QuasarEQAudioProcessorEditor(QuasarEQAudioProcessor& p): AudioProcessorEditor(&p), audioProcessor(p), visualizerComponent(p), pluginInfoComponent()
    {
        audioProcessor.addChangeListener(this);
        for (int i = 0; i < audioProcessor.NUM_BANDS; ++i)
        {
            bandControls.push_back(std::make_unique<FilterBandControl>(audioProcessor.apvts, i));
            addAndMakeVisible(*bandControls.back());
        }
        addAndMakeVisible(visualizerComponent);
        addAndMakeVisible(pluginInfoComponent);
        gainSlider.setLookAndFeel(&customLNF);
        addAndMakeVisible(gainSlider);
        addAndMakeVisible(bypathButton);
        outGainAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(audioProcessor.apvts, "outGain", gainSlider);
        bypassAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(audioProcessor.apvts, "bypass", bypathButton);
        setSize(652, 652);
    };
    ~QuasarEQAudioProcessorEditor()
    {
        audioProcessor.removeChangeListener(this);
    };
    void paint(juce::Graphics& g) override
    {
        g.fillAll(BACKGROUND_COLOR);
    };
    void resized() override
    {
        const int margin = 6;
        const int topSectionHeight = 40;
        const int midSectionHeight = 300;
        const int botSectionHeight = 300;
        juce::Rectangle<int> mainArea = getLocalBounds().reduced(margin);
        juce::Rectangle<int> top = mainArea.removeFromTop(topSectionHeight).reduced(margin);
        juce::Rectangle<int> mid = mainArea.removeFromTop(midSectionHeight).reduced(margin);
        juce::Rectangle<int> bot = mainArea.removeFromTop(botSectionHeight).reduced(margin);
        const int sideSize = top.getHeight();
        bypathButton.setBounds(top.removeFromLeft(sideSize).reduced(margin));
        pluginInfoComponent.setBounds(top.reduced(margin));
        visualizerComponent.setBounds(mid);
        gainSlider.setBounds(bot.removeFromRight(20 * 3).reduced(margin));
        const int bandWidth = bot.getWidth() / audioProcessor.NUM_BANDS;
        for (int i = 0; i < audioProcessor.NUM_BANDS; ++i)
        {
            if (bandControls[i])
            {
                bandControls[i]->setBounds(bot.removeFromLeft(bandWidth));
            }
        }
    };
    void changeListenerCallback(juce::ChangeBroadcaster* source) override
    {

        if (source == &audioProcessor)
        {
            visualizerComponent.parametersNeedUpdate = true;
        }
    };
private:
    QuasarEQAudioProcessor& audioProcessor;
    const juce::Colour BACKGROUND_COLOR = juce::Colour(juce::uint8(40), juce::uint8(42), juce::uint8(50));
    PowerButton bypathButton;
    CustomLNF customLNF;
    CustomGainSlider gainSlider;
    VisualizerComponent visualizerComponent;
    PluginInfoComponent pluginInfoComponent;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> bypassAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> outGainAttachment;
    std::vector<std::unique_ptr<FilterBandControl>> bandControls;
};
