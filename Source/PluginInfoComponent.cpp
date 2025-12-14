#include "PluginInfoComponent.h"
#include "QuasarHeader.h"

PluginInfoComponent::PluginInfoComponent()
{
    infoText = juce::String(JucePlugin_Name) + " v" + juce::String(JucePlugin_VersionString);
}
void PluginInfoComponent::paint(juce::Graphics& g)
{
    g.setColour(quasar::colours::staticText);
    g.setFont(FONT_HEIGHT);
    g.drawText(infoText, getLocalBounds(), juce::Justification::centredLeft, false);
}