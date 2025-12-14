#include "MyGainSlider.h"

CustomGainSlider::CustomGainSlider()
{
    setSliderStyle(juce::Slider::SliderStyle::LinearVertical);
    setTextBoxStyle(juce::Slider::TextBoxBelow, false, 50, 20);
}
void CustomGainSlider::mouseDoubleClick(const juce::MouseEvent& event)
{
}