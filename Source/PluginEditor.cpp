#include "PluginEditor.h"

namespace
{
const auto bg        = juce::Colour (0xff05090c);
const auto frame     = juce::Colour (0xff10171d);
const auto panel     = juce::Colour (0xff11191f);
const auto panel2    = juce::Colour (0xff090e12);
const auto border    = juce::Colour (0xff3b4851);
const auto borderHi  = juce::Colour (0xff75838c);
const auto text      = juce::Colour (0xffeef3f5);
const auto muted     = juce::Colour (0xff89969e);
const auto cyan      = juce::Colour (0xff18c9f4);
const auto yellow    = juce::Colour (0xffffd51f);
const auto green     = juce::Colour (0xff67e05f);
const auto lime      = juce::Colour (0xff8ae65f);
const auto magenta   = juce::Colour (0xffff55d8);

void styleLabel (juce::Label& label, float size, juce::Colour colour,
                 int justification = juce::Justification::centred)
{
    label.setFont (juce::FontOptions (size));
    label.setColour (juce::Label::textColourId, colour);
    label.setJustificationType (justification);
    label.setInterceptsMouseClicks (false, false);
}

void drawInsetPanel (juce::Graphics& g, juce::Rectangle<float> r, float radius = 7.0f)
{
    g.setColour (juce::Colours::black.withAlpha (0.56f));
    g.fillRoundedRectangle (r.translated (0.0f, 2.0f), radius);

    juce::ColourGradient grad (juce::Colour (0xff172027), r.getTopLeft(),
                               juce::Colour (0xff080d10), r.getBottomRight(), false);
    grad.addColour (0.52, juce::Colour (0xff10171c));
    g.setGradientFill (grad);
    g.fillRoundedRectangle (r, radius);

    g.setColour (border.withAlpha (0.95f));
    g.drawRoundedRectangle (r.reduced (0.5f), radius, 1.0f);
    g.setColour (juce::Colours::white.withAlpha (0.06f));
    g.drawLine (r.getX() + radius, r.getY() + 1.0f,
                r.getRight() - radius, r.getY() + 1.0f, 1.0f);
}

float getParam (SantosLevelerAudioProcessor& p, const char* id)
{
    if (auto* v = p.apvts.getRawParameterValue (id))
        return v->load();
    return 0.0f;
}

juce::Colour meterColourForNorm (juce::Colour base, float norm)
{
    if (norm > 0.93f)
        return juce::Colour (0xffff5c61);
    if (norm > 0.82f)
        return juce::Colour (0xffffcf43);
    return base;
}
}

SantosLevelerAudioProcessorEditor::SantosLookAndFeel::SantosLookAndFeel()
{
    setColour (juce::Slider::textBoxTextColourId, text);
    setColour (juce::Slider::textBoxBackgroundColourId, juce::Colour (0xff090e12));
    setColour (juce::Slider::textBoxOutlineColourId, border.withAlpha (0.90f));
    setColour (juce::Slider::rotarySliderOutlineColourId, juce::Colour (0xff28333a));
    setColour (juce::Slider::trackColourId, juce::Colour (0xff222d33));
}

