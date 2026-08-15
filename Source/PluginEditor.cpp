#include "PluginEditor.h"

namespace
{
const auto bg       = juce::Colour (0xff03070a);
const auto frame    = juce::Colour (0xff0c1217);
const auto panel    = juce::Colour (0xff10171d);
const auto panel2   = juce::Colour (0xff070c10);
const auto border   = juce::Colour (0xff34424a);
const auto borderHi = juce::Colour (0xff74818a);
const auto text     = juce::Colour (0xffeef4f6);
const auto muted    = juce::Colour (0xff89969d);
const auto cyan     = juce::Colour (0xff19ccf4);
const auto yellow   = juce::Colour (0xffffd51f);
const auto green    = juce::Colour (0xff67e45f);
const auto lime     = juce::Colour (0xff83e861);
const auto magenta  = juce::Colour (0xffff58d9);
const auto amber    = juce::Colour (0xffffb33f);

void styleLabel (juce::Label& label, float size, juce::Colour colour,
                 int justification = juce::Justification::centred)
{
    label.setFont (juce::FontOptions (size));
    label.setColour (juce::Label::textColourId, colour);
    label.setJustificationType (justification);
    label.setInterceptsMouseClicks (false, false);
}

void drawPanel (juce::Graphics& g, juce::Rectangle<float> r, float radius = 7.0f)
{
    g.setColour (juce::Colours::black.withAlpha (0.68f));
    g.fillRoundedRectangle (r.translated (0.0f, 2.0f), radius);

    juce::ColourGradient grad (juce::Colour (0xff151e24), r.getTopLeft(),
                               juce::Colour (0xff070b0e), r.getBottomRight(), false);
    grad.addColour (0.48, juce::Colour (0xff0d1419));
    g.setGradientFill (grad);
    g.fillRoundedRectangle (r, radius);

    g.setColour (border.withAlpha (0.95f));
    g.drawRoundedRectangle (r.reduced (0.5f), radius, 1.0f);
    g.setColour (juce::Colours::white.withAlpha (0.055f));
    g.drawLine (r.getX() + radius, r.getY() + 1.0f,
                r.getRight() - radius, r.getY() + 1.0f, 1.0f);
}

float paramValue (SantosLevelerAudioProcessor& p, const char* id)
{
    if (auto* v = p.apvts.getRawParameterValue (id))
        return v->load();
    return 0.0f;
}

juce::Colour meterLedColour (juce::Colour base, float levelNorm)
{
    if (levelNorm >= 0.94f) return juce::Colour (0xffff5459);
    if (levelNorm >= 0.84f) return juce::Colour (0xffffce42);
    return base;
}

void drawHorizontalScale (juce::Graphics& g, juce::Rectangle<float> r,
                          const std::initializer_list<const char*>& labels)
{
    const auto count = (int) labels.size();
    if (count < 2) return;

    int i = 0;
    for (auto* label : labels)
    {
        const auto x = juce::jmap ((float) i / (float) (count - 1), r.getX(), r.getRight());
        g.setColour (muted.withAlpha (0.72f));
        g.drawLine (x, r.getY(), x, r.getY() + 4.0f, 0.8f);
        g.setFont (juce::FontOptions (7.6f));
        g.drawText (label, (int) x - 18, (int) r.getY() + 5, 36, 13, juce::Justification::centred);
        ++i;
    }
}
}

SantosLevelerAudioProcessorEditor::SantosLookAndFeel::SantosLookAndFeel()
{
    setColour (juce::Slider::textBoxTextColourId, text);
    setColour (juce::Slider::textBoxBackgroundColourId, juce::Colour (0xff090e12));
    setColour (juce::Slider::textBoxOutlineColourId, border.withAlpha (0.92f));
    setColour (juce::Slider::rotarySliderOutlineColourId, juce::Colour (0xff2a343a));
    setColour (juce::Slider::trackColourId, juce::Colour (0xff202a30));
}

