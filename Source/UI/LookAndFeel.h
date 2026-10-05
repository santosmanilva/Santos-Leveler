#pragma once

#include "Theme.h"

namespace SantosUI
{
class SantosLookAndFeel final : public juce::LookAndFeel_V4
{
public:
    SantosLookAndFeel()
    {
        setDefaultSansSerifTypefaceName ("Segoe UI");   // other platforms fall back to their UI font

        setColour (juce::Slider::textBoxTextColourId, colour::text);
        setColour (juce::Slider::textBoxBackgroundColourId, juce::Colours::transparentBlack);
        setColour (juce::Slider::textBoxOutlineColourId, juce::Colours::transparentBlack);
        setColour (juce::Slider::textBoxHighlightColourId, colour::accent.withAlpha (0.35f));
        setColour (juce::Slider::rotarySliderFillColourId, colour::accent);
        setColour (juce::Slider::thumbColourId, colour::text);
        setColour (juce::TextButton::buttonColourId, colour::cardHigh);
        setColour (juce::TextButton::buttonOnColourId, colour::accent);

        setColour (juce::TextEditor::backgroundColourId, colour::window);
        setColour (juce::TextEditor::textColourId, colour::text);
        setColour (juce::TextEditor::outlineColourId, colour::edge);
        setColour (juce::TextEditor::focusedOutlineColourId, colour::accent);
        setColour (juce::CaretComponent::caretColourId, colour::accent);

        setColour (juce::PopupMenu::backgroundColourId, colour::cardHigh);
        setColour (juce::PopupMenu::textColourId, colour::text);
        setColour (juce::PopupMenu::headerTextColourId, colour::textDim);
        setColour (juce::PopupMenu::highlightedBackgroundColourId, colour::accent.withAlpha (0.22f));
        setColour (juce::PopupMenu::highlightedTextColourId, colour::text);

        setColour (juce::AlertWindow::backgroundColourId, colour::card);
        setColour (juce::AlertWindow::textColourId, colour::text);
        setColour (juce::AlertWindow::outlineColourId, colour::edge);
        setColour (juce::TooltipWindow::backgroundColourId, colour::cardHigh);
        setColour (juce::TooltipWindow::textColourId, colour::text);
        setColour (juce::TooltipWindow::outlineColourId, colour::edge);
    }

    // ---- sliders -------------------------------------------------------------------

    juce::Label* createSliderTextBox (juce::Slider& slider) override
    {
        auto* label = LookAndFeel_V4::createSliderTextBox (slider);
        label->setColour (juce::Label::textColourId, colour::text);
        label->setColour (juce::Label::backgroundColourId, juce::Colours::transparentBlack);
        label->setColour (juce::Label::outlineColourId, juce::Colours::transparentBlack);
        label->setColour (juce::Label::backgroundWhenEditingColourId, colour::window);
        label->setColour (juce::Label::outlineWhenEditingColourId, colour::accent);
        label->setColour (juce::Label::textWhenEditingColourId, colour::text);
        label->setFont (font (13.0f));
        label->setBorderSize ({ 0, 2, 0, 2 });
        label->setJustificationType (slider.isRotary() ? juce::Justification::centred
                                                       : juce::Justification::centredRight);
        return label;
    }

    juce::Slider::SliderLayout getSliderLayout (juce::Slider& slider) override
    {
        juce::Slider::SliderLayout layout;
        auto bounds = slider.getLocalBounds();
        const auto hasText = slider.getTextBoxPosition() != juce::Slider::NoTextBox;

        if (slider.isRotary())
        {
            if (hasText)
                layout.textBoxBounds = bounds.removeFromBottom (slider.getTextBoxHeight())
                                             .withSizeKeepingCentre (slider.getTextBoxWidth(), slider.getTextBoxHeight());
            layout.sliderBounds = bounds;
        }
        else
        {
            if (hasText)
            {
                auto top = bounds.removeFromTop (18);
                layout.textBoxBounds = top.removeFromRight (slider.getTextBoxWidth());
            }
            layout.sliderBounds = bounds.reduced (7, 0);
        }

        return layout;
    }

