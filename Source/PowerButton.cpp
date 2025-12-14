#include "PowerButton.h"
#include "QuasarHeader.h"

PowerButton::PowerButton(): juce::Button("PowerButton")
{
    setClickingTogglesState(true);
}
void PowerButton::paintButton(juce::Graphics& g, bool isMouseOverButton, bool isButtonDown)
{
    juce::Colour colour = getToggleState() ?  quasar::colours::enabled : quasar::colours::disabled;
    g.setColour(colour);
    g.fillRoundedRectangle(getLocalBounds().toFloat(), 2.5f);
}
void PowerButton::mouseEnter(const juce::MouseEvent& event)
{
    setMouseCursor(juce::MouseCursor::PointingHandCursor);
}
void PowerButton::mouseExit(const juce::MouseEvent& event)
{
    setMouseCursor(juce::MouseCursor::NormalCursor);
}