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
    addAndMakeVisible(gainSlider);
    addAndMakeVisible(bypathButton);
    outGainAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(audioProcessor.apvts, "outGain", gainSlider);
    bypassAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(audioProcessor.apvts, "bypass", bypathButton);
    setSize(804, 650);
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
    const int topSectionHeight = 26;
    const int midSectionHeight = 290;
    const int analyzerSectionWidth = 640;
    const int levelMeterSectionWidth = 48;

    const int knobsSectionHeight = 320;
    bypathButton.setBounds(juce::Rectangle<int>(margin, margin, topSectionHeight, topSectionHeight).reduced(4));
    pluginInfoComponent.setBounds(juce::Rectangle<int>(topSectionHeight + margin * 2, margin, 200, topSectionHeight));
    juce::Rectangle<int> mainArea = getLocalBounds().reduced(margin);
    mainArea.removeFromTop(topSectionHeight + margin);
    juce::Rectangle<int> visualizerAndMeterArea = mainArea.removeFromTop(midSectionHeight);
    visualizerComponent.setBounds(juce::Rectangle<int>(margin, topSectionHeight + margin * 2, analyzerSectionWidth, midSectionHeight));
    gainSlider.setBounds(juce::Rectangle<int>(analyzerSectionWidth + margin * 2, topSectionHeight + margin * 2, levelMeterSectionWidth, midSectionHeight));
    juce::Rectangle<int> knobsArea = mainArea.removeFromTop(knobsSectionHeight).reduced(4);
    int bandWidth = knobsArea.getWidth() / 8;
    for (int i = 0; i < 8; ++i)
    {
        if (bandControls[i])
        {
            bandControls[i]->setBounds(knobsArea.removeFromLeft(bandWidth));
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