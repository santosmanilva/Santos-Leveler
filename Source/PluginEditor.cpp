#include "PluginEditor.h"

namespace
{
const auto bg        = juce::Colour (0xff05090c);
const auto frame     = juce::Colour (0xff10171d);
const auto panel     = juce::Colour (0xff11191f);
const auto panel2    = juce::Colour (0xff0a1014);
const auto border    = juce::Colour (0xff3b4851);
const auto borderHi  = juce::Colour (0xff66737b);
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
    juce::ColourGradient grad (juce::Colour (0xff151d22), r.getTopLeft(),
                               juce::Colour (0xff090e12), r.getBottomRight(), false);
    g.setGradientFill (grad);
    g.fillRoundedRectangle (r, radius);
    g.setColour (juce::Colours::black.withAlpha (0.72f));
    g.drawRoundedRectangle (r.translated (0.0f, 1.0f), radius, 2.0f);
    g.setColour (border.withAlpha (0.95f));
    g.drawRoundedRectangle (r.reduced (0.5f), radius, 1.0f);
    g.setColour (juce::Colours::white.withAlpha (0.055f));
    g.drawLine (r.getX() + radius, r.getY() + 1.0f, r.getRight() - radius, r.getY() + 1.0f, 1.0f);
}

float getParam (SantosLevelerAudioProcessor& p, const char* id)
{
    if (auto* v = p.apvts.getRawParameterValue (id))
        return v->load();
    return 0.0f;
}
}

SantosLevelerAudioProcessorEditor::SantosLookAndFeel::SantosLookAndFeel()
{
    setColour (juce::Slider::textBoxTextColourId, text);
    setColour (juce::Slider::textBoxBackgroundColourId, juce::Colour (0xff0a0f13));
    setColour (juce::Slider::textBoxOutlineColourId, border.withAlpha (0.9f));
    setColour (juce::Slider::rotarySliderOutlineColourId, juce::Colour (0xff28333a));
    setColour (juce::Slider::trackColourId, juce::Colour (0xff222d33));
}

