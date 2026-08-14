#include "PluginEditor.h"

namespace
{
const auto bg        = juce::Colour (0xff090d12);
const auto panel     = juce::Colour (0xff141c24);
const auto panel2    = juce::Colour (0xff0e151c);
const auto border    = juce::Colour (0xff40515e);
const auto text      = juce::Colour (0xffedf4f7);
const auto muted     = juce::Colour (0xff8799a5);
const auto cyan      = juce::Colour (0xff55c5e6);
const auto yellow    = juce::Colour (0xffe8d14b);
const auto green     = juce::Colour (0xff72d081);
const auto orange    = juce::Colour (0xffee8a73);
const auto magenta   = juce::Colour (0xffd57be8);
const auto violet    = juce::Colour (0xff8e7cff);

void styleLabel (juce::Label& label, float size, juce::Colour colour,
                 int justification = juce::Justification::centred)
{
    label.setFont (juce::FontOptions (size));
    label.setColour (juce::Label::textColourId, colour);
    label.setJustificationType (justification);
    label.setInterceptsMouseClicks (false, false);
}

juce::Colour skinAccent (SantosLevelerAudioProcessorEditor::Skin skin)
{
    switch (skin)
    {
        case SantosLevelerAudioProcessorEditor::Skin::classicPro: return cyan;
        case SantosLevelerAudioProcessorEditor::Skin::neon:       return violet;
        case SantosLevelerAudioProcessorEditor::Skin::modular:    return orange;
        case SantosLevelerAudioProcessorEditor::Skin::analysis:   return cyan;
        case SantosLevelerAudioProcessorEditor::Skin::radar:      return magenta;
        case SantosLevelerAudioProcessorEditor::Skin::broadcast:  return green;
    }

    return cyan;
}

juce::Colour skinBase (SantosLevelerAudioProcessorEditor::Skin skin)
{
    switch (skin)
    {
        case SantosLevelerAudioProcessorEditor::Skin::classicPro: return juce::Colour (0xff0b1117);
        case SantosLevelerAudioProcessorEditor::Skin::neon:       return juce::Colour (0xff090816);
        case SantosLevelerAudioProcessorEditor::Skin::modular:    return juce::Colour (0xff151310);
        case SantosLevelerAudioProcessorEditor::Skin::analysis:   return juce::Colour (0xff071116);
        case SantosLevelerAudioProcessorEditor::Skin::radar:      return juce::Colour (0xff090b14);
        case SantosLevelerAudioProcessorEditor::Skin::broadcast:  return juce::Colour (0xff0c1010);
    }

    return bg;
}
}

SantosLevelerAudioProcessorEditor::SantosLookAndFeel::SantosLookAndFeel()
{
    setColour (juce::Slider::textBoxTextColourId, text);
    setColour (juce::Slider::textBoxBackgroundColourId, juce::Colour (0xff0b1117));
    setColour (juce::Slider::textBoxOutlineColourId, juce::Colours::transparentBlack);
    setColour (juce::Slider::rotarySliderFillColourId, cyan);
    setColour (juce::Slider::rotarySliderOutlineColourId, juce::Colour (0xff34434e));
    setColour (juce::Slider::thumbColourId, text);
    setColour (juce::Slider::trackColourId, juce::Colour (0xff24313a));
    setColour (juce::ComboBox::backgroundColourId, juce::Colour (0xff101820));
    setColour (juce::ComboBox::textColourId, text);
    setColour (juce::ComboBox::outlineColourId, border.withAlpha (0.7f));
    setColour (juce::ComboBox::arrowColourId, cyan);
}

