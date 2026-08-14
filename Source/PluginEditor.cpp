#include "PluginEditor.h"

namespace
{
const auto bg        = juce::Colour (0xff0d1318);
const auto panel     = juce::Colour (0xff182128);
const auto panel2    = juce::Colour (0xff11191f);
const auto border    = juce::Colour (0xff4b5b66);
const auto text      = juce::Colour (0xffedf3f6);
const auto muted     = juce::Colour (0xff8fa0aa);
const auto cyan      = juce::Colour (0xff55c5e6);
const auto yellow    = juce::Colour (0xffe8d14b);
const auto green     = juce::Colour (0xff72d081);
const auto orange    = juce::Colour (0xffee8a73);

void styleLabel (juce::Label& label, float size, juce::Colour colour, int justification = juce::Justification::centred)
{
    label.setFont (juce::FontOptions (size));
    label.setColour (juce::Label::textColourId, colour);
    label.setJustificationType (justification);
    label.setInterceptsMouseClicks (false, false);
}
}

SantosLevelerAudioProcessorEditor::SantosLookAndFeel::SantosLookAndFeel()
{
    setColour (juce::Slider::textBoxTextColourId, text);
    setColour (juce::Slider::textBoxBackgroundColourId, juce::Colour (0xff11191f));
    setColour (juce::Slider::textBoxOutlineColourId, juce::Colours::transparentBlack);
    setColour (juce::Slider::rotarySliderFillColourId, cyan);
    setColour (juce::Slider::rotarySliderOutlineColourId, juce::Colour (0xff52616d));
    setColour (juce::Slider::thumbColourId, text);
    setColour (juce::Slider::trackColourId, juce::Colour (0xff28343d));
}

