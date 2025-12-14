#pragma once

#include <JuceHeader.h>

class PluginInfoComponent: public juce::Component
{
public:
    PluginInfoComponent();
    void paint(juce::Graphics& g) override;
private:
    static constexpr int FONT_HEIGHT = 20;
    juce::String infoText;
};
