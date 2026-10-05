#pragma once

#include "Theme.h"

namespace SantosUI
{
// Component IDs that tell the look-and-feel which kind of slider read-out it is drawing.
inline const juce::String knobValueId = "knobValue";
inline const juce::String sliderValueId = "sliderValue";

class SantosLookAndFeel final : public juce::LookAndFeel_V4
{
public:
    SantosLookAndFeel()
    {
        setDefaultSansSerifTypefaceName ("Segoe UI");   // other platforms fall back to their UI font

        setColour (juce::Slider::textBoxTextColourId, colour::accent);
        setColour (juce::Slider::textBoxBackgroundColourId, juce::Colours::transparentBlack);
        setColour (juce::Slider::textBoxOutlineColourId, juce::Colours::transparentBlack);
        setColour (juce::Slider::textBoxHighlightColourId, colour::accent.withAlpha (0.35f));
        setColour (juce::Slider::rotarySliderFillColourId, colour::accent);
        setColour (juce::TextButton::buttonColourId, colour::plateBot);
        setColour (juce::TextButton::buttonOnColourId, colour::accent);

        setColour (juce::TextEditor::backgroundColourId, colour::well);
        setColour (juce::TextEditor::textColourId, colour::ink);
        setColour (juce::TextEditor::outlineColourId, colour::wellEdge);
        setColour (juce::TextEditor::focusedOutlineColourId, colour::accent);
        setColour (juce::CaretComponent::caretColourId, colour::accent);

        setColour (juce::PopupMenu::backgroundColourId, juce::Colour (0xff1b1b1e));
        setColour (juce::PopupMenu::textColourId, colour::ink);
        setColour (juce::PopupMenu::headerTextColourId, colour::dim);
        setColour (juce::PopupMenu::highlightedBackgroundColourId, colour::accent.withAlpha (0.28f));
        setColour (juce::PopupMenu::highlightedTextColourId, colour::ink);

        setColour (juce::AlertWindow::backgroundColourId, juce::Colour (0xff1b1b1e));
        setColour (juce::AlertWindow::textColourId, colour::ink);
        setColour (juce::AlertWindow::outlineColourId, colour::wellEdge);
        setColour (juce::TooltipWindow::backgroundColourId, juce::Colour (0xff1b1b1e));
        setColour (juce::TooltipWindow::textColourId, colour::ink);
        setColour (juce::TooltipWindow::outlineColourId, colour::wellEdge);
    }

    // ---- slider read-outs ---------------------------------------------------------------

    juce::Label* createSliderTextBox (juce::Slider& slider) override
    {
        auto* label = LookAndFeel_V4::createSliderTextBox (slider);
        const auto rotary = slider.isRotary();
        label->setComponentID (rotary ? knobValueId : sliderValueId);
        label->setColour (juce::Label::textColourId, slider.findColour (juce::Slider::textBoxTextColourId));
        label->setColour (juce::Label::backgroundColourId, juce::Colours::transparentBlack);
        label->setColour (juce::Label::outlineColourId, juce::Colours::transparentBlack);
        label->setColour (juce::Label::backgroundWhenEditingColourId, colour::well);
        label->setColour (juce::Label::outlineWhenEditingColourId, colour::accent);
        label->setColour (juce::Label::textWhenEditingColourId, colour::ink);
        label->setFont (font (rotary ? 11.5f : 10.5f, true));
        label->setBorderSize ({ 0, 2, 0, 2 });
        label->setJustificationType (rotary ? juce::Justification::centred : juce::Justification::centredRight);
        return label;
    }

