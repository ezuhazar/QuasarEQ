#include "FilterBandControl.h" 
#include "QuasarHeader.h"

FilterBandControl::FilterBandControl(juce::AudioProcessorValueTreeState& apvts, int bandIndex)
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
    typeComboBox.addItem("HighPass", 1);
    typeComboBox.addItem("HighShelf", 2);
    typeComboBox.addItem("LowPass", 3);
    typeComboBox.addItem("LowShelf", 4);
    typeComboBox.addItem("PeakFilter", 5);
    addAndMakeVisible(typeComboBox);
    typeAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(apvts, "Type" + index, typeComboBox);
}
FilterBandControl::~FilterBandControl()
{
    freqSlider.setLookAndFeel(nullptr);
    gainSlider.setLookAndFeel(nullptr);
    qSlider.setLookAndFeel(nullptr);
}
void FilterBandControl::resized()
{
    auto bounds = getLocalBounds().reduced(5);
    typeComboBox.setBounds(bounds.removeFromTop(30).reduced(2));
    int controlHeight = bounds.getHeight() / 3;
    freqSlider.setBounds(bounds.removeFromTop(controlHeight).reduced(2));
    gainSlider.setBounds(bounds.removeFromTop(controlHeight).reduced(2));
    qSlider.setBounds(bounds.reduced(2));
}