void SantosLevelerAudioProcessorEditor::SantosLookAndFeel::drawRotarySlider (
    juce::Graphics& g, int x, int y, int width, int height,
    float sliderPos, float rotaryStartAngle, float rotaryEndAngle, juce::Slider& slider)
{
    auto bounds = juce::Rectangle<float> (static_cast<float> (x), static_cast<float> (y),
                                           static_cast<float> (width), static_cast<float> (height)).reduced (8.0f);
    const auto radius = juce::jmin (bounds.getWidth(), bounds.getHeight()) * 0.5f;
    const auto centre = bounds.getCentre();
    const auto lineW = juce::jmax (4.0f, radius * 0.12f);
    const auto arcRadius = radius - lineW * 0.8f;

    juce::Path backgroundArc;
    backgroundArc.addCentredArc (centre.x, centre.y, arcRadius, arcRadius, 0.0f,
                                 rotaryStartAngle, rotaryEndAngle, true);
    g.setColour (slider.findColour (juce::Slider::rotarySliderOutlineColourId));
    g.strokePath (backgroundArc, juce::PathStrokeType (lineW, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

    const auto angle = rotaryStartAngle + sliderPos * (rotaryEndAngle - rotaryStartAngle);
    juce::Path valueArc;
    valueArc.addCentredArc (centre.x, centre.y, arcRadius, arcRadius, 0.0f,
                            rotaryStartAngle, angle, true);
    g.setColour (slider.findColour (juce::Slider::rotarySliderFillColourId));
    g.strokePath (valueArc, juce::PathStrokeType (lineW, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

    g.setColour (juce::Colour (0xff202b33));
    g.fillEllipse (juce::Rectangle<float> (radius * 1.2f, radius * 1.2f).withCentre (centre));
    g.setColour (border);
    g.drawEllipse (juce::Rectangle<float> (radius * 1.2f, radius * 1.2f).withCentre (centre), 1.0f);

    juce::Path pointer;
    const auto pointerLength = radius * 0.55f;
    const auto pointerThickness = 2.4f;
    pointer.addRoundedRectangle (-pointerThickness * 0.5f, -pointerLength, pointerThickness, pointerLength * 0.55f, 1.0f);
    pointer.applyTransform (juce::AffineTransform::rotation (angle).translated (centre.x, centre.y));
    g.setColour (text);
    g.fillPath (pointer);
}

void SantosLevelerAudioProcessorEditor::SantosLookAndFeel::drawLinearSlider (
    juce::Graphics& g, int x, int y, int width, int height,
    float sliderPos, float, float, juce::Slider::SliderStyle style, juce::Slider& slider)
{
    if (style != juce::Slider::LinearVertical)
    {
        juce::LookAndFeel_V4::drawLinearSlider (g, x, y, width, height, sliderPos, 0.0f, 0.0f, style, slider);
        return;
    }

    const auto cx = static_cast<float> (x + width / 2);
    const auto top = static_cast<float> (y + 6);
    const auto bottom = static_cast<float> (y + height - 6);
    const auto trackW = 12.0f;
    const auto accent = slider.findColour (juce::Slider::thumbColourId);

    g.setColour (slider.findColour (juce::Slider::trackColourId));
    g.fillRoundedRectangle (cx - trackW * 0.5f, top, trackW, bottom - top, 5.0f);

    float zeroPos = bottom;
    if (slider.getMinimum() < 0.0 && slider.getMaximum() > 0.0)
    {
        const auto proportion = static_cast<float> ((0.0 - slider.getMinimum()) / (slider.getMaximum() - slider.getMinimum()));
        zeroPos = bottom - proportion * (bottom - top);
    }

    g.setColour (accent.withAlpha (0.88f));
    const auto y1 = juce::jmin (sliderPos, zeroPos);
    const auto y2 = juce::jmax (sliderPos, zeroPos);
    g.fillRoundedRectangle (cx - trackW * 0.5f, y1, trackW, juce::jmax (2.0f, y2 - y1), 5.0f);

    g.setColour (accent);
    g.fillRoundedRectangle (cx - 28.0f, sliderPos - 8.0f, 56.0f, 16.0f, 4.0f);
    g.setColour (juce::Colours::white.withAlpha (0.35f));
    g.drawHorizontalLine (static_cast<int> (sliderPos), cx - 20.0f, cx + 20.0f);
}

SantosLevelerAudioProcessorEditor::SantosLevelerAudioProcessorEditor (SantosLevelerAudioProcessor& p)
    : AudioProcessorEditor (&p),
      processor (p),
      history (p),
      inputMeter (p, MeterComponent::Source::input, "INPUT LEVEL", cyan),
      outputMeter (p, MeterComponent::Source::output, "OUTPUT LEVEL", green),
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
    setSize (1000, 690);
    setResizable (true, true);
    setResizeLimits (820, 580, 1500, 1040);

    titleLabel.setText ("SANTOS LEVELER", juce::dontSendNotification);
    styleLabel (titleLabel, 24.0f, text, juce::Justification::centredLeft);
    addAndMakeVisible (titleLabel);

    subtitleLabel.setText ("VOICE AUTO LEVEL RIDER", juce::dontSendNotification);
    styleLabel (subtitleLabel, 10.5f, muted, juce::Justification::centredLeft);
    addAndMakeVisible (subtitleLabel);

    configureKnob (targetKnob, targetLabel, "TARGET", " dB", 1);
    configureKnob (gateKnob, gateLabel, "GATE", " dB", 1);
    configureKnob (speedKnob, speedLabel, "SPEED", " ms", 0);
    configureKnob (detectKnob, detectLabel, "DETECT", " ms", 0);
    configureKnob (lookaheadKnob, lookaheadLabel, "LOOKAHEAD", " ms", 0);
    configureKnob (holdKnob, holdLabel, "HOLD", " ms", 0);
    configureKnob (releaseKnob, releaseLabel, "RELEASE", " ms", 0);
    configureKnob (peakThresholdKnob, peakThresholdLabel, "PEAK", " dBFS", 1);

    configureFader (rangeDownSlider, rangeDownLabel, "RANGE DOWN", " dB", 1, orange);
    configureFader (rangeUpSlider, rangeUpLabel, "RANGE UP", " dB", 1, green);
    configureFader (outputSlider, outputLabel, "OUTPUT", " dB", 1, cyan);

    historyLabel.setText ("HISTORY", juce::dontSendNotification);
    styleLabel (historyLabel, 12.0f, text, juce::Justification::centredLeft);
    addAndMakeVisible (historyLabel);

    inputLegendLabel.setText ("INPUT", juce::dontSendNotification);
    styleLabel (inputLegendLabel, 10.0f, cyan, juce::Justification::centred);
    addAndMakeVisible (inputLegendLabel);

    riderLegendLabel.setText ("RIDER", juce::dontSendNotification);
    styleLabel (riderLegendLabel, 10.0f, yellow, juce::Justification::centred);
    addAndMakeVisible (riderLegendLabel);

    outputLegendLabel.setText ("OUTPUT", juce::dontSendNotification);
    styleLabel (outputLegendLabel, 10.0f, green, juce::Justification::centred);
    addAndMakeVisible (outputLegendLabel);

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
                                                        const juce::String& name, const juce::String& suffix, int decimals)
{
    slider.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
    slider.setTextBoxStyle (juce::Slider::TextBoxAbove, false, 76, 22);
    slider.setTextValueSuffix (suffix);
    slider.setNumDecimalPlacesToDisplay (decimals);
    slider.setColour (juce::Slider::rotarySliderFillColourId, cyan);
    addAndMakeVisible (slider);

    label.setText (name, juce::dontSendNotification);
    styleLabel (label, 11.0f, text);
    addAndMakeVisible (label);
}

void SantosLevelerAudioProcessorEditor::configureFader (juce::Slider& slider, juce::Label& label,
                                                         const juce::String& name, const juce::String& suffix,
                                                         int decimals, juce::Colour accent)
{
    slider.setSliderStyle (juce::Slider::LinearVertical);
    slider.setTextBoxStyle (juce::Slider::TextBoxBelow, false, 76, 22);
    slider.setTextValueSuffix (suffix);
    slider.setNumDecimalPlacesToDisplay (decimals);
    slider.setColour (juce::Slider::thumbColourId, accent);
    addAndMakeVisible (slider);

    label.setText (name, juce::dontSendNotification);
    styleLabel (label, 11.0f, text);
    addAndMakeVisible (label);
}

void SantosLevelerAudioProcessorEditor::paint (juce::Graphics& g)
{
    g.fillAll (bg);

    auto area = getLocalBounds().toFloat().reduced (18.0f);
    g.setColour (panel);
    g.fillRoundedRectangle (area, 10.0f);
    g.setColour (border.withAlpha (0.65f));
    g.drawRoundedRectangle (area, 10.0f, 1.0f);

    const auto width = getWidth();
    const auto topPanel = juce::Rectangle<float> (34.0f, 82.0f, static_cast<float> (width - 68), 176.0f);
    g.setColour (panel2);
    g.fillRoundedRectangle (topPanel, 8.0f);
    g.setColour (border.withAlpha (0.45f));
    g.drawRoundedRectangle (topPanel, 8.0f, 1.0f);

    const auto active = processor.getRiderActive();
    g.setColour (active ? green : muted.withAlpha (0.45f));
    g.fillEllipse (static_cast<float> (width - 76), 42.0f, 10.0f, 10.0f);
    g.setColour (muted);
    g.setFont (juce::FontOptions (10.0f));
    g.drawText (active ? "RIDE" : "IDLE", width - 132, 36, 50, 22, juce::Justification::centredRight);
}

void SantosLevelerAudioProcessorEditor::resized()
{
    const auto w = getWidth();
    const auto h = getHeight();

    titleLabel.setBounds (36, 28, 320, 30);
    subtitleLabel.setBounds (38, 56, 250, 16);

    const int knobY = 100;
    const int knobH = 130;

    constexpr int knobCount = 8;
    const int firstX = 42;
    const int rightMargin = 42;
    const int gap = 8;

    const int knobW =
        (w - firstX - rightMargin - gap * (knobCount - 1))
        / knobCount;

    juce::Slider* knobs[] =
    {
        &gateKnob,
        &targetKnob,
        &speedKnob,
        &detectKnob,
        &lookaheadKnob,
        &holdKnob,
        &releaseKnob,
        &peakThresholdKnob
    };

    juce::Label* labels[] =
    {
        &gateLabel,
        &targetLabel,
        &speedLabel,
        &detectLabel,
        &lookaheadLabel,
        &holdLabel,
        &releaseLabel,
        &peakThresholdLabel
    };

    for (int i = 0; i < knobCount; ++i)
    {
        const auto x = firstX + i * (knobW + gap);

        knobs[i]->setBounds (
            x,
            knobY,
            knobW,
            knobH);

        labels[i]->setBounds (
            x,
            knobY + knobH + 2,
            knobW,
            20);
    }

    const int lowerTop = 285;
    const int meterHeight = 58;
    const int bottomPadding = 34;
    const int meterY = h - bottomPadding - meterHeight;
    const int lowerBottom = meterY - 22;

    rangeDownLabel.setBounds (50, lowerTop, 120, 20);
    rangeDownSlider.setBounds (70, lowerTop + 24, 80, std::max (150, lowerBottom - lowerTop - 28));

    rangeUpLabel.setBounds (175, lowerTop, 120, 20);
    rangeUpSlider.setBounds (195, lowerTop + 24, 80, std::max (150, lowerBottom - lowerTop - 28));

    outputLabel.setBounds (w - 160, lowerTop, 100, 20);
    outputSlider.setBounds (w - 150, lowerTop + 24, 80, std::max (150, lowerBottom - lowerTop - 28));

    const int historyX = 310;
    const int historyW = std::max (260, w - historyX - 185);
    historyLabel.setBounds (historyX, lowerTop, 100, 20);

    const int legendWidth = 64;
    const int legendGap = 4;
    const int legendTotalWidth = legendWidth * 3 + legendGap * 2;
    const int legendStartX = historyX + historyW - legendTotalWidth;
    inputLegendLabel.setBounds (legendStartX, lowerTop, legendWidth, 20);
    riderLegendLabel.setBounds (legendStartX + legendWidth + legendGap, lowerTop, legendWidth, 20);
    outputLegendLabel.setBounds (legendStartX + (legendWidth + legendGap) * 2, lowerTop, legendWidth, 20);

    history.setBounds (historyX, lowerTop + 24, historyW, std::max (150, lowerBottom - lowerTop - 28));

    const int meterGap = 34;
    const int meterW = (w - 72 - meterGap) / 2;
    inputMeter.setBounds (36, meterY, meterW, meterHeight);
    outputMeter.setBounds (36 + meterW + meterGap, meterY, meterW, meterHeight);
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
    g.setColour (juce::Colour (0xff0b1115));
    g.fillRoundedRectangle (r, 6.0f);
    g.setColour (border.withAlpha (0.7f));
    g.drawRoundedRectangle (r.reduced (0.5f), 6.0f, 1.0f);

    auto plot = r.reduced (42.0f, 18.0f);
    plot.removeFromRight (36.0f);

    g.setFont (juce::FontOptions (9.5f));
    for (int i = 0; i <= 4; ++i)
    {
        const auto y = plot.getY() + plot.getHeight() * static_cast<float> (i) / 4.0f;
        g.setColour (juce::Colour (0xff26343e));
        g.drawHorizontalLine (static_cast<int> (y), plot.getX(), plot.getRight());

        const auto levelDb = 0.0f - 15.0f * static_cast<float> (i);
        g.setColour (muted);
        g.drawText (juce::String (levelDb, 0), 2, static_cast<int> (y - 7), 36, 14, juce::Justification::centredRight);

        const auto riderDb = 12.0f - 6.0f * static_cast<float> (i);
        g.drawText ((riderDb > 0.0f ? "+" : "") + juce::String (riderDb, 0),
                    static_cast<int> (plot.getRight() + 4), static_cast<int> (y - 7), 32, 14,
                    juce::Justification::centredLeft);
    }

    for (int i = 0; i <= 4; ++i)
    {
        const auto x = plot.getX() + plot.getWidth() * static_cast<float> (i) / 4.0f;
        g.setColour (juce::Colour (0xff1b2831));
        g.drawVerticalLine (static_cast<int> (x), plot.getY(), plot.getBottom());
    }

    const auto points = processor.getHistory().copyLatest (300);
    if (points.size() >= 2)
    {
        auto levelY = [&] (float db)
        {
            const auto norm = juce::jlimit (0.0f, 1.0f, (db + 60.0f) / 60.0f);
            return plot.getBottom() - norm * plot.getHeight();
        };
        auto riderY = [&] (float db)
        {
            const auto norm = juce::jlimit (0.0f, 1.0f, (db + 12.0f) / 24.0f);
            return plot.getBottom() - norm * plot.getHeight();
        };

        auto makePath = [&] (auto valueToY)
        {
            juce::Path p;
            for (std::size_t i = 0; i < points.size(); ++i)
            {
                const auto x = plot.getX() + plot.getWidth() * static_cast<float> (i) / static_cast<float> (points.size() - 1);
                const auto y = valueToY (points[i]);
                if (i == 0) p.startNewSubPath (x, y); else p.lineTo (x, y);
            }
            return p;
        };

        auto inputPath = makePath ([&] (const SantosHistoryPoint& p) { return levelY (p.inputDb); });
        auto outputPath = makePath ([&] (const SantosHistoryPoint& p) { return levelY (p.outputDb); });
        auto riderPath = makePath ([&] (const SantosHistoryPoint& p) { return riderY (p.riderDb); });

        g.setColour (cyan.withAlpha (0.86f));
        g.strokePath (inputPath, juce::PathStrokeType (2.0f));
        g.setColour (green.withAlpha (0.92f));
        g.strokePath (outputPath, juce::PathStrokeType (2.0f));
        g.setColour (yellow);
        g.strokePath (riderPath, juce::PathStrokeType (2.2f));
    }

    g.setColour (muted);
    g.setFont (juce::FontOptions (9.0f));
    g.drawText ("LEVEL dBFS", 4, 2, 70, 14, juce::Justification::centredLeft);
    g.drawText ("RIDER dB", getWidth() - 72, 2, 68, 14, juce::Justification::centredRight);

    if (processor.hasHostTransport() && ! processor.getTransportPlaying())
    {
        g.setColour (juce::Colour (0xbb0b1115));
        g.fillRoundedRectangle (plot, 5.0f);
        g.setColour (text);
        g.setFont (juce::FontOptions (14.0f));
        g.drawText ("PAUSED", plot.toNearestInt(), juce::Justification::centred);
    }
}

void SantosLevelerAudioProcessorEditor::MeterComponent::paint (juce::Graphics& g)
{
    const auto db = source == Source::input ? processor.getInputMeterDb() : processor.getOutputMeterDb();
    auto r = getLocalBounds().toFloat();

    g.setColour (panel2);
    g.fillRoundedRectangle (r, 6.0f);
    g.setColour (border.withAlpha (0.55f));
    g.drawRoundedRectangle (r.reduced (0.5f), 6.0f, 1.0f);

    g.setColour (text);
    g.setFont (juce::FontOptions (10.5f));
    g.drawText (label, 12, 6, 90, 18, juce::Justification::centredLeft);

    g.setColour (colour);
    g.setFont (juce::FontOptions (12.0f));
    g.drawText (juce::String (db, 1) + " dBFS", getWidth() - 110, 6, 98, 18, juce::Justification::centredRight);

    auto bar = juce::Rectangle<float> (12.0f, 31.0f, r.getWidth() - 24.0f, 14.0f);
    g.setColour (juce::Colour (0xff26343e));
    g.fillRoundedRectangle (bar, 4.0f);

    const auto norm = juce::jlimit (0.0f, 1.0f, (db + 60.0f) / 60.0f);
    auto fill = bar;
    fill.setWidth (bar.getWidth() * norm);
    g.setColour (colour);
    g.fillRoundedRectangle (fill, 4.0f);
}
