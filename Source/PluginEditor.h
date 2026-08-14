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
        explicit HistoryComponent (SantosLevelerAudioProcessor& p);
        void paint (juce::Graphics&) override;
        void setViewMode (int newMode);

    private:
        void drawClassic (juce::Graphics&, juce::Rectangle<float>, const std::vector<SantosHistoryPoint>&);
        void drawArea (juce::Graphics&, juce::Rectangle<float>, const std::vector<SantosHistoryPoint>&);
        void drawLayers (juce::Graphics&, juce::Rectangle<float>, const std::vector<SantosHistoryPoint>&);
        void drawSpectrum (juce::Graphics&, juce::Rectangle<float>);
        void drawRadar (juce::Graphics&, juce::Rectangle<float>, const std::vector<SantosHistoryPoint>&);
        void drawBroadcast (juce::Graphics&, juce::Rectangle<float>);

        SantosLevelerAudioProcessor& processor;
        int viewMode = 0;
        juce::dsp::FFT fft { 11 };
        juce::dsp::WindowingFunction<float> window { SantosLevelerAudioProcessor::spectrumSize,
                                                      juce::dsp::WindowingFunction<float>::hann };
        std::array<float, SantosLevelerAudioProcessor::spectrumSize> spectrumSamples {};
        std::array<float, SantosLevelerAudioProcessor::spectrumSize * 2> fftData {};
    };

    class MeterComponent final : public juce::Component
    {
    public:
        enum class Source { input, output };
        MeterComponent (SantosLevelerAudioProcessor& p, Source s, juce::String text, juce::Colour c)
            : processor (p), source (s), label (std::move (text)), colour (c) {}
        void paint (juce::Graphics&) override;

    private:
        SantosLevelerAudioProcessor& processor;
        Source source;
        juce::String label;
        juce::Colour colour;
    };

    void timerCallback() override;
    void configureKnob (juce::Slider&, juce::Label&, const juce::String& name, const juce::String& suffix, int decimals);
    void configureFader (juce::Slider&, juce::Label&, const juce::String& name, const juce::String& suffix, int decimals, juce::Colour accent);

    SantosLevelerAudioProcessor& processor;
    SantosLookAndFeel lookAndFeel;

    juce::Slider targetKnob, gateKnob, speedKnob, detectKnob, lookaheadKnob, holdKnob, releaseKnob, peakThresholdKnob;
    juce::Slider rangeDownSlider, rangeUpSlider, outputSlider;

    juce::Label targetLabel, gateLabel, speedLabel, detectLabel, lookaheadLabel, holdLabel, releaseLabel, peakThresholdLabel;
    juce::Label rangeDownLabel, rangeUpLabel, outputLabel;

    juce::Label titleLabel, subtitleLabel, historyLabel, viewLabel;
    juce::Label inputLegendLabel, riderLegendLabel, peakLegendLabel, outputLegendLabel;
    juce::ComboBox viewSelector;

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
    juce::AudioProcessorValueTreeState::ComboBoxAttachment viewAttachment;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SantosLevelerAudioProcessorEditor)
};
