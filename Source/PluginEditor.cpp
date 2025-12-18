#include "PluginProcessor.h"
#include "PluginEditor.h"

QuasarEQAudioProcessorEditor::QuasarEQAudioProcessorEditor(QuasarEQAudioProcessor& p):
    AudioProcessorEditor(&p),
    audioProcessor(p),
    visualizerComponent(p),
    pluginInfoComponent()
{
    audioProcessor.addChangeListener(this);
    for (int i = 0; i < 8; ++i)
    {
        bandControls.push_back(std::make_unique<FilterBandControl>(audioProcessor.apvts, i));
        addAndMakeVisible(*bandControls.back());
    }
    addAndMakeVisible(visualizerComponent);
    addAndMakeVisible(pluginInfoComponent);
    gainSlider.setLookAndFeel(&customLNF);
    addAndMakeVisible(gainSlider);
    addAndMakeVisible(bypathButton);
    outGainAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(audioProcessor.apvts, "outGain", gainSlider);
    bypassAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(audioProcessor.apvts, "bypass", bypathButton);
    setSize(664, 650);
}
QuasarEQAudioProcessorEditor::~QuasarEQAudioProcessorEditor()
{
    audioProcessor.removeChangeListener(this);
}
void QuasarEQAudioProcessorEditor::paint(juce::Graphics& g)
{
    g.fillAll(BACKGROUND_COLOR);
}
void QuasarEQAudioProcessorEditor::resized()
{
    const int margin = 12;
    const int topSectionHeight = 20;
    const int midSectionHeight = 290;
    const int analyzerSectionWidth = 640;
    const int knobsSectionHeight = 320;
    juce::Rectangle<int> mainArea = getLocalBounds().reduced(margin);
    juce::Rectangle<int> aaa = mainArea.removeFromTop(topSectionHeight);
    bypathButton.setBounds(aaa.removeFromLeft(topSectionHeight).reduced(2));
    aaa.removeFromLeft(margin);
    pluginInfoComponent.setBounds(aaa);
    mainArea.removeFromTop(margin);
    visualizerComponent.setBounds(mainArea.removeFromTop(midSectionHeight));
    mainArea.removeFromTop(margin);
    juce::Rectangle<int> bandArea = mainArea.removeFromTop(knobsSectionHeight);
    gainSlider.setBounds(bandArea.removeFromRight(20 * 3));
    int bandWidth = bandArea.getWidth() / audioProcessor.NUM_BANDS;
    for (int i = 0; i < audioProcessor.NUM_BANDS; ++i)
    {
        if (bandControls[i])
        {
            bandControls[i]->setBounds(bandArea.removeFromLeft(bandWidth));
        }
    }
}
void QuasarEQAudioProcessorEditor::changeListenerCallback(juce::ChangeBroadcaster* source)
{
    if (source == &audioProcessor)
    {
        visualizerComponent.parametersNeedUpdate = true;
    }
}
