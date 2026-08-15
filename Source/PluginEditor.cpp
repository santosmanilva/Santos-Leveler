#include "PluginEditor.h"

namespace
{
const auto bg       = juce::Colour (0xff070b10);
const auto panel    = juce::Colour (0xff111922);
const auto panel2   = juce::Colour (0xff0b1219);
const auto border   = juce::Colour (0xff33444f);
const auto text     = juce::Colour (0xfff1f6f8);
const auto muted    = juce::Colour (0xff82929d);
const auto cyan     = juce::Colour (0xff59d4ff);
const auto yellow   = juce::Colour (0xffffdf52);
const auto green    = juce::Colour (0xff6fe08a);
const auto orange   = juce::Colour (0xffff8d72);
const auto magenta  = juce::Colour (0xffff65d8);
const auto violet   = juce::Colour (0xff987cff);
const auto ice      = juce::Colour (0xffc9f3ff);

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
    using Skin = SantosLevelerAudioProcessorEditor::Skin;
    switch (skin)
    {
        case Skin::classicPro: return cyan;
        case Skin::neon:       return violet;
        case Skin::modular:    return orange;
        case Skin::analysis:   return ice;
        case Skin::radar:      return magenta;
        case Skin::broadcast:  return green;
    }
    return cyan;
}

juce::Colour skinBase (SantosLevelerAudioProcessorEditor::Skin skin)
{
    using Skin = SantosLevelerAudioProcessorEditor::Skin;
    switch (skin)
    {
        case Skin::classicPro: return juce::Colour (0xff081018);
        case Skin::neon:       return juce::Colour (0xff080613);
        case Skin::modular:    return juce::Colour (0xff15120f);
        case Skin::analysis:   return juce::Colour (0xff061117);
        case Skin::radar:      return juce::Colour (0xff070914);
        case Skin::broadcast:  return juce::Colour (0xff090d0d);
    }
    return bg;
}

void drawPanel (juce::Graphics& g, juce::Rectangle<float> r, juce::Colour accent,
                float alpha = 0.16f, float radius = 9.0f)
{
    g.setColour (panel.withAlpha (0.92f));
    g.fillRoundedRectangle (r, radius);
    g.setColour (accent.withAlpha (alpha));
    g.drawRoundedRectangle (r.reduced (0.5f), radius, 1.0f);
}

void placeKnobCell (juce::Slider& slider, juce::Label& label, juce::Rectangle<int> cell)
{
    cell = cell.reduced (3);
    label.setBounds (cell.removeFromBottom (18));
    slider.setBounds (cell);
}

void placeFaderCell (juce::Slider& slider, juce::Label& label, juce::Rectangle<int> cell)
{
    cell = cell.reduced (4);
    label.setBounds (cell.removeFromTop (19));
    slider.setBounds (cell);
}
}

SantosLevelerAudioProcessorEditor::SantosLookAndFeel::SantosLookAndFeel()
{
    setColour (juce::Slider::textBoxTextColourId, text);
    setColour (juce::Slider::textBoxBackgroundColourId, juce::Colour (0xff091116));
    setColour (juce::Slider::textBoxOutlineColourId, juce::Colours::transparentBlack);
    setColour (juce::Slider::rotarySliderFillColourId, cyan);
    setColour (juce::Slider::rotarySliderOutlineColourId, juce::Colour (0xff283740));
    setColour (juce::Slider::thumbColourId, text);
    setColour (juce::Slider::trackColourId, juce::Colour (0xff202c34));
    setColour (juce::ComboBox::backgroundColourId, juce::Colour (0xff0c151c));
    setColour (juce::ComboBox::textColourId, text);
    setColour (juce::ComboBox::outlineColourId, border.withAlpha (0.65f));
    setColour (juce::ComboBox::arrowColourId, cyan);
}

