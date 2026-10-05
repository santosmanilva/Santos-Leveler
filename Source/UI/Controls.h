#pragma once

#include "Theme.h"

namespace SantosUI
{
// A rotary parameter: engraved caption on top, LED-ring knob, boxed read-out underneath.
class KnobUnit final : public juce::Component
{
public:
    static constexpr int captionHeight = 16, valueHeight = 20, width = 82;

    KnobUnit (juce::AudioProcessorValueTreeState& state, const juce::String& parameterId,
              const juce::String& caption, const juce::String& suffix, int decimals,
              juce::Colour accent = colour::accent)
        : name (caption), attachment (state, parameterId, slider)
    {
        slider.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
        slider.setTextBoxStyle (juce::Slider::TextBoxBelow, false, 72, valueHeight);
        slider.setTextValueSuffix (suffix);
        slider.setNumDecimalPlacesToDisplay (decimals);
        slider.setColour (juce::Slider::rotarySliderFillColourId, accent);
        slider.setColour (juce::Slider::textBoxTextColourId, accent);
        slider.setRotaryParameters (juce::degreesToRadians (225.0f), juce::degreesToRadians (495.0f), true);

        if (auto* parameter = state.getParameter (parameterId))
            slider.setDoubleClickReturnValue (true, parameter->convertFrom0to1 (parameter->getDefaultValue()));

        addAndMakeVisible (slider);
    }

    void paint (juce::Graphics& g) override
    {
        drawCaption (g, name, getLocalBounds().removeFromTop (captionHeight).toFloat(),
                     juce::Justification::centred, colour::ink, 9.5f);
    }

    void resized() override
    {
        auto area = getLocalBounds();
        area.removeFromTop (captionHeight);
        slider.setBounds (area);
    }

private:
    juce::String name;
    juce::Slider slider;
    juce::AudioProcessorValueTreeState::SliderAttachment attachment;
};

// A horizontal parameter: caption and read-out above a metal-thumb slider.
class SliderUnit final : public juce::Component
{
public:
    SliderUnit (juce::AudioProcessorValueTreeState& state, const juce::String& parameterId,
                const juce::String& caption, const juce::String& suffix, int decimals,
                int valueWidth = 62)
        : name (caption), attachment (state, parameterId, slider)
    {
        slider.setSliderStyle (juce::Slider::LinearHorizontal);
        slider.setTextBoxStyle (juce::Slider::TextBoxAbove, false, valueWidth, 18);
        slider.setTextValueSuffix (suffix);
        slider.setNumDecimalPlacesToDisplay (decimals);
        slider.setColour (juce::Slider::rotarySliderFillColourId, colour::accent);
        slider.setColour (juce::Slider::textBoxTextColourId, colour::ink);

        if (auto* parameter = state.getParameter (parameterId))
            slider.setDoubleClickReturnValue (true, parameter->convertFrom0to1 (parameter->getDefaultValue()));

        addAndMakeVisible (slider);
    }

    void paint (juce::Graphics& g) override
    {
        drawCaption (g, name, getLocalBounds().removeFromTop (18).toFloat(), juce::Justification::centredLeft,
                     colour::dim, 8.5f);
    }

    void resized() override { slider.setBounds (getLocalBounds()); }

private:
    juce::String name;
    juce::Slider slider;
    juce::AudioProcessorValueTreeState::SliderAttachment attachment;
};
} // namespace SantosUI
