#pragma once

#include <JuceHeader.h>

class CustomGainSlider: public juce::Slider
{
public:
    CustomGainSlider();
    void mouseDoubleClick (const juce::MouseEvent& event) override;
};