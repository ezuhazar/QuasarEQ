#pragma once

#include <JuceHeader.h>

class FilterBandControl: public juce::Component
{
public:
    FilterBandControl(juce::AudioProcessorValueTreeState& apvts, int bandIndex);
    ~FilterBandControl() override;
    void resized() override;
private:
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