    void drawLabel (juce::Graphics& g, juce::Label& label) override
    {
        if (label.getComponentID() == knobValueId && ! label.isBeingEdited())
        {
            const auto bounds = label.getLocalBounds().toFloat();
            g.setColour (colour::well);
            g.fillRoundedRectangle (bounds, 4.0f);
            g.setColour (colour::wellEdge);
            g.drawRoundedRectangle (bounds.reduced (0.5f), 4.0f, 1.0f);
            g.setColour (label.findColour (juce::Label::textColourId));
            g.setFont (label.getFont());
            g.drawText (label.getText(), label.getLocalBounds(), juce::Justification::centred, false);
            return;
        }

        LookAndFeel_V4::drawLabel (g, label);
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

    // ---- LED-ring knob ------------------------------------------------------------------

    void drawRotarySlider (juce::Graphics& g, int x, int y, int width, int height, float sliderPos,
                           float startAngle, float endAngle, juce::Slider& slider) override
    {
        const auto area = juce::Rectangle<float> (static_cast<float> (x), static_cast<float> (y),
                                                  static_cast<float> (width), static_cast<float> (height));
        const auto centre = area.getCentre();
        const auto diameter = juce::jmin (area.getWidth(), area.getHeight());
        constexpr float ringGap = 6.0f, ringLength = 8.0f;
        const auto radius = diameter * 0.5f - 2.0f - ringGap - ringLength;
        const auto accent = slider.findColour (juce::Slider::rotarySliderFillColourId);
        constexpr int segments = 21;

        // LED segments, with a cheap glow made of wider translucent strokes.
        for (int pass = 0; pass < 3; ++pass)
        {
            for (int i = 0; i < segments; ++i)
            {
                const auto f = static_cast<float> (i) / static_cast<float> (segments - 1);
                const auto on = f <= sliderPos + 0.001f;
                if (pass < 2 && ! on)
                    continue;

                const auto angle = startAngle + f * (endAngle - startAngle);
                const juce::Point<float> direction (std::sin (angle), -std::cos (angle));
                const auto from = centre + direction * (radius + ringGap);
                const auto to = centre + direction * (radius + ringGap + ringLength);
                g.setColour (on ? accent.withAlpha (pass == 0 ? 0.16f : (pass == 1 ? 0.28f : 1.0f)) : juce::Colour (0xff3b3b3f));
                const auto thickness = pass == 0 ? 7.5f : (pass == 1 ? 5.0f : 2.8f);
                g.drawLine ({ from, to }, thickness);
            }
        }

        // body
        juce::Path body;
        body.addEllipse (juce::Rectangle<float> (radius * 2.0f + 6.0f, radius * 2.0f + 6.0f).withCentre (centre));
        juce::DropShadow (juce::Colours::black.withAlpha (0.65f), 6, { 0, 3 }).drawForPath (g, body);

        const auto outer = juce::Rectangle<float> (radius * 2.0f, radius * 2.0f).withCentre (centre);
        g.setColour (juce::Colour (0xff0b0b0c));
        g.fillEllipse (outer.expanded (3.0f));
        g.setGradientFill (juce::ColourGradient (juce::Colour (0xff8d8d92), outer.getX(), outer.getY(),
                                                 juce::Colour (0xff050506), outer.getRight(), outer.getBottom(), false));
        g.fillEllipse (outer);

        const auto face = outer.reduced (4.0f);
        juce::ColourGradient faceGradient (juce::Colour (0xff6b6b70), face.getX() + face.getWidth() * 0.36f, face.getY() + face.getHeight() * 0.28f,
                                           juce::Colour (0xff101012), face.getRight(), face.getBottom(), true);
        faceGradient.addColour (0.45, juce::Colour (0xff2c2c2f));
        g.setGradientFill (faceGradient);
        g.fillEllipse (face);
        g.setColour (juce::Colours::white.withAlpha (0.08f));
        g.drawEllipse (face.reduced (5.0f), 0.8f);

        // pointer
        const auto angle = startAngle + sliderPos * (endAngle - startAngle);
        const juce::Point<float> direction (std::sin (angle), -std::cos (angle));
        g.setColour (colour::ink);
        g.drawLine ({ centre + direction * (radius - 12.0f), centre + direction * (radius - 5.0f) }, 2.6f);
    }

    // ---- horizontal slider --------------------------------------------------------------

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
        const auto accent = slider.findColour (juce::Slider::rotarySliderFillColourId);
        const auto slot = juce::Rectangle<float> (minSliderPos, cy - 3.5f, maxSliderPos - minSliderPos, 7.0f);

        g.setColour (juce::Colour (0xff030304));
        g.fillRoundedRectangle (slot, 3.5f);
        g.setColour (juce::Colour (0xff35353a));
        g.drawRoundedRectangle (slot.reduced (0.5f), 3.5f, 1.0f);

        // Fill from the natural origin: zero for bipolar ranges, the 0 dB end for negative-only ranges.
        auto origin = slot.getX();
        if (slider.getMinimum() < 0.0 && slider.getMaximum() > 0.0)
            origin = minSliderPos + static_cast<float> (slider.valueToProportionOfLength (0.0)) * (maxSliderPos - minSliderPos);
        else if (slider.getMaximum() <= 0.0)
            origin = slot.getRight();

        const auto from = juce::jmin (origin, sliderPos), to = juce::jmax (origin, sliderPos);
        g.setColour (accent);
        g.fillRoundedRectangle ({ from, cy - 2.0f, to - from, 4.0f }, 2.0f);

        const auto thumb = juce::Rectangle<float> (12.0f, 19.0f).withCentre ({ sliderPos, cy });
        juce::Path thumbPath;
        thumbPath.addRoundedRectangle (thumb, 2.5f);
        juce::DropShadow (juce::Colours::black.withAlpha (0.65f), 4, { 0, 2 }).drawForPath (g, thumbPath);
        g.setGradientFill (juce::ColourGradient (juce::Colour (0xff9a9a9f), 0.0f, thumb.getY(), juce::Colour (0xff232326), 0.0f, thumb.getBottom(), false));
        g.fillRoundedRectangle (thumb, 2.5f);
        g.setColour (juce::Colour (0xff0a0a0b));
        g.drawRoundedRectangle (thumb, 2.5f, 1.0f);
        g.setColour (slider.isMouseOverOrDragging() ? juce::Colours::white : colour::ink);
        g.drawLine (sliderPos, thumb.getY() + 3.0f, sliderPos, thumb.getBottom() - 3.0f, 1.4f);
    }

