#pragma once

#include <JuceHeader.h>
#include "QuasarHeader.h"

class CustomLNF: public juce::LookAndFeel_V4
{
public:
    void drawRotarySlider(juce::Graphics& g, int x, int y, int width, int height,
        float sliderPosProportional, float rotaryStartAngle,
        float rotaryEndAngle, juce::Slider& slider) override
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
        g.setColour(quasar::colours::disabled.withAlpha(0.3f));
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
        auto bounds = juce::Rectangle<int>(0, 0, width, height).toFloat().reduced(0.5f);
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
        auto valueRect = track.withTop(sliderPos);
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
    FilterBandControl(juce::AudioProcessorValueTreeState& apvts, int bandIndex);
    ~FilterBandControl() override;
    void resized() override;
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