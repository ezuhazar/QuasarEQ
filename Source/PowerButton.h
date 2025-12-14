#pragma once

#include <JuceHeader.h>

class PowerButton: public juce::Button
{
public:
    PowerButton();
    void paintButton(juce::Graphics& g, bool isMouseOverButton, bool isButtonDown) override;
    void mouseEnter(const juce::MouseEvent& event) override;
    void mouseExit(const juce::MouseEvent& event) override;
private:
};