    // ---- pads ---------------------------------------------------------------------------

    void drawButtonBackground (juce::Graphics& g, juce::Button& button, const juce::Colour&,
                               bool highlighted, bool down) override
    {
        const auto bounds = button.getLocalBounds().toFloat().reduced (4.0f);
        const auto on = button.getToggleState();
        const auto glow = button.findColour (juce::TextButton::buttonOnColourId);

        if (on)
            for (int i = 3; i >= 1; --i)
            {
                g.setColour (glow.withAlpha (0.10f));
                g.fillRoundedRectangle (bounds.expanded (static_cast<float> (i) * 1.6f), 10.0f);
            }

        g.setColour (juce::Colour (0xff0b0b0c));
        g.fillRoundedRectangle (bounds.expanded (3.0f), 12.0f);

        juce::Path pad;
        pad.addRoundedRectangle (bounds, 9.0f);
        juce::DropShadow (juce::Colours::black.withAlpha (0.65f), 5, { 0, 2 }).drawForPath (g, pad);

        if (on)
        {
            juce::ColourGradient lit (glow.withAlpha (0.95f), bounds.getCentreX(), bounds.getCentreY(),
                                      glow.darker (0.8f).withAlpha (0.55f), bounds.getX(), bounds.getY(), true);
            g.setGradientFill (lit);
            g.fillRoundedRectangle (bounds, 9.0f);
            g.setColour (juce::Colours::white.withAlpha (0.18f));
            g.drawRoundedRectangle (bounds.reduced (3.0f), 6.0f, 1.0f);
        }
        else
        {
            const auto base = down ? juce::Colour (0xff3b3840) : (highlighted ? juce::Colour (0xff57535d) : juce::Colour (0xff4b4750));
            g.setColour (base);
            g.fillRoundedRectangle (bounds, 9.0f);
            g.setColour (juce::Colour (0xff6a6670));
            g.drawRoundedRectangle (bounds.reduced (0.5f), 9.0f, 1.0f);
            g.setColour (juce::Colours::white.withAlpha (0.05f));
            g.fillRoundedRectangle (bounds.reduced (3.0f).withHeight ((bounds.getHeight() - 6.0f) * 0.5f), 7.0f);
        }
    }

    juce::Font getTextButtonFont (juce::TextButton&, int) override
    {
        return font (10.5f, true).withExtraKerningFactor (0.12f);
    }

    void drawButtonText (juce::Graphics& g, juce::TextButton& button, bool, bool) override
    {
        const auto on = button.getToggleState();
        g.setFont (getTextButtonFont (button, button.getHeight()));
        g.setColour (on ? juce::Colours::white : juce::Colour (0xffc6c6cb));
        g.drawText (button.getButtonText().toUpperCase(), button.getLocalBounds(), juce::Justification::centred, false);
    }

    // The compressor switch: "ON" and a lit LED.
    void drawToggleButton (juce::Graphics& g, juce::ToggleButton& button, bool highlighted, bool) override
    {
        const auto on = button.getToggleState();
        const auto bounds = button.getLocalBounds().toFloat();
        drawLed (g, { bounds.getRight() - 12.0f, bounds.getCentreY() }, 6.0f, on, colour::accent);
        drawCaption (g, "On", bounds.withTrimmedRight (26.0f), juce::Justification::centredRight,
                     on ? colour::accent : (highlighted ? colour::ink : colour::dim), 9.0f);
    }

    juce::Font getPopupMenuFont() override { return font (13.0f); }

    void drawCornerResizer (juce::Graphics& g, int w, int h, bool isMouseOver, bool isMouseDragging) override
    {
        g.setColour (isMouseOver || isMouseDragging ? colour::dim : colour::faint.withAlpha (0.7f));
        const auto fw = static_cast<float> (w), fh = static_cast<float> (h);
        for (int i = 1; i <= 3; ++i)
        {
            const auto offset = static_cast<float> (i) * 4.5f;
            g.drawLine (fw - offset, fh - 1.5f, fw - 1.5f, fh - offset, 1.2f);
        }
    }
};
} // namespace SantosUI
