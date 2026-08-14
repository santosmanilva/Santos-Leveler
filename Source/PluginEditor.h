#pragma once

#include <JuceHeader.h>
#include "PluginProcessor.h"

class SantosLevelerAudioProcessorEditor final : public juce::AudioProcessorEditor,
                                                 private juce::Timer
{
public:
    enum class Skin
    {
        classicPro = 0,
        neon,
        modular,
        analysis,
        radar,
        broadcast
    };

    enum class Motion
    {
        full = 0,
        reduced,
        off
    };

    explicit SantosLevelerAudioProcessorEditor (SantosLevelerAudioProcessor&);
    ~SantosLevelerAudioProcessorEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    class SantosLookAndFeel final : public juce::LookAndFeel_V4
    {
    public:
        SantosLookAndFeel();
        void setSkin (Skin newSkin) noexcept { skin = newSkin; }
        void drawRotarySlider (juce::Graphics&, int x, int y, int width, int height,
                               float sliderPos, float rotaryStartAngle, float rotaryEndAngle,
                               juce::Slider&) override;
        void drawLinearSlider (juce::Graphics&, int x, int y, int width, int height,
                               float sliderPos, float minSliderPos, float maxSliderPos,
                               juce::Slider::SliderStyle, juce::Slider&) override;

    private:
        Skin skin = Skin::classicPro;
    };

    class HistoryComponent final : public juce::Component
    {
    public:
        explicit HistoryComponent (SantosLevelerAudioProcessor& p) : processor (p) {}
        void setSkin (Skin s) noexcept { skin = s; }
        void setPhase (float p) noexcept { phase = p; }
        void paint (juce::Graphics&) override;

    private:
        SantosLevelerAudioProcessor& processor;
        Skin skin = Skin::classicPro;
        float phase = 0.0f;
    };

    class MeterComponent final : public juce::Component
    {
    public:
        enum class Source { input, output };
        MeterComponent (SantosLevelerAudioProcessor& p, Source s, juce::String text, juce::Colour c)
            : processor (p), source (s), label (std::move (text)), colour (c) {}
        void setSkin (Skin s) noexcept { skin = s; }
        void paint (juce::Graphics&) override;

    private:
        SantosLevelerAudioProcessor& processor;
        Source source;
        juce::String label;
        juce::Colour colour;
        Skin skin = Skin::classicPro;
    };

    void timerCallback() override;
    void configureKnob (juce::Slider&, juce::Label&, const juce::String& name, const juce::String& suffix, int decimals);
    void configureFader (juce::Slider&, juce::Label&, const juce::String& name, const juce::String& suffix, int decimals, juce::Colour accent);
    void beginSkinChange (Skin newSkin);
    void applySkin (Skin newSkin);
    void layoutClassic (juce::Rectangle<int> content);
    void layoutNeon (juce::Rectangle<int> content);
    void layoutModular (juce::Rectangle<int> content);
    void layoutAnalysis (juce::Rectangle<int> content);
    void layoutRadar (juce::Rectangle<int> content);
    void layoutBroadcast (juce::Rectangle<int> content);
    void layoutControlGrid (juce::Rectangle<int> area, int columns);
    void setUiAlpha (float alpha);
    void drawSkinBackground (juce::Graphics& g, juce::Rectangle<float> area);
    void drawSkinCandy (juce::Graphics& g, juce::Rectangle<float> area);

    SantosLevelerAudioProcessor& processor;
    SantosLookAndFeel lookAndFeel;

    juce::Slider targetKnob, gateKnob, speedKnob, detectKnob, lookaheadKnob, holdKnob, releaseKnob, peakThresholdKnob;
    juce::Slider rangeDownSlider, rangeUpSlider, outputSlider;

    juce::Label targetLabel, gateLabel, speedLabel, detectLabel, lookaheadLabel, holdLabel, releaseLabel, peakThresholdLabel;
    juce::Label rangeDownLabel, rangeUpLabel, outputLabel;

    juce::Label titleLabel, subtitleLabel, historyLabel;
    juce::Label inputLegendLabel, riderLegendLabel, outputLegendLabel, peakLegendLabel;
    juce::Label skinLabel, motionLabel;
    juce::ComboBox skinSelector, motionSelector;

    HistoryComponent history;
    MeterComponent inputMeter;
    MeterComponent outputMeter;

    juce::AudioProcessorValueTreeState::SliderAttachment targetAttachment;
    juce::AudioProcessorValueTreeState::SliderAttachment gateAttachment;
    juce::AudioProcessorValueTreeState::SliderAttachment speedAttachment;
    juce::AudioProcessorValueTreeState::SliderAttachment detectAttachment;
    juce::AudioProcessorValueTreeState::SliderAttachment lookaheadAttachment;
    juce::AudioProcessorValueTreeState::SliderAttachment holdAttachment;
    juce::AudioProcessorValueTreeState::SliderAttachment releaseAttachment;
    juce::AudioProcessorValueTreeState::SliderAttachment peakThresholdAttachment;
    juce::AudioProcessorValueTreeState::SliderAttachment rangeDownAttachment;
    juce::AudioProcessorValueTreeState::SliderAttachment rangeUpAttachment;
    juce::AudioProcessorValueTreeState::SliderAttachment outputAttachment;

    Skin currentSkin = Skin::classicPro;
    Skin pendingSkin = Skin::classicPro;
    Motion motion = Motion::full;
    bool transitioning = false;
    bool pendingApplied = false;
    float transitionProgress = 1.0f;
    float animationPhase = 0.0f;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SantosLevelerAudioProcessorEditor)
};
