#pragma once

#include <JuceHeader.h>
#include "PluginProcessor.h"
#include "UI/LookAndFeel.h"

class SantosLevelerAudioProcessorEditor final : public juce::AudioProcessorEditor,
                                                 private juce::Timer
{
public:
    explicit SantosLevelerAudioProcessorEditor (SantosLevelerAudioProcessor&);
    ~SantosLevelerAudioProcessorEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    // All controls live in a fixed-size design surface that is scaled to the window,
    // so the layout is written once in design units (see Source/UI/Theme.h).
    class Content;

    void timerCallback() override;

    SantosLevelerAudioProcessor& processor;
    SantosUI::SantosLookAndFeel lookAndFeel;
    std::unique_ptr<Content> content;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SantosLevelerAudioProcessorEditor)
};
