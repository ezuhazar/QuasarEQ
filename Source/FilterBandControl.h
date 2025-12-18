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
        g.setColour(juce::Colour(0xff2d2d2d));
        g.fillEllipse(sliderBounds.reduced(lineThickness + 2.0f));
        juce::Path backgroundArc;
        backgroundArc.addCentredArc(centerX, centerY, radius, radius, 0.0f, rotaryStartAngle, rotaryEndAngle, true);
        g.setColour(juce::Colours::black.withAlpha(0.3f));
        g.strokePath(backgroundArc, juce::PathStrokeType(lineThickness, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
        juce::Path valueArc;
        valueArc.addCentredArc(centerX, centerY, radius, radius, 0.0f, centerAngle, toAngle, true);
        g.setColour(quasar::colours::enabled);
        g.strokePath(valueArc, juce::PathStrokeType(lineThickness, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
        g.setColour(juce::Colours::white);
        auto pX = centerX + (radius - 5.0f) * std::sin(toAngle);
        auto pY = centerY - (radius - 5.0f) * std::cos(toAngle);
        g.fillEllipse(pX - 1.0f, pY - 1.0f, 2.0f, 2.0f);
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