void SantosLevelerAudioProcessorEditor::SantosLookAndFeel::drawRotarySlider (
    juce::Graphics& g, int x, int y, int width, int height,
    float sliderPos, float rotaryStartAngle, float rotaryEndAngle, juce::Slider& slider)
{
    auto b = juce::Rectangle<float> ((float) x, (float) y, (float) width, (float) height).reduced (7.0f);
    const auto radius = juce::jmax (11.0f, juce::jmin (b.getWidth(), b.getHeight()) * 0.43f);
    const auto c = b.getCentre();
    const auto accent = slider.findColour (juce::Slider::rotarySliderFillColourId);
    const auto angle = rotaryStartAngle + sliderPos * (rotaryEndAngle - rotaryStartAngle);

    // Soft outer glow + deep shadow.
    g.setColour (accent.withAlpha (0.045f));
    g.fillEllipse (juce::Rectangle<float> (radius * 2.28f, radius * 2.28f).withCentre (c));
    g.setColour (juce::Colours::black.withAlpha (0.72f));
    g.fillEllipse (juce::Rectangle<float> (radius * 1.95f, radius * 1.95f).withCentre (c).translated (0.0f, 3.0f));

    // Outer metal bezel.
    auto bezel = juce::Rectangle<float> (radius * 1.82f, radius * 1.82f).withCentre (c);
    juce::ColourGradient bezelGrad (juce::Colour (0xff77838a), bezel.getTopLeft(),
                                    juce::Colour (0xff10161a), bezel.getBottomRight(), false);
    bezelGrad.addColour (0.30, juce::Colour (0xff39454c));
    bezelGrad.addColour (0.58, juce::Colour (0xff151d21));
    bezelGrad.addColour (0.82, juce::Colour (0xff06090b));
    g.setGradientFill (bezelGrad);
    g.fillEllipse (bezel);
    g.setColour (juce::Colours::black.withAlpha (0.82f));
    g.drawEllipse (bezel, 1.2f);
    g.setColour (juce::Colours::white.withAlpha (0.11f));
    g.drawEllipse (bezel.reduced (1.2f), 0.8f);

    // Inner knob body.
    auto body = juce::Rectangle<float> (radius * 1.42f, radius * 1.42f).withCentre (c);
    juce::ColourGradient bodyGrad (juce::Colour (0xff4a555b), body.getTopLeft(),
                                   juce::Colour (0xff05080a), body.getBottomRight(), false);
    bodyGrad.addColour (0.38, juce::Colour (0xff20292e));
    bodyGrad.addColour (0.66, juce::Colour (0xff0c1114));
    g.setGradientFill (bodyGrad);
    g.fillEllipse (body);
    g.setColour (juce::Colour (0xff050708));
    g.drawEllipse (body, 1.3f);

    // Tick ring, with active ticks glowing like the approved render.
    const auto tickInner = radius * 0.98f;
    const auto tickOuter = radius * 1.14f;
    constexpr int tickCount = 25;
    for (int i = 0; i < tickCount; ++i)
    {
        const auto frac = (float) i / (float) (tickCount - 1);
        const auto a = rotaryStartAngle + (rotaryEndAngle - rotaryStartAngle) * frac;
        const auto p1 = c + juce::Point<float> (std::sin (a), -std::cos (a)) * tickInner;
        const auto p2 = c + juce::Point<float> (std::sin (a), -std::cos (a)) * tickOuter;
        const bool active = frac <= sliderPos + 0.001f;
        const auto col = active ? accent : juce::Colour (0xff354149);

        if (active && i % 2 == 0)
        {
            g.setColour (accent.withAlpha (0.14f));
            g.drawLine ({ p1, p2 }, 4.0f);
        }

        g.setColour (col.withAlpha (active ? 0.98f : 0.72f));
        g.drawLine ({ p1, p2 }, i % 4 == 0 ? 1.8f : 1.05f);
    }

    // Value arc.
    juce::Path arc;
    arc.addCentredArc (c.x, c.y, radius * 0.88f, radius * 0.88f, 0.0f,
                       rotaryStartAngle, angle, true);
    g.setColour (accent.withAlpha (0.14f));
    g.strokePath (arc, juce::PathStrokeType (8.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
    g.setColour (accent);
    g.strokePath (arc, juce::PathStrokeType (2.2f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

    // Pointer with small luminous cap.
    juce::Path pointer;
    pointer.addRoundedRectangle (-1.45f, -radius * 0.61f, 2.9f, radius * 0.40f, 1.4f);
    pointer.applyTransform (juce::AffineTransform::rotation (angle).translated (c.x, c.y));
    g.setColour (juce::Colours::black.withAlpha (0.65f));
    g.fillPath (pointer, juce::AffineTransform::translation (1.0f, 1.0f));
    g.setColour (text);
    g.fillPath (pointer);

    g.setColour (accent.withAlpha (0.80f));
    g.fillEllipse (c.x - 2.1f, c.y - 2.1f, 4.2f, 4.2f);

    // Brushed-metal highlight.
    juce::ColourGradient highlight (juce::Colours::white.withAlpha (0.20f), c.x - radius * 0.35f, c.y - radius * 0.46f,
                                    juce::Colours::transparentWhite, c.x + radius * 0.15f, c.y + radius * 0.24f, false);
    g.setGradientFill (highlight);
    g.fillEllipse (body.reduced (radius * 0.10f).withHeight (body.getHeight() * 0.46f));
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
    auto area = juce::Rectangle<float> ((float) x, (float) y, (float) width, (float) height).reduced (10.0f, 9.0f);
    const auto cy = area.getCentreY();
    const auto left = area.getX();
    const auto right = area.getRight();

    g.setColour (juce::Colours::black.withAlpha (0.70f));
    g.fillRoundedRectangle (left, cy - 7.5f, right - left, 15.0f, 7.5f);
    g.setColour (juce::Colour (0xff38444b));
    g.drawRoundedRectangle ({ left, cy - 7.5f, right - left, 15.0f }, 7.5f, 1.0f);

    const auto min = slider.getMinimum();
    const auto max = slider.getMaximum();
    float zeroX = left;
    if (min < 0.0 && max > 0.0)
        zeroX = left + (float) ((0.0 - min) / (max - min)) * (right - left);

    const auto a = juce::jmin (sliderPos, zeroX);
    const auto b = juce::jmax (sliderPos, zeroX);
    g.setColour (accent.withAlpha (0.16f));
    g.fillRoundedRectangle (a, cy - 5.0f, juce::jmax (2.0f, b - a), 10.0f, 5.0f);
    g.setColour (accent.withAlpha (0.38f));
    g.fillRoundedRectangle (a, cy - 3.0f, juce::jmax (2.0f, b - a), 6.0f, 3.0f);
    g.setColour (accent);
    g.fillRoundedRectangle (a, cy - 1.5f, juce::jmax (2.0f, b - a), 3.0f, 1.5f);

    juce::ColourGradient thumbGrad (juce::Colour (0xffe7ecef), sliderPos - 8.0f, cy - 11.0f,
                                    juce::Colour (0xff232a2e), sliderPos + 8.0f, cy + 11.0f, false);
    thumbGrad.addColour (0.48, juce::Colour (0xff8d969b));
    g.setGradientFill (thumbGrad);
    g.fillRoundedRectangle (sliderPos - 7.5f, cy - 11.0f, 15.0f, 22.0f, 3.2f);
    g.setColour (juce::Colours::black.withAlpha (0.82f));
    g.drawRoundedRectangle ({ sliderPos - 7.5f, cy - 11.0f, 15.0f, 22.0f }, 3.2f, 1.0f);
    g.setColour (juce::Colours::white.withAlpha (0.45f));
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

    titleLabel.setText ("SANTOS LEVELER", juce::dontSendNotification);
    styleLabel (titleLabel, 27.0f, text, juce::Justification::centredLeft);
    addAndMakeVisible (titleLabel);

    subtitleLabel.setText ("VOICE AUTO LEVEL RIDER", juce::dontSendNotification);
    styleLabel (subtitleLabel, 10.0f, muted, juce::Justification::centredLeft);
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
    slider.setTextBoxStyle (juce::Slider::TextBoxBelow, false, 86, 24);
    slider.setTextValueSuffix (suffix);
    slider.setNumDecimalPlacesToDisplay (decimals);
    slider.setColour (juce::Slider::rotarySliderFillColourId, accent);
    slider.setColour (juce::Slider::thumbColourId, accent);
    addAndMakeVisible (slider);

    label.setText (name, juce::dontSendNotification);
    styleLabel (label, 10.5f, text);
    addAndMakeVisible (label);
}

void SantosLevelerAudioProcessorEditor::configureFader (juce::Slider& slider, juce::Label& label,
                                                         const juce::String& name, const juce::String& suffix,
                                                         int decimals, juce::Colour accent)
{
    slider.setSliderStyle (juce::Slider::LinearHorizontal);
    slider.setTextBoxStyle (juce::Slider::TextBoxAbove, false, 82, 22);
    slider.setTextValueSuffix (suffix);
    slider.setNumDecimalPlacesToDisplay (decimals);
    slider.setColour (juce::Slider::thumbColourId, accent);
    addAndMakeVisible (slider);

    label.setText (name, juce::dontSendNotification);
    styleLabel (label, 10.5f, accent);
    addAndMakeVisible (label);
}

void SantosLevelerAudioProcessorEditor::paint (juce::Graphics& g)
{
    g.fillAll (bg);

    auto outer = getLocalBounds().toFloat().reduced (4.0f);
    juce::ColourGradient frameGrad (juce::Colour (0xff33424b), outer.getTopLeft(),
                                    juce::Colour (0xff05080a), outer.getBottomRight(), false);
    frameGrad.addColour (0.38, juce::Colour (0xff182329));
    g.setGradientFill (frameGrad);
    g.fillRoundedRectangle (outer, 13.0f);
    g.setColour (juce::Colour (0xff7b8991));
    g.drawRoundedRectangle (outer.reduced (0.5f), 13.0f, 1.0f);
    g.setColour (juce::Colours::white.withAlpha (0.06f));
    g.drawRoundedRectangle (outer.reduced (2.2f), 11.0f, 0.8f);
    g.setColour (juce::Colours::black.withAlpha (0.80f));
    g.drawRoundedRectangle (outer.reduced (4.2f), 9.0f, 2.0f);

    auto inner = outer.reduced (7.0f);
    g.setColour (frame);
    g.fillRoundedRectangle (inner, 7.0f);

    const float sy = (float) getHeight() / 800.0f;
    const float sx = (float) getWidth() / 1200.0f;
    const auto S = juce::jmin (sx, sy);

    auto header = juce::Rectangle<float> (14.0f * sx, 14.0f * sy,
                                          (float) getWidth() - 28.0f * sx, 58.0f * sy);
    juce::ColourGradient headGrad (juce::Colour (0xff121b21), header.getTopLeft(),
                                   juce::Colour (0xff060a0d), header.getBottomRight(), false);
    g.setGradientFill (headGrad);
    g.fillRoundedRectangle (header, 5.0f * S);
    g.setColour (border.withAlpha (0.88f));
    g.drawRoundedRectangle (header, 5.0f * S, 1.0f);
    g.setColour (juce::Colours::white.withAlpha (0.04f));
    g.drawLine (header.getX() + 6.0f, header.getY() + 1.0f,
                header.getRight() - 6.0f, header.getY() + 1.0f, 1.0f);

    auto ride = juce::Rectangle<float> ((float) getWidth() * 0.315f,
                                        23.0f * sy, 150.0f * sx, 34.0f * sy);
    const auto active = processor.getRiderActive();
    g.setColour ((active ? green : juce::Colour (0xff263239)).withAlpha (0.24f));
    g.fillRoundedRectangle (ride, 4.0f * S);
    g.setColour ((active ? green : muted).withAlpha (0.70f));
    g.drawRoundedRectangle (ride, 4.0f * S, 1.0f);
    g.setFont (juce::FontOptions (12.0f * S));
    g.setColour (active ? green : muted);
    g.drawText (active ? "AUTO RIDING" : "IDLE",
                ride.toNearestInt().withTrimmedRight ((int) (27 * sx)), juce::Justification::centred);
    g.fillEllipse (ride.getRight() - 22.0f * sx, ride.getCentreY() - 4.0f * S, 8.0f * S, 8.0f * S);

    auto badge = juce::Rectangle<float> ((float) getWidth() * 0.53f,
                                         23.0f * sy, 184.0f * sx, 34.0f * sy);
    drawInsetPanel (g, badge, 4.0f * S);
    g.setColour (muted);
    g.setFont (juce::FontOptions (9.0f * S));
    g.drawText ("CLASSIC STUDIO", badge.toNearestInt(), juce::Justification::centred);

    auto motion = juce::Rectangle<float> ((float) getWidth() * 0.77f,
                                          23.0f * sy, 112.0f * sx, 34.0f * sy);
    drawInsetPanel (g, motion, 4.0f * S);
    g.setColour (muted);
    g.drawText ("MOTION  FULL", motion.toNearestInt(), juce::Justification::centred);

    auto knobPanel = juce::Rectangle<float> (16.0f * sx, 502.0f * sy,
                                             (float) getWidth() - 32.0f * sx, 150.0f * sy);
    drawInsetPanel (g, knobPanel, 7.0f * S);

    auto faderPanel = juce::Rectangle<float> (16.0f * sx, 660.0f * sy,
                                              (float) getWidth() - 32.0f * sx, 102.0f * sy);
    drawInsetPanel (g, faderPanel, 7.0f * S);

    g.setColour (muted.withAlpha (0.72f));
    g.setFont (juce::FontOptions (8.0f * S));
    g.drawText ("SANTOS LEVELER v1.0.0", (int) (24 * sx), (int) (772 * sy),
                (int) (160 * sx), (int) (16 * sy), juce::Justification::centredLeft);
    g.setColour (cyan.withAlpha (0.85f));
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

    titleLabel.setBounds ((int) (26 * sx), (int) (20 * sy), (int) (280 * sx), (int) (30 * sy));
    subtitleLabel.setBounds ((int) (28 * sx), (int) (49 * sy), (int) (240 * sx), (int) (14 * sy));

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
        labels[i]->setBounds ((int) x, (int) (514 * sy), (int) (cellW * sx), (int) (18 * sy));
        knobs[i]->setBounds ((int) (x + 5 * sx), (int) (531 * sy),
                             (int) ((cellW - 10) * sx), (int) (113 * sy));
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
        faders[i]->setBounds ((int) x, (int) (685 * sy), (int) (fW * sx), (int) (67 * sy));
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
    drawInsetPanel (g, r, 6.0f);

    const auto topBarH = juce::jmax (26.0f, r.getHeight() * 0.075f);
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
        g.drawLine (item.getX() + 3.0f, item.getCentreY(), item.getX() + 18.0f, item.getCentreY(), 1.6f);
        g.setFont (juce::FontOptions (8.4f));
        g.drawText (names[i], item.withTrimmedLeft (21.0f).toNearestInt(), juce::Justification::centredLeft);
    }

    auto plot = chartArea.reduced (20.0f, 8.0f);
    g.setColour (juce::Colour (0xff04090c));
    g.fillRoundedRectangle (chartArea, 4.0f);
    g.setColour (border.withAlpha (0.60f));
    g.drawRoundedRectangle (chartArea.reduced (0.5f), 4.0f, 1.0f);

    g.setFont (juce::FontOptions (7.8f));
    for (int i = 0; i <= 5; ++i)
    {
        const auto y = plot.getY() + plot.getHeight() * (float) i / 5.0f;
        g.setColour (juce::Colour (0xff223039).withAlpha (0.72f));
        g.drawHorizontalLine ((int) y, plot.getX(), plot.getRight());
        g.setColour (muted);
        g.drawText (juce::String (-12 * i), (int) chartArea.getX() + 1, (int) y - 7,
                    18, 14, juce::Justification::centredRight);
    }
    for (int i = 0; i <= 12; ++i)
    {
        const auto x = plot.getX() + plot.getWidth() * (float) i / 12.0f;
        g.setColour (juce::Colour (0xff1b282f).withAlpha (0.54f));
        g.drawVerticalLine ((int) x, plot.getY(), plot.getBottom());
    }

    // Reference lines give the graph the high-end analyser look from the approved concept.
    auto levelToY = [&] (float db)
    {
        const auto norm = juce::jlimit (0.0f, 1.0f, (db + 60.0f) / 60.0f);
        return plot.getBottom() - norm * plot.getHeight();
    };

    const auto targetDb = getParam (processor, "target");
    const auto peakThreshold = getParam (processor, "peakThreshold");
    const auto targetY = levelToY (targetDb);
    const auto peakY = levelToY (peakThreshold);

    juce::Path dashedTarget;
    dashedTarget.startNewSubPath (plot.getX(), targetY);
    dashedTarget.lineTo (plot.getRight(), targetY);
    const float dash[] = { 5.0f, 5.0f };
    g.setColour (lime.withAlpha (0.50f));
    juce::PathStrokeType (1.0f).createDashedStroke (dashedTarget, dashedTarget, dash, 2);
    g.strokePath (dashedTarget, juce::PathStrokeType (1.0f));

    juce::Path dashedPeak;
    dashedPeak.startNewSubPath (plot.getX(), peakY);
    dashedPeak.lineTo (plot.getRight(), peakY);
    g.setColour (magenta.withAlpha (0.30f));
    juce::PathStrokeType (1.0f).createDashedStroke (dashedPeak, dashedPeak, dash, 2);
    g.strokePath (dashedPeak, juce::PathStrokeType (1.0f));

    const auto points = processor.getHistory().copyLatest (300);
    if (points.size() >= 2)
    {
        auto levelY = [&] (float db)
        {
            const auto norm = juce::jlimit (0.0f, 1.0f, (db + 60.0f) / 60.0f);
            return plot.getBottom() - norm * plot.getHeight();
        };
        auto gainY = [&] (float db)
        {
            const auto norm = juce::jlimit (0.0f, 1.0f, (db + 12.0f) / 24.0f);
            return plot.getBottom() - norm * plot.getHeight();
        };
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

        g.setColour (green.withAlpha (0.12f));
        g.fillPath (makeFill (outputPath, plot.getBottom()));
        g.setColour (cyan.withAlpha (0.12f));
        g.fillPath (makeFill (inputPath, plot.getBottom()));
        g.setColour (yellow.withAlpha (0.065f));
        g.fillPath (makeFill (riderPath, plot.getY()));

        g.setColour (cyan.withAlpha (0.16f)); g.strokePath (inputPath, juce::PathStrokeType (5.0f));
        g.setColour (green.withAlpha (0.13f)); g.strokePath (outputPath, juce::PathStrokeType (5.0f));
        g.setColour (yellow.withAlpha (0.13f)); g.strokePath (riderPath, juce::PathStrokeType (5.0f));
        g.setColour (magenta.withAlpha (0.11f)); g.strokePath (peakPath, juce::PathStrokeType (5.0f));

        g.setColour (cyan); g.strokePath (inputPath, juce::PathStrokeType (1.65f));
        g.setColour (green); g.strokePath (outputPath, juce::PathStrokeType (1.65f));
        g.setColour (yellow); g.strokePath (riderPath, juce::PathStrokeType (1.85f));
        g.setColour (magenta); g.strokePath (peakPath, juce::PathStrokeType (1.65f));
    }

    g.setColour (border.withAlpha (0.65f));
    g.drawHorizontalLine ((int) stats.getY(), stats.getX(), stats.getRight());

    const float values[] = {
        processor.getRiderDb(),
        getParam (processor, "target"),
        getParam (processor, "peakThreshold"),
        processor.getOutputMeterDb()
    };
    const char* statNames[] = { "RIDER", "TARGET", "PEAK", "OUTPUT" };
    const juce::Colour statCols[] = { yellow, lime, magenta, green };
    const char* suffix[] = { " dB", " dB", " dBFS", " dB" };

    const auto sw = stats.getWidth() / 4.0f;
    for (int i = 0; i < 4; ++i)
    {
        auto cell = stats.withX (stats.getX() + sw * (float) i).withWidth (sw);
        if (i > 0)
        {
            g.setColour (border.withAlpha (0.58f));
            g.drawVerticalLine ((int) cell.getX(), cell.getY() + 10.0f, cell.getBottom() - 10.0f);
        }
        g.setColour (muted);
        g.setFont (juce::FontOptions (9.3f));
        g.drawText (statNames[i], cell.withTrimmedTop (8.0f).withHeight (16.0f).toNearestInt(),
                    juce::Justification::centred);
        g.setColour (statCols[i]);
        g.setFont (juce::FontOptions (18.5f));
        g.drawText (juce::String (values[i], 1) + suffix[i],
                    cell.withTrimmedTop (25.0f).toNearestInt(), juce::Justification::centredTop);
    }

    if (processor.hasHostTransport() && ! processor.getTransportPlaying())
    {
        g.setColour (juce::Colour (0xaa050a0d));
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
    drawInsetPanel (g, r, 6.0f);

    g.setColour (colour);
    g.setFont (juce::FontOptions (10.5f));
    g.drawText (label, 0, 12, getWidth(), 17, juce::Justification::centred);
    g.setFont (juce::FontOptions (15.0f));
    g.drawText (juce::String (db, 1) + " dB", 0, 34, getWidth(), 22, juce::Justification::centred);

    auto meterArea = r.reduced (15.0f, 64.0f);
    meterArea.removeFromBottom (24.0f);

    const bool inputSide = source == Source::input;
    const float scaleW = 21.0f;
    auto scaleArea = inputSide ? meterArea.removeFromLeft (scaleW) : meterArea.removeFromRight (scaleW);
    meterArea.reduce (4.0f, 0.0f);

    const auto gap = juce::jmax (5.0f, meterArea.getWidth() * 0.08f);
    const auto barW = (meterArea.getWidth() - gap) * 0.5f;
    constexpr int segments = 30;
    const auto segGap = 2.0f;

    for (int ch = 0; ch < 2; ++ch)
    {
        auto bar = juce::Rectangle<float> (meterArea.getX() + (barW + gap) * (float) ch,
                                           meterArea.getY(), barW, meterArea.getHeight());

        g.setColour (juce::Colours::black.withAlpha (0.58f));
        g.fillRoundedRectangle (bar.expanded (2.0f), 3.0f);
        g.setColour (juce::Colour (0xff223038));
        g.drawRoundedRectangle (bar.expanded (1.5f), 2.5f, 0.8f);

        const auto norm = juce::jlimit (0.0f, 1.0f, (db + 60.0f) / 60.0f);
        const auto segmentH = (bar.getHeight() - segGap * (segments - 1)) / (float) segments;

        for (int s = 0; s < segments; ++s)
        {
            const auto levelNorm = (float) (s + 1) / (float) segments;
            const auto yy = bar.getBottom() - (float) (s + 1) * segmentH - (float) s * segGap;
            auto seg = juce::Rectangle<float> (bar.getX(), yy, bar.getWidth(), segmentH);
            const bool on = levelNorm <= norm + 0.0001f;
            const auto segColour = meterColourForNorm (colour, levelNorm);

            if (on)
            {
                g.setColour (segColour.withAlpha (0.13f));
                g.fillRoundedRectangle (seg.expanded (2.0f, 1.0f), 1.5f);
                juce::ColourGradient led (segColour.brighter (0.22f), seg.getTopLeft(),
                                          segColour.darker (0.28f), seg.getBottomLeft(), false);
                g.setGradientFill (led);
                g.fillRoundedRectangle (seg, 1.0f);
                g.setColour (juce::Colours::white.withAlpha (0.16f));
                g.drawHorizontalLine ((int) seg.getY(), seg.getX() + 1.0f, seg.getRight() - 1.0f);
            }
            else
            {
                g.setColour (segColour.withAlpha (0.075f));
                g.fillRoundedRectangle (seg, 1.0f);
            }
        }

        if (norm > 0.0f)
        {
            const auto peakY = bar.getBottom() - bar.getHeight() * norm;
            g.setColour (meterColourForNorm (colour, norm).brighter (0.40f));
            g.fillRect (bar.getX() - 2.0f, peakY - 1.0f, bar.getWidth() + 4.0f, 2.0f);
        }
    }

    g.setFont (juce::FontOptions (7.6f));
    g.setColour (muted);
    for (int i = 0; i <= 5; ++i)
    {
        const auto value = -12 * i;
        const auto y = scaleArea.getY() + scaleArea.getHeight() * (float) i / 5.0f;
        g.drawText (juce::String (value), scaleArea.withY (y - 6.0f).withHeight (12.0f).toNearestInt(),
                    inputSide ? juce::Justification::centredRight : juce::Justification::centredLeft);

        const auto tickX1 = inputSide ? scaleArea.getRight() - 3.0f : scaleArea.getX();
        const auto tickX2 = inputSide ? scaleArea.getRight() : scaleArea.getX() + 3.0f;
        g.setColour (borderHi.withAlpha (0.45f));
        g.drawLine (tickX1, y, tickX2, y, 0.8f);
        g.setColour (muted);
    }

    g.setFont (juce::FontOptions (7.4f));
    g.setColour (muted);
    g.drawText ("RMS", 0, getHeight() - 25, getWidth(), 14, juce::Justification::centred);
}