void SantosLevelerAudioProcessorEditor::SantosLookAndFeel::drawRotarySlider (
    juce::Graphics& g, int x, int y, int width, int height,
    float sliderPos, float rotaryStartAngle, float rotaryEndAngle, juce::Slider& slider)
{
    auto bounds = juce::Rectangle<float> ((float) x, (float) y, (float) width, (float) height).reduced (8.0f);
    const auto radius = juce::jmax (10.0f, juce::jmin (bounds.getWidth(), bounds.getHeight()) * 0.45f);
    const auto centre = bounds.getCentre();
    const auto accent = slider.findColour (juce::Slider::rotarySliderFillColourId);
    const auto angle = rotaryStartAngle + sliderPos * (rotaryEndAngle - rotaryStartAngle);

    g.setColour (juce::Colours::black.withAlpha (0.58f));
    g.fillEllipse (juce::Rectangle<float> (radius * 1.92f, radius * 1.92f).withCentre (centre).translated (0.0f, 3.0f));

    juce::ColourGradient metal (juce::Colour (0xff536068), centre.x - radius, centre.y - radius,
                                juce::Colour (0xff0a0f12), centre.x + radius, centre.y + radius, false);
    metal.addColour (0.34, juce::Colour (0xff202b31));
    metal.addColour (0.60, juce::Colour (0xff05090b));
    g.setGradientFill (metal);
    g.fillEllipse (juce::Rectangle<float> (radius * 1.58f, radius * 1.58f).withCentre (centre));

    g.setColour (borderHi.withAlpha (0.75f));
    g.drawEllipse (juce::Rectangle<float> (radius * 1.58f, radius * 1.58f).withCentre (centre), 1.0f);
    g.setColour (juce::Colours::white.withAlpha (0.10f));
    g.drawEllipse (juce::Rectangle<float> (radius * 1.40f, radius * 1.40f).withCentre (centre).translated (-1.0f, -1.0f), 1.0f);

    const auto arcRadius = radius * 0.96f;
    const auto tickOuter = radius * 1.11f;
    for (int i = 0; i <= 18; ++i)
    {
        const auto a = rotaryStartAngle + (rotaryEndAngle - rotaryStartAngle) * (float) i / 18.0f;
        const auto p1 = centre + juce::Point<float> (std::sin (a), -std::cos (a)) * arcRadius;
        const auto p2 = centre + juce::Point<float> (std::sin (a), -std::cos (a)) * tickOuter;
        const bool active = (float) i / 18.0f <= sliderPos;
        g.setColour ((active ? accent : juce::Colour (0xff3b464c)).withAlpha (active ? 0.95f : 0.75f));
        g.drawLine ({ p1, p2 }, i % 3 == 0 ? 1.7f : 1.0f);
    }

    juce::Path arc;
    arc.addCentredArc (centre.x, centre.y, radius * 0.86f, radius * 0.86f, 0.0f,
                       rotaryStartAngle, angle, true);
    g.setColour (accent.withAlpha (0.13f));
    g.strokePath (arc, juce::PathStrokeType (7.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
    g.setColour (accent);
    g.strokePath (arc, juce::PathStrokeType (2.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

    juce::Path pointer;
    pointer.addRoundedRectangle (-1.2f, -radius * 0.59f, 2.4f, radius * 0.38f, 1.2f);
    pointer.applyTransform (juce::AffineTransform::rotation (angle).translated (centre.x, centre.y));
    g.setColour (text);
    g.fillPath (pointer);

    g.setColour (juce::Colours::white.withAlpha (0.18f));
    g.fillEllipse (centre.x - radius * 0.28f, centre.y - radius * 0.28f, radius * 0.56f, radius * 0.28f);
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
    auto area = juce::Rectangle<float> ((float) x, (float) y, (float) width, (float) height).reduced (9.0f, 9.0f);
    const auto cy = area.getCentreY();
    const auto left = area.getX();
    const auto right = area.getRight();

    g.setColour (juce::Colours::black.withAlpha (0.65f));
    g.fillRoundedRectangle (left, cy - 7.0f, right - left, 14.0f, 7.0f);
    g.setColour (juce::Colour (0xff303a40));
    g.drawRoundedRectangle ({ left, cy - 7.0f, right - left, 14.0f }, 7.0f, 1.0f);

    const auto min = slider.getMinimum();
    const auto max = slider.getMaximum();
    float zeroX = left;
    if (min < 0.0 && max > 0.0)
        zeroX = left + (float) ((0.0 - min) / (max - min)) * (right - left);

    const auto a = juce::jmin (sliderPos, zeroX);
    const auto b = juce::jmax (sliderPos, zeroX);
    g.setColour (accent.withAlpha (0.18f));
    g.fillRoundedRectangle (a, cy - 5.0f, juce::jmax (2.0f, b - a), 10.0f, 5.0f);
    g.setColour (accent);
    g.fillRoundedRectangle (a, cy - 2.0f, juce::jmax (2.0f, b - a), 4.0f, 2.0f);

    juce::ColourGradient knob (juce::Colour (0xffd9dde0), sliderPos - 7.0f, cy - 10.0f,
                               juce::Colour (0xff252c30), sliderPos + 7.0f, cy + 10.0f, false);
    g.setGradientFill (knob);
    g.fillRoundedRectangle (sliderPos - 7.0f, cy - 10.0f, 14.0f, 20.0f, 3.0f);
    g.setColour (juce::Colours::black.withAlpha (0.75f));
    g.drawRoundedRectangle ({ sliderPos - 7.0f, cy - 10.0f, 14.0f, 20.0f }, 3.0f, 1.0f);
    g.setColour (juce::Colours::white.withAlpha (0.35f));
    g.drawVerticalLine ((int) sliderPos, cy - 7.0f, cy + 7.0f);
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

    auto outer = getLocalBounds().toFloat().reduced (5.0f);
    juce::ColourGradient frameGrad (juce::Colour (0xff233039), outer.getTopLeft(),
                                    juce::Colour (0xff060a0d), outer.getBottomRight(), false);
    g.setGradientFill (frameGrad);
    g.fillRoundedRectangle (outer, 12.0f);
    g.setColour (juce::Colour (0xff6b7880));
    g.drawRoundedRectangle (outer.reduced (0.5f), 12.0f, 1.0f);
    g.setColour (juce::Colours::black.withAlpha (0.75f));
    g.drawRoundedRectangle (outer.reduced (3.0f), 10.0f, 2.0f);

    auto inner = outer.reduced (7.0f);
    g.setColour (frame);
    g.fillRoundedRectangle (inner, 7.0f);

    const float sy = (float) getHeight() / 800.0f;
    const float sx = (float) getWidth() / 1200.0f;
    const auto S = juce::jmin (sx, sy);

    auto header = juce::Rectangle<float> (14.0f * sx, 14.0f * sy, (float) getWidth() - 28.0f * sx, 58.0f * sy);
    juce::ColourGradient headGrad (juce::Colour (0xff11191f), header.getTopLeft(), juce::Colour (0xff070b0e), header.getBottomRight(), false);
    g.setGradientFill (headGrad);
    g.fillRoundedRectangle (header, 5.0f * S);
    g.setColour (border.withAlpha (0.8f));
    g.drawRoundedRectangle (header, 5.0f * S, 1.0f);

    auto ride = juce::Rectangle<float> ((float) getWidth() * 0.315f, 23.0f * sy, 150.0f * sx, 34.0f * sy);
    g.setColour ((processor.getRiderActive() ? green : juce::Colour (0xff24402f)).withAlpha (0.22f));
    g.fillRoundedRectangle (ride, 4.0f * S);
    g.setColour ((processor.getRiderActive() ? green : muted).withAlpha (0.65f));
    g.drawRoundedRectangle (ride, 4.0f * S, 1.0f);
    g.setFont (juce::FontOptions (12.0f * S));
    g.setColour (processor.getRiderActive() ? green : muted);
    g.drawText (processor.getRiderActive() ? "AUTO RIDING" : "IDLE", ride.toNearestInt().withTrimmedRight ((int) (27 * sx)), juce::Justification::centred);
    g.fillEllipse (ride.getRight() - 22.0f * sx, ride.getCentreY() - 4.0f * S, 8.0f * S, 8.0f * S);

    auto badge = juce::Rectangle<float> ((float) getWidth() * 0.53f, 23.0f * sy, 184.0f * sx, 34.0f * sy);
    drawInsetPanel (g, badge, 4.0f * S);
    g.setColour (muted);
    g.setFont (juce::FontOptions (9.0f * S));
    g.drawText ("CLASSIC STUDIO", badge.toNearestInt(), juce::Justification::centred);

    auto motion = juce::Rectangle<float> ((float) getWidth() * 0.77f, 23.0f * sy, 112.0f * sx, 34.0f * sy);
    drawInsetPanel (g, motion, 4.0f * S);
    g.setColour (muted);
    g.drawText ("MOTION  FULL", motion.toNearestInt(), juce::Justification::centred);

    auto knobPanel = juce::Rectangle<float> (16.0f * sx, 502.0f * sy, (float) getWidth() - 32.0f * sx, 150.0f * sy);
    drawInsetPanel (g, knobPanel, 7.0f * S);

    auto faderPanel = juce::Rectangle<float> (16.0f * sx, 660.0f * sy, (float) getWidth() - 32.0f * sx, 102.0f * sy);
    drawInsetPanel (g, faderPanel, 7.0f * S);

    g.setColour (muted.withAlpha (0.78f));
    g.setFont (juce::FontOptions (8.3f * S));
    g.drawText ("SANTOS LEVELER v1.0.0", (int) (24 * sx), (int) (772 * sy), (int) (160 * sx), (int) (16 * sy), juce::Justification::centredLeft);
    g.setColour (cyan.withAlpha (0.82f));
    g.drawText ("AUTO LEVEL RIDER TECHNOLOGY", (int) (220 * sx), (int) (772 * sy), (int) (210 * sx), (int) (16 * sy), juce::Justification::centredLeft);
    g.setColour (muted.withAlpha (0.78f));
    g.drawText ("DESIGNED & DEVELOPED BY SANTOS", (int) (930 * sx), (int) (772 * sy), (int) (240 * sx), (int) (16 * sy), juce::Justification::centredRight);
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

    juce::Slider* knobs[] = { &gateKnob, &targetKnob, &speedKnob, &detectKnob, &lookaheadKnob, &holdKnob, &releaseKnob, &peakThresholdKnob };
    juce::Label* labels[] = { &gateLabel, &targetLabel, &speedLabel, &detectLabel, &lookaheadLabel, &holdLabel, &releaseLabel, &peakThresholdLabel };

    const float startX = 30.0f;
    const float totalW = 1140.0f;
    const float cellW = totalW / 8.0f;
    for (int i = 0; i < 8; ++i)
    {
        const auto x = (startX + cellW * (float) i) * sx;
        labels[i]->setBounds ((int) x, (int) (514 * sy), (int) (cellW * sx), (int) (18 * sy));
        knobs[i]->setBounds ((int) (x + 6 * sx), (int) (532 * sy), (int) ((cellW - 12) * sx), (int) (112 * sy));
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
    g.setColour (juce::Colour (0xff050a0d));
    g.fillRoundedRectangle (chartArea, 4.0f);
    g.setColour (border.withAlpha (0.58f));
    g.drawRoundedRectangle (chartArea.reduced (0.5f), 4.0f, 1.0f);

    g.setFont (juce::FontOptions (7.8f));
    for (int i = 0; i <= 5; ++i)
    {
        const auto y = plot.getY() + plot.getHeight() * (float) i / 5.0f;
        g.setColour (juce::Colour (0xff223039).withAlpha (0.72f));
        g.drawHorizontalLine ((int) y, plot.getX(), plot.getRight());
        g.setColour (muted);
        g.drawText (juce::String (-12 * i), (int) chartArea.getX() + 1, (int) y - 7, 18, 14, juce::Justification::centredRight);
    }
    for (int i = 0; i <= 12; ++i)
    {
        const auto x = plot.getX() + plot.getWidth() * (float) i / 12.0f;
        g.setColour (juce::Colour (0xff1b282f).withAlpha (0.54f));
        g.drawVerticalLine ((int) x, plot.getY(), plot.getBottom());
    }

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
        g.setColour (cyan.withAlpha (0.11f));
        g.fillPath (makeFill (inputPath, plot.getBottom()));
        g.setColour (yellow.withAlpha (0.07f));
        g.fillPath (makeFill (riderPath, plot.getY()));

        g.setColour (cyan.withAlpha (0.15f)); g.strokePath (inputPath, juce::PathStrokeType (5.0f));
        g.setColour (green.withAlpha (0.12f)); g.strokePath (outputPath, juce::PathStrokeType (5.0f));
        g.setColour (yellow.withAlpha (0.12f)); g.strokePath (riderPath, juce::PathStrokeType (5.0f));
        g.setColour (magenta.withAlpha (0.10f)); g.strokePath (peakPath, juce::PathStrokeType (5.0f));

        g.setColour (cyan); g.strokePath (inputPath, juce::PathStrokeType (1.6f));
        g.setColour (green); g.strokePath (outputPath, juce::PathStrokeType (1.6f));
        g.setColour (yellow); g.strokePath (riderPath, juce::PathStrokeType (1.8f));
        g.setColour (magenta); g.strokePath (peakPath, juce::PathStrokeType (1.6f));
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
        g.drawText (statNames[i], cell.withTrimmedTop (8.0f).withHeight (16.0f).toNearestInt(), juce::Justification::centred);
        g.setColour (statCols[i]);
        g.setFont (juce::FontOptions (17.0f));
        g.drawText (juce::String (values[i], 1) + suffix[i], cell.withTrimmedTop (25.0f).toNearestInt(), juce::Justification::centredTop);
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
    g.setFont (juce::FontOptions (11.0f));
    g.drawText (label, 0, 12, getWidth(), 18, juce::Justification::centred);
    g.setFont (juce::FontOptions (15.0f));
    g.drawText (juce::String (db, 1) + " dB", 0, 36, getWidth(), 22, juce::Justification::centred);

    auto meterArea = r.reduced (18.0f, 68.0f);
    meterArea.removeFromBottom (20.0f);
    const auto gap = juce::jmax (6.0f, meterArea.getWidth() * 0.10f);
    const auto barW = (meterArea.getWidth() - gap) * 0.5f;

    for (int i = 0; i < 2; ++i)
    {
        auto bar = juce::Rectangle<float> (meterArea.getX() + (barW + gap) * (float) i,
                                           meterArea.getY(), barW, meterArea.getHeight());
        g.setColour (juce::Colour (0xff1b252b));
        g.fillRoundedRectangle (bar, 2.0f);

        for (int s = 1; s < 10; ++s)
        {
            const auto y = bar.getY() + bar.getHeight() * (float) s / 10.0f;
            g.setColour (juce::Colours::black.withAlpha (0.30f));
            g.drawHorizontalLine ((int) y, bar.getX(), bar.getRight());
        }

        const auto norm = juce::jlimit (0.0f, 1.0f, (db + 60.0f) / 60.0f);
        auto fill = bar;
        fill.setY (bar.getBottom() - bar.getHeight() * norm);
        fill.setHeight (bar.getHeight() * norm);
        juce::ColourGradient meterGrad (colour.darker (0.35f), fill.getBottomLeft(), colour.brighter (0.22f), fill.getTopLeft(), false);
        g.setGradientFill (meterGrad);
        g.fillRoundedRectangle (fill, 2.0f);
        g.setColour (colour.brighter (0.35f));
        g.fillRect (bar.getX() - 2.0f, fill.getY() - 1.0f, bar.getWidth() + 4.0f, 2.0f);
    }

    g.setFont (juce::FontOptions (7.5f));
    g.setColour (muted);
    for (int i = 0; i <= 5; ++i)
    {
        const auto value = -12 * i;
        const auto y = meterArea.getY() + meterArea.getHeight() * (float) i / 5.0f;
        g.drawText (juce::String (value), 1, (int) y - 6, 16, 12, juce::Justification::centredLeft);
    }
    g.drawText ("RMS", 0, getHeight() - 26, getWidth(), 16, juce::Justification::centred);
}