void SantosLevelerAudioProcessorEditor::SantosLookAndFeel::drawRotarySlider (
    juce::Graphics& g, int x, int y, int width, int height,
    float sliderPos, float rotaryStartAngle, float rotaryEndAngle, juce::Slider& slider)
{
    auto bounds = juce::Rectangle<float> ((float) x, (float) y, (float) width, (float) height).reduced (7.0f);
    const auto radius = juce::jmax (8.0f, juce::jmin (bounds.getWidth(), bounds.getHeight()) * 0.5f);
    const auto centre = bounds.getCentre();
    const auto accent = skinAccent (skin);
    const auto angle = rotaryStartAngle + sliderPos * (rotaryEndAngle - rotaryStartAngle);

    if (skin == Skin::neon || skin == Skin::radar)
    {
        g.setColour (accent.withAlpha (0.10f));
        g.fillEllipse (juce::Rectangle<float> (radius * 2.0f, radius * 2.0f).withCentre (centre));
        g.setColour (accent.withAlpha (0.16f));
        g.fillEllipse (juce::Rectangle<float> (radius * 1.65f, radius * 1.65f).withCentre (centre));
    }

    if (skin == Skin::modular)
    {
        g.setColour (juce::Colour (0xff25231f));
        g.fillEllipse (juce::Rectangle<float> (radius * 1.65f, radius * 1.65f).withCentre (centre));
        g.setColour (juce::Colour (0xff0d0c0a));
        g.drawEllipse (juce::Rectangle<float> (radius * 1.65f, radius * 1.65f).withCentre (centre), 2.0f);
    }
    else
    {
        g.setColour (juce::Colour (0xff172129));
        g.fillEllipse (juce::Rectangle<float> (radius * 1.55f, radius * 1.55f).withCentre (centre));
    }

    const auto arcRadius = radius * 0.78f;
    const auto lineW = juce::jmax (3.0f, radius * 0.10f);

    juce::Path baseArc;
    baseArc.addCentredArc (centre.x, centre.y, arcRadius, arcRadius, 0.0f,
                           rotaryStartAngle, rotaryEndAngle, true);
    g.setColour (juce::Colour (0xff33424d));
    g.strokePath (baseArc, juce::PathStrokeType (lineW, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

    juce::Path valueArc;
    valueArc.addCentredArc (centre.x, centre.y, arcRadius, arcRadius, 0.0f,
                            rotaryStartAngle, angle, true);
    g.setColour (accent);
    g.strokePath (valueArc, juce::PathStrokeType (lineW, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

    if (skin == Skin::broadcast)
    {
        for (int i = 0; i < 11; ++i)
        {
            const auto a = rotaryStartAngle + (rotaryEndAngle - rotaryStartAngle) * (float) i / 10.0f;
            const auto p1 = centre + juce::Point<float> (std::sin (a), -std::cos (a)) * (radius * 0.92f);
            const auto p2 = centre + juce::Point<float> (std::sin (a), -std::cos (a)) * (radius * 1.02f);
            g.setColour (muted.withAlpha (0.8f));
            g.drawLine ({ p1, p2 }, 1.0f);
        }
    }

    juce::Path pointer;
    pointer.addRoundedRectangle (-1.4f, -radius * 0.55f, 2.8f, radius * 0.38f, 1.4f);
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

    const auto cx = (float) x + (float) width * 0.5f;
    const auto top = (float) y + 8.0f;
    const auto bottom = (float) y + (float) height - 8.0f;
    const auto accent = slider.findColour (juce::Slider::thumbColourId);
    const auto trackW = skin == Skin::broadcast ? 18.0f : 11.0f;

    g.setColour (juce::Colour (0xff25313a));
    g.fillRoundedRectangle (cx - trackW * 0.5f, top, trackW, bottom - top, 4.0f);

    float zeroPos = bottom;
    if (slider.getMinimum() < 0.0 && slider.getMaximum() > 0.0)
    {
        const auto prop = (float) ((0.0 - slider.getMinimum()) / (slider.getMaximum() - slider.getMinimum()));
        zeroPos = bottom - prop * (bottom - top);
    }

    const auto y1 = juce::jmin (sliderPos, zeroPos);
    const auto y2 = juce::jmax (sliderPos, zeroPos);
    g.setColour (accent.withAlpha (0.92f));
    g.fillRoundedRectangle (cx - trackW * 0.5f, y1, trackW, juce::jmax (2.0f, y2 - y1), 4.0f);

    if (skin == Skin::neon)
    {
        g.setColour (accent.withAlpha (0.12f));
        g.fillRoundedRectangle (cx - 18.0f, y1, 36.0f, juce::jmax (3.0f, y2 - y1), 10.0f);
    }

    const auto thumbW = skin == Skin::modular ? 62.0f : 52.0f;
    g.setColour (accent);
    g.fillRoundedRectangle (cx - thumbW * 0.5f, sliderPos - 7.0f, thumbW, 14.0f, 4.0f);
    g.setColour (juce::Colours::white.withAlpha (0.38f));
    g.drawHorizontalLine ((int) sliderPos, cx - thumbW * 0.32f, cx + thumbW * 0.32f);
}

SantosLevelerAudioProcessorEditor::SantosLevelerAudioProcessorEditor (SantosLevelerAudioProcessor& p)
    : AudioProcessorEditor (&p), processor (p), history (p),
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
    setSize (1100, 740);
    setResizable (true, true);
    setResizeLimits (820, 580, 1600, 1100);

    titleLabel.setText ("SANTOS LEVELER", juce::dontSendNotification);
    styleLabel (titleLabel, 25.0f, text, juce::Justification::centredLeft);
    addAndMakeVisible (titleLabel);

    subtitleLabel.setText ("VOICE AUTO LEVEL RIDER", juce::dontSendNotification);
    styleLabel (subtitleLabel, 10.0f, muted, juce::Justification::centredLeft);
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

    historyLabel.setText ("LIVE RESPONSE", juce::dontSendNotification);
    styleLabel (historyLabel, 11.0f, text, juce::Justification::centredLeft);
    addAndMakeVisible (historyLabel);

    inputLegendLabel.setText ("INPUT", juce::dontSendNotification);
    riderLegendLabel.setText ("RIDER", juce::dontSendNotification);
    peakLegendLabel.setText ("PEAK", juce::dontSendNotification);
    outputLegendLabel.setText ("OUTPUT", juce::dontSendNotification);
    styleLabel (inputLegendLabel, 9.5f, cyan);
    styleLabel (riderLegendLabel, 9.5f, yellow);
    styleLabel (peakLegendLabel, 9.5f, magenta);
    styleLabel (outputLegendLabel, 9.5f, green);
    addAndMakeVisible (inputLegendLabel);
    addAndMakeVisible (riderLegendLabel);
    addAndMakeVisible (peakLegendLabel);
    addAndMakeVisible (outputLegendLabel);

    skinLabel.setText ("SKIN", juce::dontSendNotification);
    motionLabel.setText ("MOTION", juce::dontSendNotification);
    styleLabel (skinLabel, 9.0f, muted, juce::Justification::centredRight);
    styleLabel (motionLabel, 9.0f, muted, juce::Justification::centredRight);
    addAndMakeVisible (skinLabel);
    addAndMakeVisible (motionLabel);

    skinSelector.addItem ("CLASSIC PRO", 1);
    skinSelector.addItem ("NEON", 2);
    skinSelector.addItem ("MODULAR", 3);
    skinSelector.addItem ("ANALYSIS", 4);
    skinSelector.addItem ("RADAR", 5);
    skinSelector.addItem ("BROADCAST", 6);
    addAndMakeVisible (skinSelector);

    motionSelector.addItem ("FULL", 1);
    motionSelector.addItem ("REDUCED", 2);
    motionSelector.addItem ("OFF", 3);
    addAndMakeVisible (motionSelector);

    addAndMakeVisible (history);
    addAndMakeVisible (inputMeter);
    addAndMakeVisible (outputMeter);

    const auto savedSkin = juce::jlimit (0, 5, (int) processor.apvts.state.getProperty ("uiSkin", 0));
    const auto savedMotion = juce::jlimit (0, 2, (int) processor.apvts.state.getProperty ("uiMotion", 0));
    currentSkin = static_cast<Skin> (savedSkin);
    pendingSkin = currentSkin;
    motion = static_cast<Motion> (savedMotion);
    skinSelector.setSelectedId (savedSkin + 1, juce::dontSendNotification);
    motionSelector.setSelectedId (savedMotion + 1, juce::dontSendNotification);

    skinSelector.onChange = [this]
    {
        const auto selected = static_cast<Skin> (juce::jlimit (0, 5, skinSelector.getSelectedId() - 1));
        beginSkinChange (selected);
    };

    motionSelector.onChange = [this]
    {
        motion = static_cast<Motion> (juce::jlimit (0, 2, motionSelector.getSelectedId() - 1));
        processor.apvts.state.setProperty ("uiMotion", (int) motion, nullptr);
    };

    applySkin (currentSkin);
    startTimerHz (60);
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
    styleLabel (label, 10.0f, text);
    addAndMakeVisible (label);
}

void SantosLevelerAudioProcessorEditor::configureFader (juce::Slider& slider, juce::Label& label,
                                                         const juce::String& name, const juce::String& suffix,
                                                         int decimals, juce::Colour accent)
{
    slider.setSliderStyle (juce::Slider::LinearVertical);
    slider.setTextBoxStyle (juce::Slider::TextBoxBelow, false, 78, 22);
    slider.setTextValueSuffix (suffix);
    slider.setNumDecimalPlacesToDisplay (decimals);
    slider.setColour (juce::Slider::thumbColourId, accent);
    addAndMakeVisible (slider);
    label.setText (name, juce::dontSendNotification);
    styleLabel (label, 10.0f, text);
    addAndMakeVisible (label);
}

void SantosLevelerAudioProcessorEditor::beginSkinChange (Skin newSkin)
{
    if (newSkin == currentSkin || transitioning)
        return;

    if (motion == Motion::off)
    {
        applySkin (newSkin);
        return;
    }

    pendingSkin = newSkin;
    transitioning = true;
    pendingApplied = false;
    transitionProgress = 0.0f;
}

void SantosLevelerAudioProcessorEditor::applySkin (Skin newSkin)
{
    currentSkin = newSkin;
    pendingSkin = newSkin;
    lookAndFeel.setSkin (newSkin);
    history.setSkin (newSkin);
    inputMeter.setSkin (newSkin);
    outputMeter.setSkin (newSkin);
    processor.apvts.state.setProperty ("uiSkin", (int) newSkin, nullptr);

    const auto accent = skinAccent (newSkin);
    lookAndFeel.setColour (juce::ComboBox::arrowColourId, accent);

    titleLabel.setColour (juce::Label::textColourId, text);
    historyLabel.setColour (juce::Label::textColourId, accent.brighter (0.2f));
    resized();
    repaint();
}

void SantosLevelerAudioProcessorEditor::setUiAlpha (float alpha)
{
    juce::Component* fadeComponents[] =
    {
        &targetKnob, &gateKnob, &speedKnob, &detectKnob, &lookaheadKnob, &holdKnob, &releaseKnob, &peakThresholdKnob,
        &rangeDownSlider, &rangeUpSlider, &outputSlider,
        &targetLabel, &gateLabel, &speedLabel, &detectLabel, &lookaheadLabel, &holdLabel, &releaseLabel, &peakThresholdLabel,
        &rangeDownLabel, &rangeUpLabel, &outputLabel,
        &historyLabel, &inputLegendLabel, &riderLegendLabel, &peakLegendLabel, &outputLegendLabel,
        &history, &inputMeter, &outputMeter
    };

    for (auto* c : fadeComponents)
        c->setAlpha (alpha);
}

void SantosLevelerAudioProcessorEditor::timerCallback()
{
    animationPhase += 0.016f;
    if (animationPhase > juce::MathConstants<float>::twoPi)
        animationPhase -= juce::MathConstants<float>::twoPi;

    if (transitioning)
    {
        const auto step = motion == Motion::reduced ? 0.11f : 0.055f;
        transitionProgress = juce::jmin (1.0f, transitionProgress + step);

        if (! pendingApplied && transitionProgress >= 0.5f)
        {
            pendingApplied = true;
            applySkin (pendingSkin);
        }

        const auto alpha = transitionProgress < 0.5f
            ? 1.0f - transitionProgress * 2.0f
            : (transitionProgress - 0.5f) * 2.0f;

        setUiAlpha (juce::jlimit (0.0f, 1.0f, alpha));

        if (transitionProgress >= 1.0f)
        {
            transitioning = false;
            setUiAlpha (1.0f);
        }
    }

    history.setPhase (animationPhase);
    repaint();
    history.repaint();
    inputMeter.repaint();
    outputMeter.repaint();
}

void SantosLevelerAudioProcessorEditor::paint (juce::Graphics& g)
{
    auto area = getLocalBounds().toFloat();
    drawSkinBackground (g, area);
    drawSkinCandy (g, area);

    const auto active = processor.getRiderActive();
    const auto statusX = (float) getWidth() - 48.0f;
    g.setColour (active ? green : muted.withAlpha (0.45f));
    g.fillEllipse (statusX, 30.0f, 9.0f, 9.0f);
    g.setFont (juce::FontOptions (9.0f));
    g.setColour (muted);
    g.drawText (active ? "RIDE" : "IDLE", getWidth() - 102, 24, 46, 20, juce::Justification::centredRight);

    if (transitioning)
    {
        const auto glow = 1.0f - std::abs (transitionProgress - 0.5f) * 2.0f;
        g.setColour (skinAccent (pendingSkin).withAlpha (0.10f * glow));
        g.fillAll();
    }
}

void SantosLevelerAudioProcessorEditor::drawSkinBackground (juce::Graphics& g, juce::Rectangle<float> area)
{
    const auto base = skinBase (currentSkin);
    g.fillAll (base);

    if (currentSkin == Skin::neon || currentSkin == Skin::radar)
    {
        juce::ColourGradient grad (base.brighter (0.04f), area.getTopLeft(),
                                   skinAccent (currentSkin).withAlpha (0.10f), area.getBottomRight(), false);
        g.setGradientFill (grad);
        g.fillRect (area);
    }

    auto frame = area.reduced (16.0f);
    const auto corner = currentSkin == Skin::broadcast ? 3.0f : 12.0f;
    g.setColour (juce::Colour (0xff131b22).withAlpha (0.92f));
    g.fillRoundedRectangle (frame, corner);
    g.setColour (skinAccent (currentSkin).withAlpha (0.26f));
    g.drawRoundedRectangle (frame, corner, 1.0f);

    if (currentSkin == Skin::modular)
    {
        g.setColour (orange.withAlpha (0.06f));
        for (int y = 90; y < getHeight() - 30; y += 48)
            g.drawHorizontalLine (y, 26.0f, (float) getWidth() - 26.0f);
    }

    if (currentSkin == Skin::analysis)
    {
        g.setColour (cyan.withAlpha (0.035f));
        for (int x = 24; x < getWidth(); x += 32)
            g.drawVerticalLine (x, 18.0f, (float) getHeight() - 18.0f);
        for (int y = 18; y < getHeight(); y += 32)
            g.drawHorizontalLine (y, 18.0f, (float) getWidth() - 18.0f);
    }
}

void SantosLevelerAudioProcessorEditor::drawSkinCandy (juce::Graphics& g, juce::Rectangle<float>)
{
    const auto rider = processor.getRiderDb();
    const auto input = processor.getInputMeterDb();
    const auto pulse = 0.5f + 0.5f * std::sin (animationPhase * 1.7f);

    if (motion == Motion::off)
        return;

    switch (currentSkin)
    {
        case Skin::classicPro:
        {
            const auto c = juce::Point<float> ((float) getWidth() * 0.50f, 47.0f);
            const auto r = 18.0f + juce::jlimit (0.0f, 10.0f, std::abs (rider));
            g.setColour (cyan.withAlpha (0.06f + 0.04f * pulse));
            g.drawEllipse (juce::Rectangle<float> (r * 2.0f, r * 2.0f).withCentre (c), 2.0f);
            break;
        }
        case Skin::neon:
        {
            const auto x = juce::jmap (std::fmod (animationPhase, 3.0f), 0.0f, 3.0f, 30.0f, (float) getWidth() - 30.0f);
            juce::ColourGradient beam (violet.withAlpha (0.0f), x - 45.0f, 0.0f,
                                       violet.withAlpha (0.12f), x, 0.0f, false);
            beam.addColour (1.0, violet.withAlpha (0.0f));
            g.setGradientFill (beam);
            g.fillRect (juce::Rectangle<float> (x - 45.0f, 18.0f, 90.0f, (float) getHeight() - 36.0f));
            break;
        }
        case Skin::modular:
        {
            const auto needleX = juce::jmap (juce::jlimit (-12.0f, 12.0f, rider), -12.0f, 12.0f,
                                             (float) getWidth() * 0.42f, (float) getWidth() * 0.58f);
            g.setColour (orange.withAlpha (0.7f));
            g.drawLine (needleX, 78.0f, (float) getWidth() * 0.5f, 52.0f, 2.0f);
            g.setColour (text.withAlpha (0.6f));
            g.fillEllipse ((float) getWidth() * 0.5f - 3.0f, 49.0f, 6.0f, 6.0f);
            break;
        }
        case Skin::analysis:
        {
            const auto strength = juce::jlimit (0.0f, 1.0f, (input + 60.0f) / 60.0f);
            const auto x = 24.0f + std::fmod (animationPhase * 110.0f, juce::jmax (1.0f, (float) getWidth() - 48.0f));
            g.setColour (cyan.withAlpha (0.06f + 0.12f * strength));
            g.drawVerticalLine ((int) x, 74.0f, (float) getHeight() - 28.0f);
            break;
        }
        case Skin::radar:
        {
            const auto centre = juce::Point<float> ((float) getWidth() * 0.5f, (float) getHeight() * 0.53f);
            const auto radius = juce::jmin ((float) getWidth(), (float) getHeight()) * 0.20f;
            const auto angle = animationPhase * 0.65f;
            const auto end = centre + juce::Point<float> (std::cos (angle), std::sin (angle)) * radius;
            g.setColour (magenta.withAlpha (0.12f));
            g.drawEllipse (juce::Rectangle<float> (radius * 2.0f, radius * 2.0f).withCentre (centre), 1.5f);
            g.setColour (cyan.withAlpha (0.22f));
            g.drawLine ({ centre, end }, 1.5f);
            break;
        }
        case Skin::broadcast:
        {
            const auto on = processor.getRiderActive();
            g.setColour ((on ? green : orange).withAlpha (0.12f + 0.06f * pulse));
            g.fillRoundedRectangle ((float) getWidth() * 0.44f, 22.0f, (float) getWidth() * 0.12f, 28.0f, 3.0f);
            g.setColour (on ? green : orange);
            g.setFont (juce::FontOptions (11.0f));
            g.drawText (on ? "AUTO RIDING" : "STANDBY", (int) ((float) getWidth() * 0.44f), 22,
                        (int) ((float) getWidth() * 0.12f), 28, juce::Justification::centred);
            break;
        }
    }
}

void SantosLevelerAudioProcessorEditor::resized()
{
    const auto w = getWidth();
    titleLabel.setBounds (32, 24, juce::jmax (190, w / 3), 30);
    subtitleLabel.setBounds (34, 52, 230, 16);

    skinLabel.setBounds (w - 390, 24, 42, 24);
    skinSelector.setBounds (w - 342, 22, 132, 28);
    motionLabel.setBounds (w - 206, 24, 54, 24);
    motionSelector.setBounds (w - 148, 22, 82, 28);

    auto content = getLocalBounds().reduced (28);
    content.removeFromTop (58);

    switch (currentSkin)
    {
        case Skin::classicPro: layoutClassic (content); break;
        case Skin::neon:       layoutNeon (content); break;
        case Skin::modular:    layoutModular (content); break;
        case Skin::analysis:   layoutAnalysis (content); break;
        case Skin::radar:      layoutRadar (content); break;
        case Skin::broadcast:  layoutBroadcast (content); break;
    }
}

void SantosLevelerAudioProcessorEditor::layoutControlGrid (juce::Rectangle<int> area, int columns)
{
    juce::Slider* sliders[] = { &gateKnob, &targetKnob, &speedKnob, &detectKnob, &lookaheadKnob, &holdKnob, &releaseKnob, &peakThresholdKnob };
    juce::Label* labels[] = { &gateLabel, &targetLabel, &speedLabel, &detectLabel, &lookaheadLabel, &holdLabel, &releaseLabel, &peakThresholdLabel };

    columns = juce::jmax (1, columns);
    const int rows = (8 + columns - 1) / columns;
    const int cellW = area.getWidth() / columns;
    const int cellH = area.getHeight() / rows;

    for (int i = 0; i < 8; ++i)
    {
        const int col = i % columns;
        const int row = i / columns;
        auto cell = juce::Rectangle<int> (area.getX() + col * cellW, area.getY() + row * cellH, cellW, cellH).reduced (4);
        labels[i]->setBounds (cell.removeFromBottom (18));
        sliders[i]->setBounds (cell);
    }
}

void SantosLevelerAudioProcessorEditor::layoutClassic (juce::Rectangle<int> c)
{
    auto top = c.removeFromTop ((int) (c.getHeight() * 0.30f));
    layoutControlGrid (top, 8);

    c.removeFromTop (8);
    auto meters = c.removeFromBottom (58);
    inputMeter.setBounds (meters.removeFromLeft (meters.getWidth() / 2).reduced (4));
    outputMeter.setBounds (meters.reduced (4));

    auto left = c.removeFromLeft (150);
    auto right = c.removeFromRight (110);
    rangeDownLabel.setBounds (left.removeFromTop (22));
    rangeDownSlider.setBounds (left.reduced (28, 4));
    outputLabel.setBounds (right.removeFromTop (22));
    outputSlider.setBounds (right.reduced (10, 4));
    auto left2 = c.removeFromLeft (130);
    rangeUpLabel.setBounds (left2.removeFromTop (22));
    rangeUpSlider.setBounds (left2.reduced (18, 4));

    historyLabel.setBounds (c.getX() + 6, c.getY(), 140, 20);
    auto legend = juce::Rectangle<int> (c.getRight() - 260, c.getY(), 260, 20);
    inputLegendLabel.setBounds (legend.removeFromLeft (65));
    riderLegendLabel.setBounds (legend.removeFromLeft (65));
    peakLegendLabel.setBounds (legend.removeFromLeft (65));
    outputLegendLabel.setBounds (legend);
    history.setBounds (c.withTrimmedTop (24).reduced (4));
}

void SantosLevelerAudioProcessorEditor::layoutNeon (juce::Rectangle<int> c)
{
    auto visual = c.removeFromTop ((int) (c.getHeight() * 0.55f));
    historyLabel.setBounds (visual.getX() + 8, visual.getY(), 130, 20);
    auto legend = juce::Rectangle<int> (visual.getRight() - 250, visual.getY(), 250, 20);
    inputLegendLabel.setBounds (legend.removeFromLeft (62));
    riderLegendLabel.setBounds (legend.removeFromLeft (62));
    peakLegendLabel.setBounds (legend.removeFromLeft (62));
    outputLegendLabel.setBounds (legend);
    history.setBounds (visual.withTrimmedTop (24).reduced (2));

    c.removeFromTop (8);
    auto side = c.removeFromRight (260);
    layoutControlGrid (c.reduced (4), 4);

    auto faderW = side.getWidth() / 3;
    auto a = side.removeFromLeft (faderW).reduced (4);
    auto b = side.removeFromLeft (faderW).reduced (4);
    auto d = side.reduced (4);
    rangeDownLabel.setBounds (a.removeFromTop (20)); rangeDownSlider.setBounds (a);
    rangeUpLabel.setBounds (b.removeFromTop (20)); rangeUpSlider.setBounds (b);
    outputLabel.setBounds (d.removeFromTop (20)); outputSlider.setBounds (d);

    auto meterStrip = visual.removeFromBottom (0);
    juce::ignoreUnused (meterStrip);
    inputMeter.setBounds (getWidth() - 1, getHeight() - 1, 1, 1);
    outputMeter.setBounds (getWidth() - 1, getHeight() - 1, 1, 1);
}

void SantosLevelerAudioProcessorEditor::layoutModular (juce::Rectangle<int> c)
{
    const int gap = 10;
    auto top = c.removeFromTop ((int) (c.getHeight() * 0.46f));
    auto detectModule = top.removeFromLeft (top.getWidth() / 3).reduced (4);
    auto riderModule = top.removeFromLeft (top.getWidth() / 2).reduced (4);
    auto peakModule = top.reduced (4);

    layoutControlGrid (detectModule, 2);
    layoutControlGrid (riderModule, 2);
    peakThresholdLabel.setBounds (peakModule.getX(), peakModule.getY(), peakModule.getWidth(), 20);
    peakThresholdKnob.setBounds (peakModule.withTrimmedTop (20));

    c.removeFromTop (gap);
    auto bottom = c;
    auto faders = bottom.removeFromLeft (260);
    const int faderW = faders.getWidth() / 3;
    auto a = faders.removeFromLeft (faderW).reduced (4);
    auto b = faders.removeFromLeft (faderW).reduced (4);
    auto d = faders.reduced (4);
    rangeDownLabel.setBounds (a.removeFromTop (20)); rangeDownSlider.setBounds (a);
    rangeUpLabel.setBounds (b.removeFromTop (20)); rangeUpSlider.setBounds (b);
    outputLabel.setBounds (d.removeFromTop (20)); outputSlider.setBounds (d);

    historyLabel.setBounds (bottom.getX() + 6, bottom.getY(), 120, 20);
    history.setBounds (bottom.withTrimmedTop (24).reduced (4));
    inputMeter.setBounds (getWidth() - 1, getHeight() - 1, 1, 1);
    outputMeter.setBounds (getWidth() - 1, getHeight() - 1, 1, 1);
}

void SantosLevelerAudioProcessorEditor::layoutAnalysis (juce::Rectangle<int> c)
{
    auto left = c.removeFromLeft ((int) (c.getWidth() * 0.23f));
    layoutControlGrid (left.reduced (4), 2);

    auto bottom = c.removeFromBottom (150);
    const int faderW = 95;
    auto a = bottom.removeFromLeft (faderW).reduced (4);
    auto b = bottom.removeFromLeft (faderW).reduced (4);
    auto d = bottom.removeFromLeft (faderW).reduced (4);
    rangeDownLabel.setBounds (a.removeFromTop (20)); rangeDownSlider.setBounds (a);
    rangeUpLabel.setBounds (b.removeFromTop (20)); rangeUpSlider.setBounds (b);
    outputLabel.setBounds (d.removeFromTop (20)); outputSlider.setBounds (d);
    inputMeter.setBounds (bottom.removeFromLeft (bottom.getWidth() / 2).reduced (5));
    outputMeter.setBounds (bottom.reduced (5));

    historyLabel.setBounds (c.getX() + 8, c.getY(), 130, 20);
    history.setBounds (c.withTrimmedTop (24).reduced (4));
}

void SantosLevelerAudioProcessorEditor::layoutRadar (juce::Rectangle<int> c)
{
    const int sideW = (int) (c.getWidth() * 0.25f);
    auto left = c.removeFromLeft (sideW);
    auto right = c.removeFromRight (sideW);
    layoutControlGrid (left.reduced (4), 2);

    auto rfaders = right.removeFromBottom ((int) (right.getHeight() * 0.50f));
    const int fw = rfaders.getWidth() / 3;
    auto a = rfaders.removeFromLeft (fw).reduced (3);
    auto b = rfaders.removeFromLeft (fw).reduced (3);
    auto d = rfaders.reduced (3);
    rangeDownLabel.setBounds (a.removeFromTop (20)); rangeDownSlider.setBounds (a);
    rangeUpLabel.setBounds (b.removeFromTop (20)); rangeUpSlider.setBounds (b);
    outputLabel.setBounds (d.removeFromTop (20)); outputSlider.setBounds (d);

    inputMeter.setBounds (right.removeFromTop (right.getHeight() / 2).reduced (6));
    outputMeter.setBounds (right.reduced (6));

    historyLabel.setBounds (c.getX() + 8, c.getY(), 130, 20);
    history.setBounds (c.withTrimmedTop (24).reduced (4));
}

void SantosLevelerAudioProcessorEditor::layoutBroadcast (juce::Rectangle<int> c)
{
    auto meters = c.removeFromLeft ((int) (c.getWidth() * 0.20f));
    inputMeter.setBounds (meters.removeFromLeft (meters.getWidth() / 2).reduced (5));
    outputMeter.setBounds (meters.reduced (5));

    auto right = c.removeFromRight ((int) (c.getWidth() * 0.30f));
    layoutControlGrid (right.reduced (4), 2);

    auto faders = c.removeFromBottom ((int) (c.getHeight() * 0.43f));
    const int fw = faders.getWidth() / 3;
    auto a = faders.removeFromLeft (fw).reduced (8);
    auto b = faders.removeFromLeft (fw).reduced (8);
    auto d = faders.reduced (8);
    rangeDownLabel.setBounds (a.removeFromTop (22)); rangeDownSlider.setBounds (a);
    rangeUpLabel.setBounds (b.removeFromTop (22)); rangeUpSlider.setBounds (b);
    outputLabel.setBounds (d.removeFromTop (22)); outputSlider.setBounds (d);

    historyLabel.setBounds (c.getX() + 8, c.getY(), 130, 20);
    history.setBounds (c.withTrimmedTop (24).reduced (4));
}

void SantosLevelerAudioProcessorEditor::HistoryComponent::paint (juce::Graphics& g)
{
    auto r = getLocalBounds().toFloat();
    const auto accent = skinAccent (skin);
    g.setColour (skin == Skin::modular ? juce::Colour (0xff1c1914) : juce::Colour (0xff070c11));
    g.fillRoundedRectangle (r, skin == Skin::broadcast ? 3.0f : 8.0f);
    g.setColour (accent.withAlpha (0.26f));
    g.drawRoundedRectangle (r.reduced (0.5f), skin == Skin::broadcast ? 3.0f : 8.0f, 1.0f);

    auto plot = r.reduced (skin == Skin::radar ? 28.0f : 38.0f, 18.0f);
    if (plot.getWidth() < 40.0f || plot.getHeight() < 40.0f)
        return;

    g.setFont (juce::FontOptions (8.5f));
    for (int i = 0; i <= 4; ++i)
    {
        const auto y = plot.getY() + plot.getHeight() * (float) i / 4.0f;
        g.setColour (accent.withAlpha (0.07f));
        g.drawHorizontalLine ((int) y, plot.getX(), plot.getRight());
        if (skin != Skin::radar)
        {
            g.setColour (muted.withAlpha (0.65f));
            g.drawText (juce::String (0 - i * 15), 2, (int) y - 7, 30, 14, juce::Justification::centredRight);
        }
    }

    const auto points = processor.getHistory().copyLatest (300);
    if (points.size() < 2)
        return;

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

    auto inputPath = makePath ([&] (const SantosHistoryPoint& p) { return levelY (p.inputDb); });
    auto outputPath = makePath ([&] (const SantosHistoryPoint& p) { return levelY (p.outputDb); });
    auto riderPath = makePath ([&] (const SantosHistoryPoint& p) { return gainY (p.riderDb); });
    auto peakPath = makePath ([&] (const SantosHistoryPoint& p) { return gainY (p.peakDb); });

    if (skin == Skin::neon)
    {
        g.setColour (cyan.withAlpha (0.10f)); g.strokePath (inputPath, juce::PathStrokeType (7.0f));
        g.setColour (yellow.withAlpha (0.08f)); g.strokePath (riderPath, juce::PathStrokeType (7.0f));
        g.setColour (magenta.withAlpha (0.08f)); g.strokePath (peakPath, juce::PathStrokeType (7.0f));
        g.setColour (green.withAlpha (0.10f)); g.strokePath (outputPath, juce::PathStrokeType (7.0f));
    }

    if (skin == Skin::radar)
    {
        const auto centre = plot.getCentre();
        const auto radius = juce::jmin (plot.getWidth(), plot.getHeight()) * 0.38f;
        for (int ring = 1; ring <= 4; ++ring)
        {
            g.setColour (accent.withAlpha (0.07f));
            const auto rr = radius * (float) ring / 4.0f;
            g.drawEllipse (juce::Rectangle<float> (rr * 2.0f, rr * 2.0f).withCentre (centre), 1.0f);
        }

        const auto& p = points.back();
        const float vals[4] = {
            juce::jlimit (0.0f, 1.0f, (p.inputDb + 60.0f) / 60.0f),
            juce::jlimit (0.0f, 1.0f, (p.riderDb + 12.0f) / 24.0f),
            juce::jlimit (0.0f, 1.0f, (p.peakDb + 12.0f) / 12.0f),
            juce::jlimit (0.0f, 1.0f, (p.outputDb + 60.0f) / 60.0f)
        };
        juce::Path poly;
        for (int i = 0; i < 4; ++i)
        {
            const auto a = -juce::MathConstants<float>::halfPi + juce::MathConstants<float>::twoPi * (float) i / 4.0f;
            const auto pt = centre + juce::Point<float> (std::cos (a), std::sin (a)) * (radius * vals[i]);
            if (i == 0) poly.startNewSubPath (pt); else poly.lineTo (pt);
        }
        poly.closeSubPath();
        g.setColour (accent.withAlpha (0.15f)); g.fillPath (poly);
        g.setColour (accent.withAlpha (0.9f)); g.strokePath (poly, juce::PathStrokeType (2.0f));

        const auto sweep = phase * 0.55f;
        const auto end = centre + juce::Point<float> (std::cos (sweep), std::sin (sweep)) * radius;
        g.setColour (cyan.withAlpha (0.28f)); g.drawLine ({ centre, end }, 1.4f);
        return;
    }

    const auto thick = skin == Skin::broadcast ? 2.6f : 2.0f;
    g.setColour (cyan.withAlpha (0.92f)); g.strokePath (inputPath, juce::PathStrokeType (thick));
    g.setColour (green.withAlpha (0.94f)); g.strokePath (outputPath, juce::PathStrokeType (thick));
    g.setColour (yellow); g.strokePath (riderPath, juce::PathStrokeType (thick + 0.2f));
    g.setColour (magenta.withAlpha (0.95f)); g.strokePath (peakPath, juce::PathStrokeType (thick));

    if (skin == Skin::analysis)
    {
        const auto scanX = plot.getX() + std::fmod (phase * 90.0f, juce::jmax (1.0f, plot.getWidth()));
        g.setColour (cyan.withAlpha (0.18f));
        g.drawVerticalLine ((int) scanX, plot.getY(), plot.getBottom());
    }
}

void SantosLevelerAudioProcessorEditor::MeterComponent::paint (juce::Graphics& g)
{
    const auto db = source == Source::input ? processor.getInputMeterDb() : processor.getOutputMeterDb();
    auto r = getLocalBounds().toFloat();
    const bool vertical = r.getHeight() > r.getWidth() * 1.3f;

    g.setColour (juce::Colour (0xff0b1116));
    g.fillRoundedRectangle (r, skin == Skin::broadcast ? 3.0f : 6.0f);
    g.setColour (colour.withAlpha (0.25f));
    g.drawRoundedRectangle (r.reduced (0.5f), skin == Skin::broadcast ? 3.0f : 6.0f, 1.0f);

    g.setColour (text);
    g.setFont (juce::FontOptions (10.0f));
    g.drawText (label, 8, 5, getWidth() - 16, 16, vertical ? juce::Justification::centred : juce::Justification::centredLeft);

    g.setColour (colour);
    g.setFont (juce::FontOptions (vertical ? 12.0f : 11.0f));
    g.drawText (juce::String (db, 1), 8, vertical ? 23 : 5, getWidth() - 16, 18,
                vertical ? juce::Justification::centred : juce::Justification::centredRight);

    const auto norm = juce::jlimit (0.0f, 1.0f, (db + 60.0f) / 60.0f);
    if (vertical)
    {
        auto bar = juce::Rectangle<float> (r.getX() + r.getWidth() * 0.30f, r.getY() + 48.0f,
                                           r.getWidth() * 0.40f, juce::jmax (10.0f, r.getHeight() - 60.0f));
        g.setColour (juce::Colour (0xff26323a)); g.fillRoundedRectangle (bar, 3.0f);
        auto fill = bar;
        fill.setY (bar.getBottom() - bar.getHeight() * norm);
        fill.setHeight (bar.getHeight() * norm);
        g.setColour (colour); g.fillRoundedRectangle (fill, 3.0f);
        g.setColour (colour.brighter (0.35f));
        g.fillRect (bar.getX() - 3.0f, fill.getY() - 1.0f, bar.getWidth() + 6.0f, 2.0f);
    }
    else
    {
        auto bar = juce::Rectangle<float> (12.0f, r.getHeight() - 21.0f, r.getWidth() - 24.0f, 11.0f);
        g.setColour (juce::Colour (0xff26323a)); g.fillRoundedRectangle (bar, 3.0f);
        auto fill = bar; fill.setWidth (bar.getWidth() * norm);
        g.setColour (colour); g.fillRoundedRectangle (fill, 3.0f);
    }
}