void SantosLevelerAudioProcessorEditor::SantosLookAndFeel::drawRotarySlider (
    juce::Graphics& g, int x, int y, int width, int height,
    float sliderPos, float rotaryStartAngle, float rotaryEndAngle, juce::Slider& slider)
{
    auto b = juce::Rectangle<float> ((float) x, (float) y, (float) width, (float) height).reduced (5.0f);
    const auto radius = juce::jmax (11.0f, juce::jmin (b.getWidth(), b.getHeight()) * 0.39f);
    const auto c = b.getCentre();
    const auto accent = slider.findColour (juce::Slider::rotarySliderFillColourId);
    const auto angle = rotaryStartAngle + sliderPos * (rotaryEndAngle - rotaryStartAngle);

    // coloured aura
    g.setColour (accent.withAlpha (0.055f));
    g.fillEllipse (juce::Rectangle<float> (radius * 2.42f, radius * 2.42f).withCentre (c));

    // deep shadow
    g.setColour (juce::Colours::black.withAlpha (0.78f));
    g.fillEllipse (juce::Rectangle<float> (radius * 1.96f, radius * 1.96f).withCentre (c).translated (0.0f, 3.0f));

    // satin metal bezel
    auto bezel = juce::Rectangle<float> (radius * 1.82f, radius * 1.82f).withCentre (c);
    juce::ColourGradient bezelGrad (juce::Colour (0xff9aa3a8), bezel.getTopLeft(),
                                    juce::Colour (0xff0b0f11), bezel.getBottomRight(), false);
    bezelGrad.addColour (0.18, juce::Colour (0xff606b71));
    bezelGrad.addColour (0.42, juce::Colour (0xff2a3439));
    bezelGrad.addColour (0.67, juce::Colour (0xff12191d));
    bezelGrad.addColour (0.86, juce::Colour (0xff050709));
    g.setGradientFill (bezelGrad);
    g.fillEllipse (bezel);
    g.setColour (juce::Colours::black.withAlpha (0.85f));
    g.drawEllipse (bezel, 1.1f);
    g.setColour (juce::Colours::white.withAlpha (0.12f));
    g.drawEllipse (bezel.reduced (1.1f), 0.8f);

    // black inner recess
    auto recess = bezel.reduced (radius * 0.12f);
    g.setColour (juce::Colour (0xff06090b));
    g.fillEllipse (recess);

    // central cap with directional brushed-metal shading
    auto body = juce::Rectangle<float> (radius * 1.37f, radius * 1.37f).withCentre (c);
    juce::ColourGradient bodyGrad (juce::Colour (0xff667178), body.getTopLeft(),
                                   juce::Colour (0xff050708), body.getBottomRight(), false);
    bodyGrad.addColour (0.26, juce::Colour (0xff394349));
    bodyGrad.addColour (0.48, juce::Colour (0xff1c252a));
    bodyGrad.addColour (0.69, juce::Colour (0xff0a0e11));
    g.setGradientFill (bodyGrad);
    g.fillEllipse (body);
    g.setColour (juce::Colours::black.withAlpha (0.9f));
    g.drawEllipse (body, 1.0f);

    // specular highlight
    juce::ColourGradient spec (juce::Colours::white.withAlpha (0.25f), c.x - radius * 0.36f, c.y - radius * 0.46f,
                               juce::Colours::transparentWhite, c.x + radius * 0.26f, c.y + radius * 0.12f, false);
    g.setGradientFill (spec);
    g.fillEllipse (body.reduced (radius * 0.08f).withHeight (body.getHeight() * 0.44f));

    // ticks
    const auto tickInner = radius * 0.99f;
    const auto tickOuter = radius * 1.19f;
    constexpr int ticks = 27;
    for (int i = 0; i < ticks; ++i)
    {
        const auto frac = (float) i / (float) (ticks - 1);
        const auto a = rotaryStartAngle + (rotaryEndAngle - rotaryStartAngle) * frac;
        const auto p1 = c + juce::Point<float> (std::sin (a), -std::cos (a)) * tickInner;
        const auto p2 = c + juce::Point<float> (std::sin (a), -std::cos (a)) * tickOuter;
        const bool on = frac <= sliderPos + 0.001f;
        if (on)
        {
            g.setColour (accent.withAlpha (0.12f));
            g.drawLine ({ p1, p2 }, 4.6f);
        }
        g.setColour ((on ? accent : juce::Colour (0xff334047)).withAlpha (on ? 0.98f : 0.68f));
        g.drawLine ({ p1, p2 }, i % 4 == 0 ? 1.8f : 1.0f);
    }

    // luminous value arc
    juce::Path arc;
    arc.addCentredArc (c.x, c.y, radius * 0.90f, radius * 0.90f, 0.0f,
                       rotaryStartAngle, angle, true);
    g.setColour (accent.withAlpha (0.15f));
    g.strokePath (arc, juce::PathStrokeType (8.5f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
    g.setColour (accent);
    g.strokePath (arc, juce::PathStrokeType (2.1f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

    // pointer and hub
    juce::Path pointer;
    pointer.addRoundedRectangle (-1.45f, -radius * 0.61f, 2.9f, radius * 0.40f, 1.4f);
    pointer.applyTransform (juce::AffineTransform::rotation (angle).translated (c.x, c.y));
    g.setColour (juce::Colours::black.withAlpha (0.7f));
    g.fillPath (pointer, juce::AffineTransform::translation (1.0f, 1.0f));
    g.setColour (text);
    g.fillPath (pointer);
    g.setColour (accent.withAlpha (0.85f));
    g.fillEllipse (c.x - 2.1f, c.y - 2.1f, 4.2f, 4.2f);
}

void SantosLevelerAudioProcessorEditor::SantosLookAndFeel::drawLinearSlider (
    juce::Graphics& g, int x, int y, int width, int height,
    float sliderPos, float, float, juce::Slider::SliderStyle style, juce::Slider& slider)
{
    if (style != juce::Slider::LinearHorizontal)
    {
        juce::LookAndFeel_V4::drawLinearSlider (g, x, y, width, height, sliderPos, 0.0f, 0.0f, style, slider);
        return;
    }

    const auto accent = slider.findColour (juce::Slider::thumbColourId);
    auto area = juce::Rectangle<float> ((float) x, (float) y, (float) width, (float) height).reduced (12.0f, 10.0f);
    const auto cy = area.getCentreY();
    const auto left = area.getX();
    const auto right = area.getRight();

    // recessed rail
    g.setColour (juce::Colours::black.withAlpha (0.78f));
    g.fillRoundedRectangle (left, cy - 8.0f, right - left, 16.0f, 8.0f);
    g.setColour (juce::Colour (0xff3a474e));
    g.drawRoundedRectangle ({ left, cy - 8.0f, right - left, 16.0f }, 8.0f, 1.0f);
    g.setColour (juce::Colours::white.withAlpha (0.035f));
    g.drawLine (left + 7.0f, cy - 6.0f, right - 7.0f, cy - 6.0f, 1.0f);

    const auto min = slider.getMinimum();
    const auto max = slider.getMaximum();
    float zeroX = left;
    if (min < 0.0 && max > 0.0)
        zeroX = left + (float) ((0.0 - min) / (max - min)) * (right - left);

    const auto a = juce::jmin (sliderPos, zeroX);
    const auto b = juce::jmax (sliderPos, zeroX);
    g.setColour (accent.withAlpha (0.13f));
    g.fillRoundedRectangle (a, cy - 6.0f, juce::jmax (2.0f, b - a), 12.0f, 6.0f);
    g.setColour (accent.withAlpha (0.34f));
    g.fillRoundedRectangle (a, cy - 3.0f, juce::jmax (2.0f, b - a), 6.0f, 3.0f);
    g.setColour (accent);
    g.fillRoundedRectangle (a, cy - 1.6f, juce::jmax (2.0f, b - a), 3.2f, 1.6f);

    // metallic slider handle
    juce::ColourGradient thumbGrad (juce::Colour (0xffeef2f4), sliderPos - 8.0f, cy - 11.0f,
                                    juce::Colour (0xff1b2125), sliderPos + 8.0f, cy + 11.0f, false);
    thumbGrad.addColour (0.45, juce::Colour (0xffa2abb0));
    thumbGrad.addColour (0.66, juce::Colour (0xff535c61));
    g.setGradientFill (thumbGrad);
    g.fillRoundedRectangle (sliderPos - 8.0f, cy - 11.5f, 16.0f, 23.0f, 3.5f);
    g.setColour (juce::Colours::black.withAlpha (0.85f));
    g.drawRoundedRectangle ({ sliderPos - 8.0f, cy - 11.5f, 16.0f, 23.0f }, 3.5f, 1.0f);
    g.setColour (juce::Colours::white.withAlpha (0.48f));
    g.drawVerticalLine ((int) sliderPos, cy - 8.0f, cy + 8.0f);
}

SantosLevelerAudioProcessorEditor::SantosLevelerAudioProcessorEditor (SantosLevelerAudioProcessor& p)
    : AudioProcessorEditor (&p),
      processor (p),
      history (p),
      inputMeter (p, MeterComponent::Source::input, "INPUT", cyan),
      outputMeter (p, MeterComponent::Source::output, "OUTPUT", green),
      targetAttachment (p.apvts, "target", targetKnob),
      gateAttachment (p.apvts, "gate", gateKnob),
      speedAttachment (p.apvts, "speed", speedKnob),
      detectAttachment (p.apvts, "detect", detectKnob),
      lookaheadAttachment (p.apvts, "lookahead", lookaheadKnob),
      holdAttachment (p.apvts, "hold", holdKnob),
      releaseAttachment (p.apvts, "release", releaseKnob),
      peakThresholdAttachment (p.apvts, "peakThreshold", peakThresholdKnob),
      rangeDownAttachment (p.apvts, "rangeDown", rangeDownSlider),
      rangeUpAttachment (p.apvts, "rangeUp", rangeUpSlider),
      outputAttachment (p.apvts, "output", outputSlider)
{
    setLookAndFeel (&lookAndFeel);
    setSize (1200, 800);
    setResizable (true, true);
    setResizeLimits (900, 620, 1800, 1200);

    // Header text is drawn manually so SANTOS and LEVELER can use different colours.
    titleLabel.setText ({}, juce::dontSendNotification);
    subtitleLabel.setText ({}, juce::dontSendNotification);
    addAndMakeVisible (titleLabel);
    addAndMakeVisible (subtitleLabel);

    configureKnob (gateKnob, gateLabel, "GATE", " dB", 1, cyan);
    configureKnob (targetKnob, targetLabel, "TARGET", " dB", 1, lime);
    configureKnob (speedKnob, speedLabel, "SPEED", " ms", 0, cyan);
    configureKnob (detectKnob, detectLabel, "DETECT", " ms", 0, cyan);
    configureKnob (lookaheadKnob, lookaheadLabel, "LOOKAHEAD", " ms", 0, cyan);
    configureKnob (holdKnob, holdLabel, "HOLD", " ms", 0, cyan);
    configureKnob (releaseKnob, releaseLabel, "RELEASE", " ms", 0, cyan);
    configureKnob (peakThresholdKnob, peakThresholdLabel, "PEAK", " dBFS", 1, magenta);

    configureFader (rangeDownSlider, rangeDownLabel, "RANGE DOWN", " dB", 1, cyan);
    configureFader (rangeUpSlider, rangeUpLabel, "RANGE UP", " dB", 1, yellow);
    configureFader (outputSlider, outputLabel, "OUTPUT", " dB", 1, green);

    addAndMakeVisible (history);
    addAndMakeVisible (inputMeter);
    addAndMakeVisible (outputMeter);

    startTimerHz (30);
}

SantosLevelerAudioProcessorEditor::~SantosLevelerAudioProcessorEditor()
{
    setLookAndFeel (nullptr);
}

void SantosLevelerAudioProcessorEditor::configureKnob (juce::Slider& slider, juce::Label& label,
                                                        const juce::String& name, const juce::String& suffix,
                                                        int decimals, juce::Colour accent)
{
    slider.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
    slider.setTextBoxStyle (juce::Slider::TextBoxBelow, false, 88, 24);
    slider.setTextValueSuffix (suffix);
    slider.setNumDecimalPlacesToDisplay (decimals);
    slider.setColour (juce::Slider::rotarySliderFillColourId, accent);
    slider.setColour (juce::Slider::thumbColourId, accent);
    addAndMakeVisible (slider);

    label.setText (name, juce::dontSendNotification);
    styleLabel (label, 10.4f, text);
    addAndMakeVisible (label);
}

void SantosLevelerAudioProcessorEditor::configureFader (juce::Slider& slider, juce::Label& label,
                                                         const juce::String& name, const juce::String& suffix,
                                                         int decimals, juce::Colour accent)
{
    slider.setSliderStyle (juce::Slider::LinearHorizontal);
    slider.setTextBoxStyle (juce::Slider::TextBoxAbove, false, 84, 22);
    slider.setTextValueSuffix (suffix);
    slider.setNumDecimalPlacesToDisplay (decimals);
    slider.setColour (juce::Slider::thumbColourId, accent);
    addAndMakeVisible (slider);

    label.setText (name, juce::dontSendNotification);
    styleLabel (label, 10.2f, accent);
    addAndMakeVisible (label);
}

void SantosLevelerAudioProcessorEditor::paint (juce::Graphics& g)
{
    g.fillAll (bg);

    const float sx = (float) getWidth() / 1200.0f;
    const float sy = (float) getHeight() / 800.0f;
    const float S = juce::jmin (sx, sy);

    // multi-layer aluminium frame
    auto outer = getLocalBounds().toFloat().reduced (3.0f);
    juce::ColourGradient outerGrad (juce::Colour (0xff63737d), outer.getTopLeft(),
                                    juce::Colour (0xff06090b), outer.getBottomRight(), false);
    outerGrad.addColour (0.15, juce::Colour (0xff33434d));
    outerGrad.addColour (0.42, juce::Colour (0xff162128));
    outerGrad.addColour (0.74, juce::Colour (0xff0a0f12));
    g.setGradientFill (outerGrad);
    g.fillRoundedRectangle (outer, 13.0f * S);
    g.setColour (juce::Colour (0xff8b9aa2));
    g.drawRoundedRectangle (outer.reduced (0.5f), 13.0f * S, 1.0f);
    g.setColour (juce::Colours::white.withAlpha (0.07f));
    g.drawRoundedRectangle (outer.reduced (2.0f), 11.0f * S, 0.8f);
    g.setColour (juce::Colours::black.withAlpha (0.82f));
    g.drawRoundedRectangle (outer.reduced (4.4f), 9.0f * S, 2.0f);

    auto inner = outer.reduced (7.0f);
    g.setColour (frame);
    g.fillRoundedRectangle (inner, 7.0f * S);

    // header panel
    auto header = juce::Rectangle<float> (14.0f * sx, 14.0f * sy,
                                          (float) getWidth() - 28.0f * sx, 58.0f * sy);
    juce::ColourGradient headGrad (juce::Colour (0xff11191e), header.getTopLeft(),
                                   juce::Colour (0xff05090c), header.getBottomRight(), false);
    g.setGradientFill (headGrad);
    g.fillRoundedRectangle (header, 5.0f * S);
    g.setColour (border.withAlpha (0.88f));
    g.drawRoundedRectangle (header, 5.0f * S, 1.0f);

    // two-colour product title
    g.setFont (juce::FontOptions (27.5f * S));
    g.setColour (text);
    const auto titleX = 30.0f * sx;
    const auto titleY = 20.0f * sy;
    g.drawText ("SANTOS", (int) titleX, (int) titleY,
                (int) (130.0f * sx), (int) (31.0f * sy), juce::Justification::centredLeft);
    const auto santosW = g.getCurrentFont().getStringWidthFloat ("SANTOS");
    g.setColour (cyan);
    g.drawText ("LEVELER", (int) (titleX + santosW + 8.0f * sx), (int) titleY,
                (int) (170.0f * sx), (int) (31.0f * sy), juce::Justification::centredLeft);
    g.setFont (juce::FontOptions (9.5f * S));
    g.setColour (muted);
    g.drawText ("VOICE AUTO LEVEL RIDER", (int) titleX, (int) (49.0f * sy),
                (int) (250.0f * sx), (int) (14.0f * sy), juce::Justification::centredLeft);

    // Auto Riding status badge
    auto ride = juce::Rectangle<float> ((float) getWidth() * 0.315f,
                                        23.0f * sy, 150.0f * sx, 34.0f * sy);
    const auto active = processor.getRiderActive();
    g.setColour ((active ? green : juce::Colour (0xff22302a)).withAlpha (0.20f));
    g.fillRoundedRectangle (ride, 4.0f * S);
    g.setColour ((active ? green : muted).withAlpha (0.72f));
    g.drawRoundedRectangle (ride, 4.0f * S, 1.0f);
    g.setFont (juce::FontOptions (12.0f * S));
    g.setColour (active ? green : muted);
    g.drawText (active ? "AUTO RIDING" : "IDLE",
                ride.toNearestInt().withTrimmedRight ((int) (28 * sx)), juce::Justification::centred);
    g.fillEllipse (ride.getRight() - 22.0f * sx, ride.getCentreY() - 4.0f * S, 8.0f * S, 8.0f * S);

    // Static identity badge - no fake controls.
    g.setFont (juce::FontOptions (9.0f * S));
    g.setColour (muted);
    g.drawText ("CLASSIC STUDIO", (int) (690 * sx), (int) (29 * sy),
                (int) (155 * sx), (int) (20 * sy), juce::Justification::centred);

    // top-right detail rails inspired by the approved mockup
    auto rail = juce::Rectangle<float> (864.0f * sx, 23.0f * sy, 205.0f * sx, 34.0f * sy);
    drawPanel (g, rail, 4.0f * S);
    g.setFont (juce::FontOptions (8.3f * S));
    g.setColour (muted);
    g.drawText ("MOTION", rail.toNearestInt().withTrimmedRight ((int) (105 * sx)), juce::Justification::centred);
    g.setColour (text);
    g.drawText ("FULL", rail.toNearestInt().withTrimmedLeft ((int) (105 * sx)), juce::Justification::centred);

    auto knobPanel = juce::Rectangle<float> (16.0f * sx, 502.0f * sy,
                                             (float) getWidth() - 32.0f * sx, 150.0f * sy);
    drawPanel (g, knobPanel, 7.0f * S);

    // subtle vertical separators between knob cells
    const auto knobCellW = knobPanel.getWidth() / 8.0f;
    for (int i = 1; i < 8; ++i)
    {
        const auto x = knobPanel.getX() + knobCellW * (float) i;
        g.setColour (juce::Colours::black.withAlpha (0.40f));
        g.drawVerticalLine ((int) x, knobPanel.getY() + 9.0f, knobPanel.getBottom() - 9.0f);
        g.setColour (juce::Colours::white.withAlpha (0.025f));
        g.drawVerticalLine ((int) x + 1, knobPanel.getY() + 9.0f, knobPanel.getBottom() - 9.0f);
    }

    auto faderPanel = juce::Rectangle<float> (16.0f * sx, 660.0f * sy,
                                              (float) getWidth() - 32.0f * sx, 102.0f * sy);
    drawPanel (g, faderPanel, 7.0f * S);

    // fader scale numbers exactly where the approved concept places them
    const auto fStart = 35.0f;
    const auto fGap = 18.0f;
    const auto fW = (1130.0f - fGap * 2.0f) / 3.0f;
    auto s0 = juce::Rectangle<float> (fStart * sx, 737.0f * sy, fW * sx, 22.0f * sy);
    auto s1 = juce::Rectangle<float> ((fStart + fW + fGap) * sx, 737.0f * sy, fW * sx, 22.0f * sy);
    auto s2 = juce::Rectangle<float> ((fStart + 2.0f * (fW + fGap)) * sx, 737.0f * sy, fW * sx, 22.0f * sy);
    drawHorizontalScale (g, s0, { "-24", "-18", "-12", "-6", "0" });
    drawHorizontalScale (g, s1, { "0", "6", "12", "18", "24" });
    drawHorizontalScale (g, s2, { "-24", "-12", "0", "+12", "+24" });

    // footer
    g.setFont (juce::FontOptions (8.0f * S));
    g.setColour (muted.withAlpha (0.72f));
    g.drawText ("SANTOS LEVELER v1.0.0", (int) (24 * sx), (int) (772 * sy),
                (int) (160 * sx), (int) (16 * sy), juce::Justification::centredLeft);
    g.setColour (cyan.withAlpha (0.86f));
    g.drawText ("AUTO LEVEL RIDER TECHNOLOGY", (int) (220 * sx), (int) (772 * sy),
                (int) (220 * sx), (int) (16 * sy), juce::Justification::centredLeft);
    g.setColour (muted.withAlpha (0.72f));
    g.drawText ("DESIGNED & DEVELOPED BY SANTOS", (int) (925 * sx), (int) (772 * sy),
                (int) (245 * sx), (int) (16 * sy), juce::Justification::centredRight);
}

void SantosLevelerAudioProcessorEditor::resized()
{
    const auto sx = (float) getWidth() / 1200.0f;
    const auto sy = (float) getHeight() / 800.0f;

    titleLabel.setBounds (0, 0, 1, 1);
    subtitleLabel.setBounds (0, 0, 1, 1);

    inputMeter.setBounds ((int) (18 * sx), (int) (82 * sy), (int) (132 * sx), (int) (408 * sy));
    outputMeter.setBounds ((int) (1050 * sx), (int) (82 * sy), (int) (132 * sx), (int) (408 * sy));
    history.setBounds ((int) (158 * sx), (int) (82 * sy), (int) (884 * sx), (int) (408 * sy));

    juce::Slider* knobs[] = { &gateKnob, &targetKnob, &speedKnob, &detectKnob,
                              &lookaheadKnob, &holdKnob, &releaseKnob, &peakThresholdKnob };
    juce::Label* labels[] = { &gateLabel, &targetLabel, &speedLabel, &detectLabel,
                             &lookaheadLabel, &holdLabel, &releaseLabel, &peakThresholdLabel };

    const float startX = 30.0f;
    const float totalW = 1140.0f;
    const float cellW = totalW / 8.0f;
    for (int i = 0; i < 8; ++i)
    {
        const auto x = (startX + cellW * (float) i) * sx;
        labels[i]->setBounds ((int) x, (int) (511 * sy), (int) (cellW * sx), (int) (18 * sy));
        knobs[i]->setBounds ((int) (x + 5 * sx), (int) (529 * sy),
                             (int) ((cellW - 10) * sx), (int) (115 * sy));
    }

    const float fStart = 35.0f;
    const float fGap = 18.0f;
    const float fW = (1130.0f - fGap * 2.0f) / 3.0f;
    juce::Slider* faders[] = { &rangeDownSlider, &rangeUpSlider, &outputSlider };
    juce::Label* fLabels[] = { &rangeDownLabel, &rangeUpLabel, &outputLabel };
    for (int i = 0; i < 3; ++i)
    {
        const auto x = (fStart + (fW + fGap) * (float) i) * sx;
        fLabels[i]->setBounds ((int) x, (int) (668 * sy), (int) (fW * sx), (int) (18 * sy));
        faders[i]->setBounds ((int) x, (int) (686 * sy), (int) (fW * sx), (int) (55 * sy));
    }
}

void SantosLevelerAudioProcessorEditor::timerCallback()
{
    repaint();
    history.repaint();
    inputMeter.repaint();
    outputMeter.repaint();
}

void SantosLevelerAudioProcessorEditor::HistoryComponent::paint (juce::Graphics& g)
{
    auto r = getLocalBounds().toFloat();
    drawPanel (g, r, 6.0f);

    const auto topBarH = juce::jmax (27.0f, r.getHeight() * 0.075f);
    const auto statH = juce::jmax (66.0f, r.getHeight() * 0.19f);
    auto chartArea = r.reduced (10.0f);
    auto topBar = chartArea.removeFromTop (topBarH);
    auto stats = chartArea.removeFromBottom (statH);
    chartArea.removeFromBottom (6.0f);

    g.setColour (text);
    g.setFont (juce::FontOptions (11.0f));
    g.drawText ("LIVE RESPONSE", topBar.toNearestInt(), juce::Justification::centredLeft);

    const juce::Colour cols[] = { cyan, yellow, magenta, green };
    const char* names[] = { "INPUT", "RIDER", "PEAK", "OUTPUT" };
    auto legend = topBar.removeFromRight (juce::jmin (topBar.getWidth() * 0.48f, 360.0f));
    const auto itemW = legend.getWidth() / 4.0f;
    for (int i = 0; i < 4; ++i)
    {
        auto item = legend.removeFromLeft (itemW);
        g.setColour (cols[i]);
        g.drawLine (item.getX() + 3.0f, item.getCentreY(), item.getX() + 20.0f, item.getCentreY(), 1.6f);
        g.setFont (juce::FontOptions (8.4f));
        g.drawText (names[i], item.withTrimmedLeft (23.0f).toNearestInt(), juce::Justification::centredLeft);
    }

    auto plot = chartArea.reduced (20.0f, 8.0f);
    g.setColour (juce::Colour (0xff03080b));
    g.fillRoundedRectangle (chartArea, 4.0f);
    g.setColour (border.withAlpha (0.62f));
    g.drawRoundedRectangle (chartArea.reduced (0.5f), 4.0f, 1.0f);

    // technical grid and level labels
    g.setFont (juce::FontOptions (7.8f));
    for (int i = 0; i <= 5; ++i)
    {
        const auto y = plot.getY() + plot.getHeight() * (float) i / 5.0f;
        g.setColour (juce::Colour (0xff25353d).withAlpha (0.70f));
        g.drawHorizontalLine ((int) y, plot.getX(), plot.getRight());
        g.setColour (muted);
        g.drawText (juce::String (-12 * i), (int) chartArea.getX() + 1, (int) y - 7,
                    18, 14, juce::Justification::centredRight);
    }
    for (int i = 0; i <= 12; ++i)
    {
        const auto x = plot.getX() + plot.getWidth() * (float) i / 12.0f;
        g.setColour (juce::Colour (0xff1b2930).withAlpha (0.55f));
        g.drawVerticalLine ((int) x, plot.getY(), plot.getBottom());
    }
    g.setColour (muted.withAlpha (0.85f));
    g.drawText ("dB", (int) chartArea.getX() + 1, (int) chartArea.getBottom() - 18,
                22, 14, juce::Justification::centredRight);

    auto levelY = [&] (float db)
    {
        const auto n = juce::jlimit (0.0f, 1.0f, (db + 60.0f) / 60.0f);
        return plot.getBottom() - n * plot.getHeight();
    };
    auto gainY = [&] (float db)
    {
        const auto n = juce::jlimit (0.0f, 1.0f, (db + 12.0f) / 24.0f);
        return plot.getBottom() - n * plot.getHeight();
    };

    // dotted target guide
    const auto targetY = levelY (paramValue (processor, "target"));
    g.setColour (lime.withAlpha (0.52f));
    for (float x = plot.getX(); x < plot.getRight(); x += 8.0f)
        g.drawLine (x, targetY, juce::jmin (x + 4.0f, plot.getRight()), targetY, 1.0f);

    const auto points = processor.getHistory().copyLatest (300);
    if (points.size() >= 2)
    {
        auto makePath = [&] (auto valueToY)
        {
            juce::Path p;
            for (std::size_t i = 0; i < points.size(); ++i)
            {
                const auto x = plot.getX() + plot.getWidth() * (float) i / (float) (points.size() - 1);
                const auto y = valueToY (points[i]);
                if (i == 0) p.startNewSubPath (x, y); else p.lineTo (x, y);
            }
            return p;
        };
        auto makeFill = [&] (const juce::Path& source, float baseline)
        {
            auto p = source;
            p.lineTo (plot.getRight(), baseline);
            p.lineTo (plot.getX(), baseline);
            p.closeSubPath();
            return p;
        };

        auto inputPath = makePath ([&] (const SantosHistoryPoint& p) { return levelY (p.inputDb); });
        auto outputPath = makePath ([&] (const SantosHistoryPoint& p) { return levelY (p.outputDb); });
        auto riderPath = makePath ([&] (const SantosHistoryPoint& p) { return gainY (p.riderDb); });
        auto peakPath = makePath ([&] (const SantosHistoryPoint& p) { return gainY (p.peakDb); });

        // broad translucent fills like the approved render
        g.setColour (green.withAlpha (0.13f));
        g.fillPath (makeFill (outputPath, plot.getBottom()));
        g.setColour (cyan.withAlpha (0.12f));
        g.fillPath (makeFill (inputPath, plot.getBottom()));
        g.setColour (yellow.withAlpha (0.065f));
        g.fillPath (makeFill (riderPath, plot.getY()));

        // controlled glow
        g.setColour (cyan.withAlpha (0.16f)); g.strokePath (inputPath, juce::PathStrokeType (5.5f));
        g.setColour (green.withAlpha (0.14f)); g.strokePath (outputPath, juce::PathStrokeType (5.5f));
        g.setColour (yellow.withAlpha (0.13f)); g.strokePath (riderPath, juce::PathStrokeType (5.5f));
        g.setColour (magenta.withAlpha (0.11f)); g.strokePath (peakPath, juce::PathStrokeType (5.5f));

        // crisp signal lines
        g.setColour (cyan); g.strokePath (inputPath, juce::PathStrokeType (1.65f));
        g.setColour (green); g.strokePath (outputPath, juce::PathStrokeType (1.65f));
        g.setColour (yellow); g.strokePath (riderPath, juce::PathStrokeType (1.85f));
        g.setColour (magenta); g.strokePath (peakPath, juce::PathStrokeType (1.65f));
    }

    // lower numeric dashboard
    g.setColour (border.withAlpha (0.68f));
    g.drawHorizontalLine ((int) stats.getY(), stats.getX(), stats.getRight());

    const float values[] = {
        processor.getRiderDb(),
        paramValue (processor, "target"),
        paramValue (processor, "peakThreshold"),
        processor.getOutputMeterDb()
    };
    const char* names2[] = { "RIDER", "TARGET", "PEAK", "OUTPUT" };
    const juce::Colour colours[] = { yellow, lime, magenta, green };
    const char* suffix[] = { " dB", " dB", " dBFS", " dB" };

    const auto sw = stats.getWidth() / 4.0f;
    for (int i = 0; i < 4; ++i)
    {
        auto cell = stats.withX (stats.getX() + sw * (float) i).withWidth (sw);
        if (i > 0)
        {
            g.setColour (border.withAlpha (0.62f));
            g.drawVerticalLine ((int) cell.getX(), cell.getY() + 10.0f, cell.getBottom() - 10.0f);
        }
        g.setColour (muted);
        g.setFont (juce::FontOptions (9.3f));
        g.drawText (names2[i], cell.withTrimmedTop (8.0f).withHeight (16.0f).toNearestInt(), juce::Justification::centred);
        g.setColour (colours[i]);
        g.setFont (juce::FontOptions (18.7f));
        g.drawText (juce::String (values[i], 1) + suffix[i],
                    cell.withTrimmedTop (25.0f).toNearestInt(), juce::Justification::centredTop);
    }

    if (processor.hasHostTransport() && ! processor.getTransportPlaying())
    {
        g.setColour (juce::Colour (0xaa020608));
        g.fillRoundedRectangle (chartArea, 4.0f);
        g.setColour (text);
        g.setFont (juce::FontOptions (13.0f));
        g.drawText ("PAUSED", chartArea.toNearestInt(), juce::Justification::centred);
    }
}

void SantosLevelerAudioProcessorEditor::MeterComponent::paint (juce::Graphics& g)
{
    const auto db = source == Source::input ? processor.getInputMeterDb() : processor.getOutputMeterDb();
    auto r = getLocalBounds().toFloat();
    drawPanel (g, r, 6.0f);

    // display header
    g.setColour (colour);
    g.setFont (juce::FontOptions (10.6f));
    g.drawText (label, 0, 12, getWidth(), 17, juce::Justification::centred);
    g.setFont (juce::FontOptions (15.2f));
    g.drawText (juce::String (db, 1) + " dB", 0, 34, getWidth(), 22, juce::Justification::centred);

    auto meterArea = r.reduced (14.0f, 64.0f);
    meterArea.removeFromBottom (23.0f);

    const bool leftScale = source == Source::input;
    const float scaleWidth = 22.0f;
    auto scale = leftScale ? meterArea.removeFromLeft (scaleWidth) : meterArea.removeFromRight (scaleWidth);
    meterArea.reduce (5.0f, 0.0f);

    const auto gap = juce::jmax (5.0f, meterArea.getWidth() * 0.10f);
    const auto barW = (meterArea.getWidth() - gap) * 0.5f;
    constexpr int segments = 34;
    constexpr float segGap = 2.0f;
    const auto norm = juce::jlimit (0.0f, 1.0f, (db + 60.0f) / 60.0f);

    for (int ch = 0; ch < 2; ++ch)
    {
        auto bar = juce::Rectangle<float> (meterArea.getX() + (barW + gap) * (float) ch,
                                           meterArea.getY(), barW, meterArea.getHeight());
        g.setColour (juce::Colours::black.withAlpha (0.72f));
        g.fillRoundedRectangle (bar.expanded (2.0f), 3.0f);
        g.setColour (juce::Colour (0xff253239));
        g.drawRoundedRectangle (bar.expanded (1.5f), 2.5f, 0.8f);

        const auto segH = (bar.getHeight() - segGap * (segments - 1)) / (float) segments;
        for (int s = 0; s < segments; ++s)
        {
            const auto level = (float) (s + 1) / (float) segments;
            const auto y = bar.getBottom() - (float) (s + 1) * segH - (float) s * segGap;
            auto seg = juce::Rectangle<float> (bar.getX(), y, bar.getWidth(), segH);
            const auto led = meterLedColour (colour, level);
            const bool on = level <= norm + 0.0001f;

            if (on)
            {
                g.setColour (led.withAlpha (0.15f));
                g.fillRoundedRectangle (seg.expanded (2.0f, 1.0f), 1.3f);
                juce::ColourGradient ledGrad (led.brighter (0.32f), seg.getTopLeft(),
                                              led.darker (0.22f), seg.getBottomLeft(), false);
                g.setGradientFill (ledGrad);
                g.fillRoundedRectangle (seg, 0.8f);
                g.setColour (juce::Colours::white.withAlpha (0.18f));
                g.drawHorizontalLine ((int) seg.getY(), seg.getX() + 1.0f, seg.getRight() - 1.0f);
            }
            else
            {
                g.setColour (led.withAlpha (0.055f));
                g.fillRoundedRectangle (seg, 0.8f);
            }
        }

        if (norm > 0.0f)
        {
            const auto peakY = bar.getBottom() - bar.getHeight() * norm;
            g.setColour (meterLedColour (colour, norm).brighter (0.42f));
            g.fillRect (bar.getX() - 2.0f, peakY - 1.0f, bar.getWidth() + 4.0f, 2.0f);
        }
    }

    // precision side scale
    g.setFont (juce::FontOptions (7.6f));
    for (int i = 0; i <= 5; ++i)
    {
        const auto value = -12 * i;
        const auto y = scale.getY() + scale.getHeight() * (float) i / 5.0f;
        g.setColour (muted);
        g.drawText (juce::String (value), scale.withY (y - 6.0f).withHeight (12.0f).toNearestInt(),
                    leftScale ? juce::Justification::centredRight : juce::Justification::centredLeft);
        g.setColour (borderHi.withAlpha (0.52f));
        if (leftScale)
            g.drawLine (scale.getRight() - 3.0f, y, scale.getRight(), y, 0.8f);
        else
            g.drawLine (scale.getX(), y, scale.getX() + 3.0f, y, 0.8f);
    }

    g.setFont (juce::FontOptions (7.4f));
    g.setColour (muted);
    g.drawText ("RMS", 0, getHeight() - 25, getWidth(), 14, juce::Justification::centred);
}
