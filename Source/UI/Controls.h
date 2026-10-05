#pragma once

#include "Theme.h"

namespace SantosUI
{
// A rotary parameter control: caption on top, knob, editable value underneath.
class KnobUnit final : public juce::Component
{
public:
    KnobUnit (juce::AudioProcessorValueTreeState& state, const juce::String& parameterId,
              const juce::String& caption, const juce::String& suffix, int decimals,
              juce::Colour accent = colour::accent)
        : name (caption), attachment (state, parameterId, slider)
    {
        slider.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
        slider.setTextBoxStyle (juce::Slider::TextBoxBelow, false, 82, 20);
        slider.setTextValueSuffix (suffix);
        slider.setNumDecimalPlacesToDisplay (decimals);
        slider.setColour (juce::Slider::rotarySliderFillColourId, accent);
        slider.setRotaryParameters (juce::degreesToRadians (225.0f), juce::degreesToRadians (495.0f), true);

        if (auto* parameter = state.getParameter (parameterId))
            slider.setDoubleClickReturnValue (true, parameter->convertFrom0to1 (parameter->getDefaultValue()));

        addAndMakeVisible (slider);
    }

    void paint (juce::Graphics& g) override
    {
        drawCaption (g, name, getLocalBounds().removeFromTop (16).toFloat(), juce::Justification::centred);
    }

    void resized() override
    {
        auto area = getLocalBounds();
        area.removeFromTop (16);
        slider.setBounds (area);
    }

private:
    juce::String name;
    juce::Slider slider;
    juce::AudioProcessorValueTreeState::SliderAttachment attachment;
};

// A horizontal parameter control: caption and editable value above a thin track.
class SliderUnit final : public juce::Component
{
public:
    SliderUnit (juce::AudioProcessorValueTreeState& state, const juce::String& parameterId,
                const juce::String& caption, const juce::String& suffix, int decimals,
                juce::Colour accent = colour::accent)
        : name (caption), attachment (state, parameterId, slider)
    {
        slider.setSliderStyle (juce::Slider::LinearHorizontal);
        slider.setTextBoxStyle (juce::Slider::TextBoxAbove, false, 62, 18);
        slider.setTextValueSuffix (suffix);
        slider.setNumDecimalPlacesToDisplay (decimals);
        slider.setColour (juce::Slider::rotarySliderFillColourId, accent);

        if (auto* parameter = state.getParameter (parameterId))
            slider.setDoubleClickReturnValue (true, parameter->convertFrom0to1 (parameter->getDefaultValue()));

        addAndMakeVisible (slider);
    }

    void paint (juce::Graphics& g) override
    {
        drawCaption (g, name, getLocalBounds().removeFromTop (18).toFloat());
    }

    void resized() override { slider.setBounds (getLocalBounds()); }

private:
    juce::String name;
    juce::Slider slider;
    juce::AudioProcessorValueTreeState::SliderAttachment attachment;
};
} // namespace SantosUI
