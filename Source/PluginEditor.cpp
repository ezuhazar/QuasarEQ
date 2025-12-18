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
    const int margin = 6;
    const int topSectionHeight = 42;
    const int midSectionHeight = 290;
    const int botSectionHeight = 320;
    juce::Rectangle<int> mainArea = getLocalBounds().reduced(margin);
    juce::Rectangle<int> top = mainArea.removeFromTop(topSectionHeight).reduced(margin);
    juce::Rectangle<int> mid = mainArea.removeFromTop(midSectionHeight).reduced(margin);
    juce::Rectangle<int> bot = mainArea.removeFromTop(botSectionHeight).reduced(margin);
    int cccc = top.getHeight();
    bypathButton.setBounds(top.removeFromLeft(cccc).reduced(margin));
    pluginInfoComponent.setBounds(top.reduced(margin));
    visualizerComponent.setBounds(mid);
    gainSlider.setBounds(bot.removeFromRight(20 * 3));
    int bandWidth = bot.getWidth() / audioProcessor.NUM_BANDS;
    for (int i = 0; i < audioProcessor.NUM_BANDS; ++i)
    {
        if (bandControls[i])
        {
            bandControls[i]->setBounds(bot.removeFromLeft(bandWidth));
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
