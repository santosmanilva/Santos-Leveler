#pragma once

#include <JuceHeader.h>
#include "PluginProcessor.h"

class SantosLevelerAudioProcessorEditor final : public juce::AudioProcessorEditor,
                                                 private juce::Timer
{
public:
    explicit SantosLevelerAudioProcessorEditor (SantosLevelerAudioProcessor&);
    ~SantosLevelerAudioProcessorEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    class SantosLookAndFeel final : public juce::LookAndFeel_V4
    {
    public:
        SantosLookAndFeel();
        void drawRotarySlider (juce::Graphics&, int x, int y, int width, int height,
                               float sliderPos, float rotaryStartAngle, float rotaryEndAngle,
                               juce::Slider&) override;
        void drawLinearSlider (juce::Graphics&, int x, int y, int width, int height,
                               float sliderPos, float minSliderPos, float maxSliderPos,
                               juce::Slider::SliderStyle, juce::Slider&) override;
    };

    class HistoryComponent final : public juce::Component
    {
    public:
        explicit HistoryComponent (SantosLevelerAudioProcessor& p) : processor (p) {}
        void paint (juce::Graphics&) override;
    private:
        SantosLevelerAudioProcessor& processor;
    };

    class MeterComponent final : public juce::Component
    {
    public:
        enum class Source { input, levelerOutput };
        MeterComponent (SantosLevelerAudioProcessor& p, Source s, juce::String text, juce::Colour c)
            : processor (p), source (s), label (std::move (text)), colour (c) {}
        void paint (juce::Graphics&) override;
    private:
        SantosLevelerAudioProcessor& processor;
        Source source;
        juce::String label;
        juce::Colour colour;
        float heldPeakDb = -100.0f;
        double peakHoldUntilMs = 0.0;
        double lastPeakUpdateMs = 0.0;
    };

    class FinalMeterComponent final : public juce::Component
    {
    public:
        explicit FinalMeterComponent (SantosLevelerAudioProcessor& p) : processor (p) {}
        void paint (juce::Graphics&) override;
    private:
        SantosLevelerAudioProcessor& processor;
        float heldPeakDb = -100.0f;
        double peakHoldUntilMs = 0.0;
        double lastPeakUpdateMs = 0.0;
    };

    class LoudnessMeterComponent final : public juce::Component
    {
    public:
        explicit LoudnessMeterComponent (SantosLevelerAudioProcessor& p) : processor (p) {}
        void paint (juce::Graphics&) override;
    private:
        SantosLevelerAudioProcessor& processor;
    };

    void timerCallback() override;
    void configureKnob (juce::Slider&, juce::Label&, const juce::String& name,
                        const juce::String& suffix, int decimals, juce::Colour accent);
    void configureFader (juce::Slider&, juce::Label&, const juce::String& name,
                         const juce::String& suffix, int decimals, juce::Colour accent);
    void configureHeaderButton (juce::TextButton& button, const juce::String& text);
    void updateABButtons();
    void showPresetMenu();
    void applyFactoryPreset (int presetIndex);

    SantosLevelerAudioProcessor& processor;
    SantosLookAndFeel lookAndFeel;

    juce::Slider targetKnob, gateKnob, speedKnob, detectKnob, lookaheadKnob, holdKnob, releaseKnob, peakThresholdKnob;
    juce::Slider rangeDownSlider, downStrengthSlider, rangeUpSlider, upStrengthSlider, outputSlider;
    juce::Slider intensitySlider;
    juce::Slider compThresholdKnob, compRatioKnob, compAttackKnob, compReleaseKnob, compMakeupKnob, ceilingKnob;

    juce::Label targetLabel, gateLabel, speedLabel, detectLabel, lookaheadLabel, holdLabel, releaseLabel, peakThresholdLabel;
    juce::Label rangeDownLabel, downStrengthLabel, rangeUpLabel, upStrengthLabel, outputLabel;
    juce::Label intensityLabel;
    juce::Label compThresholdLabel, compRatioLabel, compAttackLabel, compReleaseLabel, compMakeupLabel, ceilingLabel;
    juce::Label titleLabel, subtitleLabel;

    juce::TextButton presetButton;
    juce::TextButton resetLoudnessButton;
    juce::TextButton aButton;
    juce::TextButton bButton;
    juce::TextButton bypassButton;
    juce::TextButton compButton;

    HistoryComponent history;
    MeterComponent inputMeter;
    MeterComponent outputMeter;
    FinalMeterComponent finalMeter;
    LoudnessMeterComponent loudnessMeter;

    juce::AudioProcessorValueTreeState::SliderAttachment targetAttachment;
    juce::AudioProcessorValueTreeState::SliderAttachment gateAttachment;
    juce::AudioProcessorValueTreeState::SliderAttachment speedAttachment;
    juce::AudioProcessorValueTreeState::SliderAttachment detectAttachment;
    juce::AudioProcessorValueTreeState::SliderAttachment lookaheadAttachment;
    juce::AudioProcessorValueTreeState::SliderAttachment holdAttachment;
    juce::AudioProcessorValueTreeState::SliderAttachment releaseAttachment;
    juce::AudioProcessorValueTreeState::SliderAttachment peakThresholdAttachment;
    juce::AudioProcessorValueTreeState::SliderAttachment rangeDownAttachment;
    juce::AudioProcessorValueTreeState::SliderAttachment downStrengthAttachment;
    juce::AudioProcessorValueTreeState::SliderAttachment rangeUpAttachment;
    juce::AudioProcessorValueTreeState::SliderAttachment upStrengthAttachment;
    juce::AudioProcessorValueTreeState::SliderAttachment outputAttachment;
    juce::AudioProcessorValueTreeState::SliderAttachment intensityAttachment;
    juce::AudioProcessorValueTreeState::SliderAttachment compThresholdAttachment;
    juce::AudioProcessorValueTreeState::SliderAttachment compRatioAttachment;
    juce::AudioProcessorValueTreeState::SliderAttachment compAttackAttachment;
    juce::AudioProcessorValueTreeState::SliderAttachment compReleaseAttachment;
    juce::AudioProcessorValueTreeState::SliderAttachment compMakeupAttachment;
    juce::AudioProcessorValueTreeState::SliderAttachment ceilingAttachment;
    juce::AudioProcessorValueTreeState::ButtonAttachment compEnabledAttachment;
    juce::AudioProcessorValueTreeState::ButtonAttachment bypassAttachment;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SantosLevelerAudioProcessorEditor)
};