    int getSliderThumbRadius (juce::Slider&) override { return 7; }

    void drawRotarySlider (juce::Graphics& g, int x, int y, int width, int height, float sliderPos,
                           float startAngle, float endAngle, juce::Slider& slider) override
    {
        const auto area = juce::Rectangle<float> (static_cast<float> (x), static_cast<float> (y),
                                                  static_cast<float> (width), static_cast<float> (height));
        const auto diameter = juce::jmin (area.getWidth(), area.getHeight());
        const auto circle = area.withSizeKeepingCentre (diameter, diameter).reduced (3.0f);
        const auto centre = circle.getCentre();
        const auto radius = circle.getWidth() * 0.5f;
        const auto accent = slider.findColour (juce::Slider::rotarySliderFillColourId);
        const auto hot = slider.isMouseOverOrDragging();

        const auto trackWidth = juce::jmax (3.0f, radius * 0.13f);
        const auto arcRadius = radius - trackWidth * 0.5f;
        const auto angle = startAngle + sliderPos * (endAngle - startAngle);

        // body
        const auto body = circle.reduced (trackWidth + 4.0f);
        g.setColour (colour::cardHigh);
        g.fillEllipse (body);
        g.setColour (hot ? colour::textFaint : colour::edge);
        g.drawEllipse (body, 1.0f);

        // track + value arc
        juce::Path track;
        track.addCentredArc (centre.x, centre.y, arcRadius, arcRadius, 0.0f, startAngle, endAngle, true);
        g.setColour (colour::track);
        g.strokePath (track, juce::PathStrokeType (trackWidth, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

        if (sliderPos > 0.001f)
        {
            juce::Path value;
            value.addCentredArc (centre.x, centre.y, arcRadius, arcRadius, 0.0f, startAngle, angle, true);
            g.setColour (accent);
            g.strokePath (value, juce::PathStrokeType (trackWidth, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
        }

        // pointer
        const auto direction = juce::Point<float> (std::sin (angle), -std::cos (angle));
        const auto inner = body.getWidth() * 0.5f * 0.30f;
        const auto outer = body.getWidth() * 0.5f * 0.86f;
        g.setColour (colour::text);
        g.drawLine ({ centre + direction * inner, centre + direction * outer }, 2.2f);
    }

    void drawLinearSlider (juce::Graphics& g, int x, int y, int width, int height, float sliderPos,
                           float minSliderPos, float maxSliderPos, juce::Slider::SliderStyle style,
                           juce::Slider& slider) override
    {
        if (style != juce::Slider::LinearHorizontal)
        {
            LookAndFeel_V4::drawLinearSlider (g, x, y, width, height, sliderPos, minSliderPos, maxSliderPos, style, slider);
            return;
        }

        const auto area = juce::Rectangle<float> (static_cast<float> (x), static_cast<float> (y),
                                                  static_cast<float> (width), static_cast<float> (height));
        const auto cy = area.getCentreY();
        constexpr float trackHeight = 4.0f;
        const auto trackArea = juce::Rectangle<float> (minSliderPos, cy - trackHeight * 0.5f, maxSliderPos - minSliderPos, trackHeight);

        g.setColour (colour::track);
        g.fillRoundedRectangle (trackArea, trackHeight * 0.5f);

        // Fill from the natural origin: zero for bipolar ranges, the 0 dB end for negative-only ranges.
        auto origin = trackArea.getX();
        if (slider.getMinimum() < 0.0 && slider.getMaximum() > 0.0)
            origin = minSliderPos + static_cast<float> (slider.valueToProportionOfLength (0.0)) * (maxSliderPos - minSliderPos);
        else if (slider.getMaximum() <= 0.0)
            origin = trackArea.getRight();

        const auto from = juce::jmin (origin, sliderPos);
        const auto to = juce::jmax (origin, sliderPos);
        g.setColour (slider.findColour (juce::Slider::rotarySliderFillColourId));
        g.fillRoundedRectangle ({ from, trackArea.getY(), to - from, trackHeight }, trackHeight * 0.5f);

        const auto thumb = juce::Rectangle<float> (13.0f, 13.0f).withCentre ({ sliderPos, cy });
        g.setColour (colour::card);
        g.fillEllipse (thumb.expanded (2.0f));
        g.setColour (slider.isMouseOverOrDragging() ? juce::Colours::white : colour::text);
        g.fillEllipse (thumb);
    }

    // ---- buttons -------------------------------------------------------------------

    void drawButtonBackground (juce::Graphics& g, juce::Button& button, const juce::Colour&,
                               bool highlighted, bool down) override
    {
        auto bounds = button.getLocalBounds().toFloat().reduced (0.5f);
        const auto on = button.getToggleState();
        const auto onColour = button.findColour (juce::TextButton::buttonOnColourId);
        const auto radius = 6.0f;

        if (on)
        {
            g.setColour (onColour.withMultipliedBrightness (down ? 0.9f : (highlighted ? 1.08f : 1.0f)));
            g.fillRoundedRectangle (bounds, radius);
        }
        else
        {
            g.setColour (down ? colour::track : (highlighted ? colour::cardHigh.brighter (0.12f) : colour::cardHigh));
            g.fillRoundedRectangle (bounds, radius);
            g.setColour (colour::edge);
            g.drawRoundedRectangle (bounds, radius, 1.0f);
        }
    }

    juce::Font getTextButtonFont (juce::TextButton&, int) override
    {
        return font (11.5f, true).withExtraKerningFactor (0.08f);
    }

    void drawButtonText (juce::Graphics& g, juce::TextButton& button, bool highlighted, bool) override
    {
        const auto on = button.getToggleState();
        g.setFont (getTextButtonFont (button, button.getHeight()));
        g.setColour (on ? colour::window : (highlighted ? colour::text : colour::textDim));
        g.drawText (button.getButtonText().toUpperCase(), button.getLocalBounds(), juce::Justification::centred, false);
    }

    void drawToggleButton (juce::Graphics& g, juce::ToggleButton& button, bool highlighted, bool) override
    {
        // Pill switch, drawn centred in the button bounds.
        const auto pill = juce::Rectangle<float> (36.0f, 20.0f).withCentre (button.getLocalBounds().toFloat().getCentre());
        const auto on = button.getToggleState();
        g.setColour (on ? colour::accent : colour::track);
        g.fillRoundedRectangle (pill, pill.getHeight() * 0.5f);
        const auto knob = juce::Rectangle<float> (14.0f, 14.0f)
                              .withCentre ({ on ? pill.getRight() - 10.0f : pill.getX() + 10.0f, pill.getCentreY() });
        g.setColour (on ? colour::window : (highlighted ? colour::text : colour::textDim));
        g.fillEllipse (knob);
    }

    void drawCornerResizer (juce::Graphics& g, int w, int h, bool isMouseOver, bool isMouseDragging) override
    {
        g.setColour (isMouseOver || isMouseDragging ? colour::textDim : colour::textFaint.withAlpha (0.7f));
        const auto fw = static_cast<float> (w), fh = static_cast<float> (h);
        for (int i = 1; i <= 3; ++i)
        {
            const auto offset = static_cast<float> (i) * 4.5f;
            g.drawLine (fw - offset, fh - 1.5f, fw - 1.5f, fh - offset, 1.2f);
        }
    }

    // ---- popup menu ----------------------------------------------------------------

    juce::Font getPopupMenuFont() override { return font (13.0f); }
};
} // namespace SantosUI