void SantosLevelerAudioProcessorEditor::SantosLookAndFeel::drawRotarySlider (
    juce::Graphics& g, int x, int y, int width, int height,
    float sliderPos, float rotaryStartAngle, float rotaryEndAngle, juce::Slider&)
{
    auto b = juce::Rectangle<float> ((float) x, (float) y, (float) width, (float) height).reduced (6.0f);
    const auto radius = juce::jmax (9.0f, juce::jmin (b.getWidth(), b.getHeight()) * 0.47f);
    const auto c = b.getCentre();
    const auto accent = skinAccent (skin);
    const auto angle = rotaryStartAngle + sliderPos * (rotaryEndAngle - rotaryStartAngle);

    if (skin == Skin::neon)
    {
        g.setColour (accent.withAlpha (0.08f));
        g.fillEllipse (juce::Rectangle<float> (radius * 2.2f, radius * 2.2f).withCentre (c));
        g.setColour (cyan.withAlpha (0.06f));
        g.fillEllipse (juce::Rectangle<float> (radius * 1.75f, radius * 1.75f).withCentre (c));
    }
    else if (skin == Skin::modular)
    {
        g.setColour (juce::Colour (0xff2a251f));
        g.fillEllipse (juce::Rectangle<float> (radius * 1.62f, radius * 1.62f).withCentre (c));
        g.setColour (juce::Colour (0xff090908));
        g.drawEllipse (juce::Rectangle<float> (radius * 1.62f, radius * 1.62f).withCentre (c), 2.0f);
    }
    else if (skin == Skin::analysis)
    {
        g.setColour (juce::Colour (0xff0d1b22));
        g.fillEllipse (juce::Rectangle<float> (radius * 1.45f, radius * 1.45f).withCentre (c));
        g.setColour (accent.withAlpha (0.15f));
        g.drawEllipse (juce::Rectangle<float> (radius * 1.45f, radius * 1.45f).withCentre (c), 1.0f);
    }
    else if (skin == Skin::radar)
    {
        g.setColour (magenta.withAlpha (0.07f));
        g.fillEllipse (juce::Rectangle<float> (radius * 2.0f, radius * 2.0f).withCentre (c));
        g.setColour (juce::Colour (0xff111426));
        g.fillEllipse (juce::Rectangle<float> (radius * 1.48f, radius * 1.48f).withCentre (c));
    }
    else if (skin == Skin::broadcast)
    {
        g.setColour (juce::Colour (0xff111819));
        g.fillEllipse (juce::Rectangle<float> (radius * 1.52f, radius * 1.52f).withCentre (c));
        for (int i = 0; i < 13; ++i)
        {
            const auto a = rotaryStartAngle + (rotaryEndAngle - rotaryStartAngle) * (float) i / 12.0f;
            const auto p1 = c + juce::Point<float> (std::sin (a), -std::cos (a)) * (radius * 0.83f);
            const auto p2 = c + juce::Point<float> (std::sin (a), -std::cos (a)) * (radius * 0.98f);
            g.setColour (muted.withAlpha (0.55f));
            g.drawLine ({ p1, p2 }, i % 3 == 0 ? 1.5f : 0.8f);
        }
    }
    else
    {
        g.setColour (juce::Colour (0xff101a21));
        g.fillEllipse (juce::Rectangle<float> (radius * 1.55f, radius * 1.55f).withCentre (c));
    }

    const auto arcR = radius * 0.78f;
    const auto lineW = juce::jmax (3.0f, radius * 0.095f);
    juce::Path baseArc, valueArc;
    baseArc.addCentredArc (c.x, c.y, arcR, arcR, 0.0f, rotaryStartAngle, rotaryEndAngle, true);
    valueArc.addCentredArc (c.x, c.y, arcR, arcR, 0.0f, rotaryStartAngle, angle, true);

    g.setColour (juce::Colour (0xff2c3942));
    g.strokePath (baseArc, juce::PathStrokeType (lineW, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

    if (skin == Skin::neon || skin == Skin::radar)
    {
        g.setColour (accent.withAlpha (0.18f));
        g.strokePath (valueArc, juce::PathStrokeType (lineW * 2.1f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
    }

    g.setColour (accent);
    g.strokePath (valueArc, juce::PathStrokeType (lineW, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

    juce::Path pointer;
    pointer.addRoundedRectangle (-1.3f, -radius * 0.56f, 2.6f, radius * 0.37f, 1.3f);
    pointer.applyTransform (juce::AffineTransform::rotation (angle).translated (c.x, c.y));
    g.setColour (skin == Skin::modular ? juce::Colour (0xffffe3bd) : text);
    g.fillPath (pointer);

    if (skin == Skin::radar)
    {
        g.setColour (accent.withAlpha (0.75f));
        g.fillEllipse (c.x - 2.0f, c.y - 2.0f, 4.0f, 4.0f);
    }
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
    const auto trackW = skin == Skin::broadcast ? 16.0f : skin == Skin::analysis ? 8.0f : 11.0f;

    if (skin == Skin::neon)
    {
        g.setColour (accent.withAlpha (0.08f));
        g.fillRoundedRectangle (cx - 18.0f, top, 36.0f, bottom - top, 12.0f);
    }

    g.setColour (juce::Colour (0xff202c34));
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

    const auto thumbW = skin == Skin::broadcast ? 60.0f : skin == Skin::modular ? 64.0f : 52.0f;
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
    setSize (1120, 760);
    setResizable (true, true);
    setResizeLimits (820, 580, 1680, 1120);

    titleLabel.setText ("SANTOS LEVELER", juce::dontSendNotification);
    styleLabel (titleLabel, 25.0f, text, juce::Justification::centredLeft);
    addAndMakeVisible (titleLabel);

    subtitleLabel.setText ("VOICE AUTO LEVEL RIDER", juce::dontSendNotification);
    styleLabel (subtitleLabel, 9.5f, muted, juce::Justification::centredLeft);
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
    styleLabel (historyLabel, 10.5f, text, juce::Justification::centredLeft);
    addAndMakeVisible (historyLabel);

    inputLegendLabel.setText ("INPUT", juce::dontSendNotification);
    riderLegendLabel.setText ("RIDER", juce::dontSendNotification);
    peakLegendLabel.setText ("PEAK", juce::dontSendNotification);
    outputLegendLabel.setText ("OUTPUT", juce::dontSendNotification);
    styleLabel (inputLegendLabel, 9.2f, cyan);
    styleLabel (riderLegendLabel, 9.2f, yellow);
    styleLabel (peakLegendLabel, 9.2f, magenta);
    styleLabel (outputLegendLabel, 9.2f, green);
    addAndMakeVisible (inputLegendLabel);
    addAndMakeVisible (riderLegendLabel);
    addAndMakeVisible (peakLegendLabel);
    addAndMakeVisible (outputLegendLabel);

    skinLabel.setText ("SKIN", juce::dontSendNotification);
    motionLabel.setText ("MOTION", juce::dontSendNotification);
    styleLabel (skinLabel, 8.5f, muted, juce::Justification::centredRight);
    styleLabel (motionLabel, 8.5f, muted, juce::Justification::centredRight);
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
        beginSkinChange (static_cast<Skin> (juce::jlimit (0, 5, skinSelector.getSelectedId() - 1)));
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
    slider.setTextBoxStyle (juce::Slider::TextBoxAbove, false, 78, 22);
    slider.setTextValueSuffix (suffix);
    slider.setNumDecimalPlacesToDisplay (decimals);
    addAndMakeVisible (slider);
    label.setText (name, juce::dontSendNotification);
    styleLabel (label, 9.5f, text);
    addAndMakeVisible (label);
}

void SantosLevelerAudioProcessorEditor::configureFader (juce::Slider& slider, juce::Label& label,
                                                         const juce::String& name, const juce::String& suffix,
                                                         int decimals, juce::Colour accent)
{
    slider.setSliderStyle (juce::Slider::LinearVertical);
    slider.setTextBoxStyle (juce::Slider::TextBoxBelow, false, 82, 22);
    slider.setTextValueSuffix (suffix);
    slider.setNumDecimalPlacesToDisplay (decimals);
    slider.setColour (juce::Slider::thumbColourId, accent);
    addAndMakeVisible (slider);
    label.setText (name, juce::dontSendNotification);
    styleLabel (label, 9.5f, text);
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
    historyLabel.setColour (juce::Label::textColourId, accent.brighter (0.15f));

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
    animationPhase += 0.017f;
    if (animationPhase > juce::MathConstants<float>::twoPi)
        animationPhase -= juce::MathConstants<float>::twoPi;

    if (transitioning)
    {
        const auto step = motion == Motion::reduced ? 0.105f : 0.052f;
        transitionProgress = juce::jmin (1.0f, transitionProgress + step);

        if (! pendingApplied && transitionProgress >= 0.5f)
        {
            pendingApplied = true;
            applySkin (pendingSkin);
        }

        const auto t = transitionProgress < 0.5f
            ? 1.0f - transitionProgress * 2.0f
            : (transitionProgress - 0.5f) * 2.0f;
        setUiAlpha (juce::jlimit (0.0f, 1.0f, t));

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
    drawSkinBackground (g, getLocalBounds().toFloat());
    drawSkinCandy (g, getLocalBounds().toFloat());

    const auto active = processor.getRiderActive();
    const auto statusWidth = getWidth() < 930 ? 62 : 86;
    auto status = juce::Rectangle<float> ((float) getWidth() - (float) statusWidth - 18.0f, 55.0f,
                                          (float) statusWidth, 22.0f);
    g.setColour ((active ? green : muted).withAlpha (active ? 0.14f : 0.08f));
    g.fillRoundedRectangle (status, 11.0f);
    g.setColour (active ? green : muted);
    g.fillEllipse (status.getX() + 8.0f, status.getCentreY() - 3.0f, 6.0f, 6.0f);
    g.setFont (juce::FontOptions (8.5f));
    g.drawText (active ? "RIDING" : "IDLE", status.toNearestInt().withTrimmedLeft (17), juce::Justification::centred);

    if (transitioning)
    {
        const auto glow = 1.0f - std::abs (transitionProgress - 0.5f) * 2.0f;
        g.setColour (skinAccent (pendingSkin).withAlpha (0.08f * glow));
        g.fillAll();
    }
}

void SantosLevelerAudioProcessorEditor::drawSkinBackground (juce::Graphics& g, juce::Rectangle<float> area)
{
    const auto base = skinBase (currentSkin);
    g.fillAll (base);

    if (currentSkin == Skin::classicPro)
    {
        juce::ColourGradient grad (juce::Colour (0xff0d1a23), area.getTopLeft(), base, area.getBottomRight(), false);
        g.setGradientFill (grad);
        g.fillRect (area);
    }
    else if (currentSkin == Skin::neon)
    {
        juce::ColourGradient grad (juce::Colour (0xff0d0821), area.getTopLeft(), juce::Colour (0xff03151b), area.getBottomRight(), false);
        grad.addColour (0.48, violet.withAlpha (0.11f));
        g.setGradientFill (grad);
        g.fillRect (area);
    }
    else if (currentSkin == Skin::modular)
    {
        juce::ColourGradient grad (juce::Colour (0xff1f1914), area.getTopLeft(), juce::Colour (0xff0b0c0d), area.getBottomRight(), false);
        g.setGradientFill (grad);
        g.fillRect (area);
    }
    else if (currentSkin == Skin::analysis)
    {
        g.setColour (ice.withAlpha (0.035f));
        for (int x = 16; x < getWidth(); x += 28) g.drawVerticalLine (x, 0.0f, (float) getHeight());
        for (int y = 16; y < getHeight(); y += 28) g.drawHorizontalLine (y, 0.0f, (float) getWidth());
    }
    else if (currentSkin == Skin::radar)
    {
        juce::ColourGradient grad (juce::Colour (0xff100920), area.getCentre(), juce::Colour (0xff03070d), area.getBottomRight(), true);
        grad.addColour (0.35, magenta.withAlpha (0.08f));
        g.setGradientFill (grad);
        g.fillRect (area);
    }
    else if (currentSkin == Skin::broadcast)
    {
        g.setColour (juce::Colour (0xff0d1414));
        for (int y = 0; y < getHeight(); y += 4)
            g.drawHorizontalLine (y, 0.0f, (float) getWidth());
    }

    auto frame = area.reduced (14.0f);
    const auto radius = currentSkin == Skin::broadcast ? 4.0f : 13.0f;
    g.setColour (juce::Colour (0xff101820).withAlpha (0.88f));
    g.fillRoundedRectangle (frame, radius);
    g.setColour (skinAccent (currentSkin).withAlpha (0.24f));
    g.drawRoundedRectangle (frame, radius, 1.0f);
}

void SantosLevelerAudioProcessorEditor::drawSkinCandy (juce::Graphics& g, juce::Rectangle<float>)
{
    if (motion == Motion::off)
        return;

    const auto rider = processor.getRiderDb();
    const auto input = processor.getInputMeterDb();
    const auto pulse = 0.5f + 0.5f * std::sin (animationPhase * 1.8f);

    switch (currentSkin)
    {
        case Skin::classicPro:
        {
            const auto c = juce::Point<float> ((float) getWidth() * 0.5f, 43.0f);
            const auto r = 16.0f + juce::jlimit (0.0f, 10.0f, std::abs (rider));
            g.setColour (cyan.withAlpha (0.05f + 0.035f * pulse));
            g.drawEllipse (juce::Rectangle<float> (r * 2.0f, r * 2.0f).withCentre (c), 2.0f);
            break;
        }
        case Skin::neon:
        {
            const auto x = 20.0f + std::fmod (animationPhase * 150.0f, juce::jmax (1.0f, (float) getWidth() - 40.0f));
            juce::ColourGradient beam (violet.withAlpha (0.0f), x - 55.0f, 0.0f, cyan.withAlpha (0.13f), x, 0.0f, false);
            beam.addColour (1.0, violet.withAlpha (0.0f));
            g.setGradientFill (beam);
            g.fillRect (juce::Rectangle<float> (x - 55.0f, 16.0f, 110.0f, (float) getHeight() - 32.0f));
            break;
        }
        case Skin::modular:
        {
            const auto centreX = (float) getWidth() * 0.5f;
            const auto needleX = juce::jmap (juce::jlimit (-12.0f, 12.0f, rider), -12.0f, 12.0f,
                                             centreX - 70.0f, centreX + 70.0f);
            g.setColour (orange.withAlpha (0.70f));
            g.drawLine (centreX, 48.0f, needleX, 70.0f, 2.2f);
            g.setColour (juce::Colour (0xffffe1b9));
            g.fillEllipse (centreX - 3.0f, 45.0f, 6.0f, 6.0f);
            break;
        }
        case Skin::analysis:
        {
            const auto strength = juce::jlimit (0.0f, 1.0f, (input + 60.0f) / 60.0f);
            const auto x = 18.0f + std::fmod (animationPhase * 120.0f, juce::jmax (1.0f, (float) getWidth() - 36.0f));
            g.setColour (ice.withAlpha (0.05f + 0.13f * strength));
            g.drawVerticalLine ((int) x, 80.0f, (float) getHeight() - 22.0f);
            break;
        }
        case Skin::radar:
        {
            const auto c = juce::Point<float> ((float) getWidth() * 0.5f, (float) getHeight() * 0.52f);
            const auto r = juce::jmin ((float) getWidth(), (float) getHeight()) * 0.22f;
            const auto angle = animationPhase * 0.75f;
            const auto e = c + juce::Point<float> (std::cos (angle), std::sin (angle)) * r;
            g.setColour (magenta.withAlpha (0.08f + 0.04f * pulse));
            g.drawEllipse (juce::Rectangle<float> (r * 2.0f, r * 2.0f).withCentre (c), 1.2f);
            g.setColour (cyan.withAlpha (0.26f));
            g.drawLine ({ c, e }, 1.2f);
            break;
        }
        case Skin::broadcast:
        {
            const auto active = processor.getRiderActive();
            auto lamp = juce::Rectangle<float> ((float) getWidth() * 0.43f, 21.0f, (float) getWidth() * 0.14f, 29.0f);
            g.setColour ((active ? green : orange).withAlpha (0.10f + 0.05f * pulse));
            g.fillRoundedRectangle (lamp, 3.0f);
            g.setColour (active ? green : orange);
            g.setFont (juce::FontOptions (10.5f));
            g.drawText (active ? "AUTO RIDING" : "STANDBY", lamp.toNearestInt(), juce::Justification::centred);
            break;
        }
    }
}

void SantosLevelerAudioProcessorEditor::resized()
{
    const auto w = getWidth();
    const bool compact = w < 930;

    titleLabel.setBounds (30, 22, compact ? 240 : 330, 30);
    subtitleLabel.setBounds (32, 50, 230, 15);

    if (compact)
    {
        skinLabel.setBounds (w - 320, 22, 34, 24);
        skinSelector.setBounds (w - 282, 20, 118, 27);
        motionLabel.setBounds (w - 160, 22, 46, 24);
        motionSelector.setBounds (w - 110, 20, 76, 27);
    }
    else
    {
        skinLabel.setBounds (w - 390, 22, 42, 24);
        skinSelector.setBounds (w - 342, 20, 132, 28);
        motionLabel.setBounds (w - 206, 22, 54, 24);
        motionSelector.setBounds (w - 148, 20, 82, 28);
    }

    auto content = getLocalBounds().reduced (26);
    content.removeFromTop (64);

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
    const int cellW = juce::jmax (1, area.getWidth() / columns);
    const int cellH = juce::jmax (1, area.getHeight() / rows);

    for (int i = 0; i < 8; ++i)
    {
        const int col = i % columns;
        const int row = i / columns;
        placeKnobCell (*sliders[i], *labels[i],
                       { area.getX() + col * cellW, area.getY() + row * cellH, cellW, cellH });
    }
}

void SantosLevelerAudioProcessorEditor::layoutClassic (juce::Rectangle<int> c)
{
    const bool compact = getWidth() < 930;
    auto top = c.removeFromTop (compact ? 176 : 170);
    layoutControlGrid (top, compact ? 4 : 8);
    c.removeFromTop (8);

    auto meters = c.removeFromBottom (56);
    inputMeter.setBounds (meters.removeFromLeft (meters.getWidth() / 2).reduced (3));
    outputMeter.setBounds (meters.reduced (3));
    c.removeFromBottom (6);

    auto leftFaders = c.removeFromLeft (compact ? 165 : 210);
    auto a = leftFaders.removeFromLeft (leftFaders.getWidth() / 2);
    auto b = leftFaders;
    placeFaderCell (rangeDownSlider, rangeDownLabel, a);
    placeFaderCell (rangeUpSlider, rangeUpLabel, b);

    auto out = c.removeFromRight (compact ? 88 : 105);
    placeFaderCell (outputSlider, outputLabel, out);

    auto visual = c.reduced (6, 0);
    historyLabel.setBounds (visual.getX(), visual.getY(), 120, 20);
    auto legend = juce::Rectangle<int> (visual.getRight() - juce::jmin (240, visual.getWidth() - 125), visual.getY(),
                                        juce::jmin (240, visual.getWidth() - 125), 20);
    const int lw = juce::jmax (45, legend.getWidth() / 4);
    inputLegendLabel.setBounds (legend.removeFromLeft (lw));
    riderLegendLabel.setBounds (legend.removeFromLeft (lw));
    peakLegendLabel.setBounds (legend.removeFromLeft (lw));
    outputLegendLabel.setBounds (legend);
    history.setBounds (visual.withTrimmedTop (23));
}

void SantosLevelerAudioProcessorEditor::layoutNeon (juce::Rectangle<int> c)
{
    const bool compact = getWidth() < 930;
    auto visual = c.removeFromTop ((int) (c.getHeight() * (compact ? 0.50f : 0.58f)));
    historyLabel.setBounds (visual.getX() + 4, visual.getY(), 130, 20);
    auto legend = juce::Rectangle<int> (visual.getRight() - 240, visual.getY(), 240, 20);
    inputLegendLabel.setBounds (legend.removeFromLeft (60));
    riderLegendLabel.setBounds (legend.removeFromLeft (60));
    peakLegendLabel.setBounds (legend.removeFromLeft (60));
    outputLegendLabel.setBounds (legend);
    history.setBounds (visual.withTrimmedTop (23));

    c.removeFromTop (8);
    auto meterBar = c.removeFromBottom (54);
    inputMeter.setBounds (meterBar.removeFromLeft (meterBar.getWidth() / 2).reduced (3));
    outputMeter.setBounds (meterBar.reduced (3));
    c.removeFromBottom (5);

    auto faders = c.removeFromRight (compact ? 225 : 270);
    const int fw = faders.getWidth() / 3;
    auto a = faders.removeFromLeft (fw);
    auto b = faders.removeFromLeft (fw);
    auto d = faders;
    placeFaderCell (rangeDownSlider, rangeDownLabel, a);
    placeFaderCell (rangeUpSlider, rangeUpLabel, b);
    placeFaderCell (outputSlider, outputLabel, d);
    layoutControlGrid (c.reduced (3), compact ? 4 : 8);
}

void SantosLevelerAudioProcessorEditor::layoutModular (juce::Rectangle<int> c)
{
    const bool compact = getWidth() < 930;
    auto modules = c.removeFromTop (compact ? 230 : 205);

    if (compact)
    {
        layoutControlGrid (modules, 4);
    }
    else
    {
        juce::Slider* s[] = { &gateKnob, &detectKnob, &lookaheadKnob, &targetKnob, &speedKnob, &holdKnob, &releaseKnob, &peakThresholdKnob };
        juce::Label* l[] = { &gateLabel, &detectLabel, &lookaheadLabel, &targetLabel, &speedLabel, &holdLabel, &releaseLabel, &peakThresholdLabel };
        const int gap = 10;
        const int third = (modules.getWidth() - gap * 2) / 3;
        auto detector = modules.removeFromLeft (third);
        modules.removeFromLeft (gap);
        auto rider = modules.removeFromLeft (third);
        modules.removeFromLeft (gap);
        auto peak = modules;

        const int dw = detector.getWidth() / 3;
        for (int i = 0; i < 3; ++i) placeKnobCell (*s[i], *l[i], detector.withX (detector.getX() + i * dw).withWidth (dw));
        const int rw = rider.getWidth() / 4;
        for (int i = 0; i < 4; ++i) placeKnobCell (*s[i + 3], *l[i + 3], rider.withX (rider.getX() + i * rw).withWidth (rw));
        placeKnobCell (*s[7], *l[7], peak.reduced (peak.getWidth() / 4, 0));
    }

    c.removeFromTop (8);
    auto meters = c.removeFromBottom (54);
    inputMeter.setBounds (meters.removeFromLeft (meters.getWidth() / 2).reduced (3));
    outputMeter.setBounds (meters.reduced (3));
    c.removeFromBottom (5);

    auto faders = c.removeFromLeft (compact ? 220 : 260);
    const int fw = faders.getWidth() / 3;
    auto a = faders.removeFromLeft (fw);
    auto b = faders.removeFromLeft (fw);
    auto d = faders;
    placeFaderCell (rangeDownSlider, rangeDownLabel, a);
    placeFaderCell (rangeUpSlider, rangeUpLabel, b);
    placeFaderCell (outputSlider, outputLabel, d);

    auto visual = c.reduced (6, 0);
    historyLabel.setBounds (visual.getX(), visual.getY(), 130, 20);
    inputLegendLabel.setBounds (visual.getRight() - 220, visual.getY(), 55, 20);
    riderLegendLabel.setBounds (visual.getRight() - 165, visual.getY(), 55, 20);
    peakLegendLabel.setBounds (visual.getRight() - 110, visual.getY(), 55, 20);
    outputLegendLabel.setBounds (visual.getRight() - 55, visual.getY(), 55, 20);
    history.setBounds (visual.withTrimmedTop (23));
}

void SantosLevelerAudioProcessorEditor::layoutAnalysis (juce::Rectangle<int> c)
{
    const bool compact = getWidth() < 930;
    auto left = c.removeFromLeft (compact ? 240 : 275);
    layoutControlGrid (left.reduced (2), 2);
    c.removeFromLeft (8);

    auto bottom = c.removeFromBottom (compact ? 145 : 160);
    auto faders = bottom.removeFromLeft (compact ? 230 : 270);
    const int fw = faders.getWidth() / 3;
    auto a = faders.removeFromLeft (fw);
    auto b = faders.removeFromLeft (fw);
    auto d = faders;
    placeFaderCell (rangeDownSlider, rangeDownLabel, a);
    placeFaderCell (rangeUpSlider, rangeUpLabel, b);
    placeFaderCell (outputSlider, outputLabel, d);
    inputMeter.setBounds (bottom.removeFromLeft (bottom.getWidth() / 2).reduced (4));
    outputMeter.setBounds (bottom.reduced (4));
    c.removeFromBottom (6);

    historyLabel.setBounds (c.getX(), c.getY(), 130, 20);
    auto legend = juce::Rectangle<int> (c.getRight() - 230, c.getY(), 230, 20);
    inputLegendLabel.setBounds (legend.removeFromLeft (57));
    riderLegendLabel.setBounds (legend.removeFromLeft (57));
    peakLegendLabel.setBounds (legend.removeFromLeft (57));
    outputLegendLabel.setBounds (legend);
    history.setBounds (c.withTrimmedTop (23));
}

void SantosLevelerAudioProcessorEditor::layoutRadar (juce::Rectangle<int> c)
{
    const bool compact = getWidth() < 930;
    if (compact)
    {
        auto controls = c.removeFromBottom (208);
        auto faders = controls.removeFromRight (210);
        const int fw = faders.getWidth() / 3;
        auto a = faders.removeFromLeft (fw);
        auto b = faders.removeFromLeft (fw);
        auto d = faders;
        placeFaderCell (rangeDownSlider, rangeDownLabel, a);
        placeFaderCell (rangeUpSlider, rangeUpLabel, b);
        placeFaderCell (outputSlider, outputLabel, d);
        layoutControlGrid (controls, 4);

        auto meterRow = c.removeFromBottom (52);
        inputMeter.setBounds (meterRow.removeFromLeft (meterRow.getWidth() / 2).reduced (3));
        outputMeter.setBounds (meterRow.reduced (3));
        historyLabel.setBounds (c.getX(), c.getY(), 120, 20);
        history.setBounds (c.withTrimmedTop (23));
    }
    else
    {
        auto left = c.removeFromLeft ((int) (c.getWidth() * 0.23f));
        auto right = c.removeFromRight ((int) (c.getWidth() * 0.23f));
        layoutControlGrid (left.reduced (2), 2);

        auto faders = right.removeFromBottom ((int) (right.getHeight() * 0.48f));
        const int fw = faders.getWidth() / 3;
        auto a = faders.removeFromLeft (fw);
        auto b = faders.removeFromLeft (fw);
        auto d = faders;
        placeFaderCell (rangeDownSlider, rangeDownLabel, a);
        placeFaderCell (rangeUpSlider, rangeUpLabel, b);
        placeFaderCell (outputSlider, outputLabel, d);
        inputMeter.setBounds (right.removeFromTop (right.getHeight() / 2).reduced (4));
        outputMeter.setBounds (right.reduced (4));

        historyLabel.setBounds (c.getX(), c.getY(), 120, 20);
        history.setBounds (c.withTrimmedTop (23).reduced (3));
    }

    inputLegendLabel.setBounds (getWidth() - 1, getHeight() - 1, 1, 1);
    riderLegendLabel.setBounds (getWidth() - 1, getHeight() - 1, 1, 1);
    peakLegendLabel.setBounds (getWidth() - 1, getHeight() - 1, 1, 1);
    outputLegendLabel.setBounds (getWidth() - 1, getHeight() - 1, 1, 1);
}

void SantosLevelerAudioProcessorEditor::layoutBroadcast (juce::Rectangle<int> c)
{
    const bool compact = getWidth() < 930;

    if (! compact)
    {
        auto meterColumn = c.removeFromLeft (170);
        inputMeter.setBounds (meterColumn.removeFromLeft (82).reduced (4));
        outputMeter.setBounds (meterColumn.reduced (4));
        c.removeFromLeft (8);

        auto controlColumn = c.removeFromRight (330);
        layoutControlGrid (controlColumn.reduced (4), 2);
        c.removeFromRight (8);

        auto faderBand = c.removeFromBottom (190);
        const int fw = faderBand.getWidth() / 3;
        auto a = faderBand.removeFromLeft (fw);
        auto b = faderBand.removeFromLeft (fw);
        auto d = faderBand;
        placeFaderCell (rangeDownSlider, rangeDownLabel, a);
        placeFaderCell (rangeUpSlider, rangeUpLabel, b);
        placeFaderCell (outputSlider, outputLabel, d);
        c.removeFromBottom (8);

        historyLabel.setBounds (c.getX(), c.getY(), 125, 20);
        auto legend = juce::Rectangle<int> (c.getRight() - 240, c.getY(), 240, 20);
        inputLegendLabel.setBounds (legend.removeFromLeft (60));
        riderLegendLabel.setBounds (legend.removeFromLeft (60));
        peakLegendLabel.setBounds (legend.removeFromLeft (60));
        outputLegendLabel.setBounds (legend);
        history.setBounds (c.withTrimmedTop (23));
    }
    else
    {
        auto meters = c.removeFromLeft (118);
        inputMeter.setBounds (meters.removeFromLeft (56).reduced (2));
        outputMeter.setBounds (meters.reduced (2));
        c.removeFromLeft (7);

        auto topVisual = c.removeFromTop ((int) (c.getHeight() * 0.52f));
        historyLabel.setBounds (topVisual.getX(), topVisual.getY(), 120, 20);
        auto legend = juce::Rectangle<int> (topVisual.getX(), topVisual.getY() + 20, topVisual.getWidth(), 18);
        const int lw = legend.getWidth() / 4;
        inputLegendLabel.setBounds (legend.removeFromLeft (lw));
        riderLegendLabel.setBounds (legend.removeFromLeft (lw));
        peakLegendLabel.setBounds (legend.removeFromLeft (lw));
        outputLegendLabel.setBounds (legend);
        history.setBounds (topVisual.withTrimmedTop (40));

        c.removeFromTop (7);
        auto faders = c.removeFromLeft (205);
        const int fw = faders.getWidth() / 3;
        auto a = faders.removeFromLeft (fw);
        auto b = faders.removeFromLeft (fw);
        auto d = faders;
        placeFaderCell (rangeDownSlider, rangeDownLabel, a);
        placeFaderCell (rangeUpSlider, rangeUpLabel, b);
        placeFaderCell (outputSlider, outputLabel, d);
        c.removeFromLeft (6);
        layoutControlGrid (c, 4);
    }
}

void SantosLevelerAudioProcessorEditor::HistoryComponent::paint (juce::Graphics& g)
{
    auto r = getLocalBounds().toFloat();
    const auto accent = skinAccent (skin);

    if (skin == Skin::neon)
    {
        juce::ColourGradient grad (juce::Colour (0xff080713), r.getTopLeft(), juce::Colour (0xff04151b), r.getBottomRight(), false);
        grad.addColour (0.52, violet.withAlpha (0.13f));
        g.setGradientFill (grad);
        g.fillRoundedRectangle (r, 10.0f);
    }
    else if (skin == Skin::modular)
    {
        g.setColour (juce::Colour (0xff17130f));
        g.fillRoundedRectangle (r, 7.0f);
    }
    else
    {
        g.setColour (juce::Colour (0xff050b10));
        g.fillRoundedRectangle (r, skin == Skin::broadcast ? 3.0f : 8.0f);
    }

    g.setColour (accent.withAlpha (0.24f));
    g.drawRoundedRectangle (r.reduced (0.5f), skin == Skin::broadcast ? 3.0f : 8.0f, 1.0f);

    auto plot = r.reduced (skin == Skin::radar ? 24.0f : 36.0f, 18.0f);
    if (plot.getWidth() < 40.0f || plot.getHeight() < 40.0f)
        return;

    if (skin == Skin::radar)
    {
        const auto centre = plot.getCentre();
        const auto radius = juce::jmin (plot.getWidth(), plot.getHeight()) * 0.40f;
        for (int ring = 1; ring <= 5; ++ring)
        {
            const auto rr = radius * (float) ring / 5.0f;
            g.setColour (magenta.withAlpha (ring == 5 ? 0.16f : 0.07f));
            g.drawEllipse (juce::Rectangle<float> (rr * 2.0f, rr * 2.0f).withCentre (centre), 1.0f);
        }
        g.setColour (cyan.withAlpha (0.09f));
        g.drawLine (centre.x - radius, centre.y, centre.x + radius, centre.y, 1.0f);
        g.drawLine (centre.x, centre.y - radius, centre.x, centre.y + radius, 1.0f);

        const auto points = processor.getHistory().copyLatest (300);
        if (points.empty()) return;
        const auto& p = points.back();
        const float vals[4] = {
            juce::jlimit (0.0f, 1.0f, (p.inputDb + 60.0f) / 60.0f),
            juce::jlimit (0.0f, 1.0f, (p.riderDb + 12.0f) / 24.0f),
            juce::jlimit (0.0f, 1.0f, (p.peakDb + 12.0f) / 12.0f),
            juce::jlimit (0.0f, 1.0f, (p.outputDb + 60.0f) / 60.0f)
        };
        const juce::Colour cols[4] = { cyan, yellow, magenta, green };
        juce::Path poly;
        for (int i = 0; i < 4; ++i)
        {
            const auto a = -juce::MathConstants<float>::halfPi + juce::MathConstants<float>::twoPi * (float) i / 4.0f;
            const auto pt = centre + juce::Point<float> (std::cos (a), std::sin (a)) * (radius * vals[i]);
            if (i == 0) poly.startNewSubPath (pt); else poly.lineTo (pt);
            g.setColour (cols[i]);
            g.fillEllipse (pt.x - 3.0f, pt.y - 3.0f, 6.0f, 6.0f);
        }
        poly.closeSubPath();
        g.setColour (magenta.withAlpha (0.15f)); g.fillPath (poly);
        g.setColour (ice.withAlpha (0.85f)); g.strokePath (poly, juce::PathStrokeType (1.8f));

        const auto sweep = phase * 0.62f;
        const auto e = centre + juce::Point<float> (std::cos (sweep), std::sin (sweep)) * radius;
        g.setColour (cyan.withAlpha (0.26f)); g.drawLine ({ centre, e }, 1.4f);
        return;
    }

    g.setFont (juce::FontOptions (8.3f));
    for (int i = 0; i <= 4; ++i)
    {
        const auto y = plot.getY() + plot.getHeight() * (float) i / 4.0f;
        g.setColour (accent.withAlpha (skin == Skin::analysis ? 0.12f : 0.07f));
        g.drawHorizontalLine ((int) y, plot.getX(), plot.getRight());
        g.setColour (muted.withAlpha (0.65f));
        g.drawText (juce::String (0 - i * 15), 2, (int) y - 7, 30, 14, juce::Justification::centredRight);
    }

    for (int i = 0; i <= 6; ++i)
    {
        const auto x = plot.getX() + plot.getWidth() * (float) i / 6.0f;
        g.setColour (accent.withAlpha (skin == Skin::analysis ? 0.08f : 0.035f));
        g.drawVerticalLine ((int) x, plot.getY(), plot.getBottom());
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
        g.setColour (cyan.withAlpha (0.14f)); g.strokePath (inputPath, juce::PathStrokeType (8.0f));
        g.setColour (yellow.withAlpha (0.12f)); g.strokePath (riderPath, juce::PathStrokeType (8.0f));
        g.setColour (magenta.withAlpha (0.12f)); g.strokePath (peakPath, juce::PathStrokeType (8.0f));
        g.setColour (green.withAlpha (0.14f)); g.strokePath (outputPath, juce::PathStrokeType (8.0f));
    }

    const auto thick = skin == Skin::broadcast ? 2.5f : skin == Skin::analysis ? 1.7f : 2.0f;
    g.setColour (cyan.withAlpha (0.94f)); g.strokePath (inputPath, juce::PathStrokeType (thick));
    g.setColour (green.withAlpha (0.94f)); g.strokePath (outputPath, juce::PathStrokeType (thick));
    g.setColour (yellow); g.strokePath (riderPath, juce::PathStrokeType (thick + 0.2f));
    g.setColour (magenta.withAlpha (0.96f)); g.strokePath (peakPath, juce::PathStrokeType (thick));

    if (skin == Skin::analysis)
    {
        const auto scanX = plot.getX() + std::fmod (phase * 100.0f, juce::jmax (1.0f, plot.getWidth()));
        g.setColour (ice.withAlpha (0.20f));
        g.drawVerticalLine ((int) scanX, plot.getY(), plot.getBottom());
    }
}

void SantosLevelerAudioProcessorEditor::MeterComponent::paint (juce::Graphics& g)
{
    const auto db = source == Source::input ? processor.getInputMeterDb() : processor.getOutputMeterDb();
    auto r = getLocalBounds().toFloat();
    const bool vertical = r.getHeight() > r.getWidth() * 1.25f;
    const auto radius = skin == Skin::broadcast ? 3.0f : 7.0f;

    if (skin == Skin::broadcast)
        g.setColour (juce::Colour (0xff081011));
    else if (skin == Skin::neon)
        g.setColour (juce::Colour (0xff080713));
    else
        g.setColour (juce::Colour (0xff071017));
    g.fillRoundedRectangle (r, radius);
    g.setColour (colour.withAlpha (0.24f));
    g.drawRoundedRectangle (r.reduced (0.5f), radius, 1.0f);

    g.setColour (text);
    g.setFont (juce::FontOptions (vertical ? 8.8f : 9.5f));
    g.drawText (label, 6, 5, getWidth() - 12, 16, vertical ? juce::Justification::centred : juce::Justification::centredLeft);

    g.setColour (colour);
    g.setFont (juce::FontOptions (vertical ? 11.5f : 10.5f));
    g.drawText (juce::String (db, 1), 6, vertical ? 22 : 5, getWidth() - 12, 18,
                vertical ? juce::Justification::centred : juce::Justification::centredRight);

    const auto norm = juce::jlimit (0.0f, 1.0f, (db + 60.0f) / 60.0f);
    if (vertical)
    {
        auto bar = juce::Rectangle<float> (r.getX() + r.getWidth() * 0.26f, r.getY() + 45.0f,
                                           r.getWidth() * 0.48f, juce::jmax (10.0f, r.getHeight() - 56.0f));
        g.setColour (juce::Colour (0xff1c2a31));
        g.fillRoundedRectangle (bar, 3.0f);
        auto fill = bar;
        fill.setY (bar.getBottom() - bar.getHeight() * norm);
        fill.setHeight (bar.getHeight() * norm);
        g.setColour (colour.withAlpha (0.90f));
        g.fillRoundedRectangle (fill, 3.0f);
        g.setColour (colour.brighter (0.35f));
        g.fillRect (bar.getX() - 3.0f, fill.getY() - 1.0f, bar.getWidth() + 6.0f, 2.0f);
    }
    else
    {
        auto bar = juce::Rectangle<float> (10.0f, r.getHeight() - 18.0f, r.getWidth() - 20.0f, 9.0f);
        g.setColour (juce::Colour (0xff1c2a31));
        g.fillRoundedRectangle (bar, 3.0f);
        auto fill = bar;
        fill.setWidth (bar.getWidth() * norm);
        g.setColour (colour.withAlpha (0.92f));
        g.fillRoundedRectangle (fill, 3.0f);
    }
}
