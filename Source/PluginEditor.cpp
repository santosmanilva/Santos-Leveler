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
const auto magenta   = juce::Colour (0xffd57be8);

void styleLabel (juce::Label& label, float size, juce::Colour colour, int justification = juce::Justification::centred)
{
    label.setFont (juce::FontOptions (size));
    label.setColour (juce::Label::textColourId, colour);
    label.setJustificationType (justification);
    label.setInterceptsMouseClicks (false, false);
}

float levelToNorm (float db)
{
    return juce::jlimit (0.0f, 1.0f, (db + 60.0f) / 60.0f);
}

float gainToNorm (float db)
{
    return juce::jlimit (0.0f, 1.0f, (db + 12.0f) / 24.0f);
}

juce::Path makeHistoryPath (juce::Rectangle<float> plot,
                            const std::vector<SantosHistoryPoint>& points,
                            const std::function<float(const SantosHistoryPoint&)>& value,
                            const std::function<float(float)>& normalise)
{
    juce::Path path;
    if (points.empty())
        return path;

    for (std::size_t i = 0; i < points.size(); ++i)
    {
        const auto x = plot.getX()
            + plot.getWidth() * static_cast<float> (i)
              / static_cast<float> (std::max<std::size_t> (1, points.size() - 1));
        const auto y = plot.getBottom() - normalise (value (points[i])) * plot.getHeight();

        if (i == 0)
            path.startNewSubPath (x, y);
        else
            path.lineTo (x, y);
    }

    return path;
}

void drawCard (juce::Graphics& g, juce::Rectangle<float> area)
{
    g.setColour (juce::Colour (0xff0b1115));
    g.fillRoundedRectangle (area, 6.0f);
    g.setColour (border.withAlpha (0.7f));
    g.drawRoundedRectangle (area.reduced (0.5f), 6.0f, 1.0f);
}

void drawValueText (juce::Graphics& g, juce::Rectangle<float> area,
                    juce::String name, juce::String value, juce::Colour colour)
{
    g.setColour (muted);
    g.setFont (juce::FontOptions (9.0f));
    g.drawText (name, area.removeFromTop (15.0f).toNearestInt(), juce::Justification::centred);
    g.setColour (colour);
    g.setFont (juce::FontOptions (15.0f));
    g.drawText (value, area.toNearestInt(), juce::Justification::centred);
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
    setColour (juce::ComboBox::backgroundColourId, juce::Colour (0xff11191f));
    setColour (juce::ComboBox::textColourId, text);
    setColour (juce::ComboBox::outlineColourId, border);
    setColour (juce::ComboBox::arrowColourId, cyan);
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

SantosLevelerAudioProcessorEditor::HistoryComponent::HistoryComponent (SantosLevelerAudioProcessor& p)
    : processor (p)
{
}

void SantosLevelerAudioProcessorEditor::HistoryComponent::setViewMode (int newMode)
{
    viewMode = juce::jlimit (0, 5, newMode);
    repaint();
}

void SantosLevelerAudioProcessorEditor::HistoryComponent::paint (juce::Graphics& g)
{
    auto r = getLocalBounds().toFloat();
    drawCard (g, r);

    const auto points = processor.getHistory().copyLatest (300);

    switch (viewMode)
    {
        case 1: drawArea (g, r, points); break;
        case 2: drawLayers (g, r, points); break;
        case 3: drawSpectrum (g, r); break;
        case 4: drawRadar (g, r, points); break;
        case 5: drawBroadcast (g, r); break;
        default: drawClassic (g, r, points); break;
    }

    if (processor.hasHostTransport() && ! processor.getTransportPlaying())
    {
        auto overlay = r.reduced (42.0f, 18.0f);
        g.setColour (juce::Colour (0xaa0b1115));
        g.fillRoundedRectangle (overlay, 5.0f);
        g.setColour (text);
        g.setFont (juce::FontOptions (14.0f));
        g.drawText ("PAUSED", overlay.toNearestInt(), juce::Justification::centred);
    }
}

void SantosLevelerAudioProcessorEditor::HistoryComponent::drawClassic (
    juce::Graphics& g, juce::Rectangle<float> r, const std::vector<SantosHistoryPoint>& points)
{
    auto plot = r.reduced (42.0f, 18.0f);
    plot.removeFromRight (36.0f);

    g.setFont (juce::FontOptions (9.5f));
    for (int i = 0; i <= 4; ++i)
    {
        const auto y = plot.getY() + plot.getHeight() * static_cast<float> (i) / 4.0f;
        g.setColour (juce::Colour (0xff26343e));
        g.drawHorizontalLine (static_cast<int> (y), plot.getX(), plot.getRight());

        const auto levelDb = -15.0f * static_cast<float> (i);
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

    if (points.size() >= 2)
    {
        auto inputPath = makeHistoryPath (plot, points, [] (const auto& p) { return p.inputDb; }, levelToNorm);
        auto outputPath = makeHistoryPath (plot, points, [] (const auto& p) { return p.outputDb; }, levelToNorm);
        auto riderPath = makeHistoryPath (plot, points, [] (const auto& p) { return p.riderDb; }, gainToNorm);
        auto peakPath = makeHistoryPath (plot, points, [] (const auto& p) { return p.peakDb; }, gainToNorm);

        g.setColour (cyan.withAlpha (0.86f));
        g.strokePath (inputPath, juce::PathStrokeType (2.0f));
        g.setColour (green.withAlpha (0.92f));
        g.strokePath (outputPath, juce::PathStrokeType (2.0f));
        g.setColour (yellow);
        g.strokePath (riderPath, juce::PathStrokeType (2.2f));
        g.setColour (magenta.withAlpha (0.95f));
        g.strokePath (peakPath, juce::PathStrokeType (1.8f));
    }

    g.setColour (muted);
    g.setFont (juce::FontOptions (9.0f));
    g.drawText ("LEVEL dBFS", 4, 2, 70, 14, juce::Justification::centredLeft);
    g.drawText ("GAIN dB", getWidth() - 72, 2, 68, 14, juce::Justification::centredRight);
}

void SantosLevelerAudioProcessorEditor::HistoryComponent::drawArea (
    juce::Graphics& g, juce::Rectangle<float> r, const std::vector<SantosHistoryPoint>& points)
{
    auto plot = r.reduced (28.0f, 22.0f);

    for (int i = 0; i < 5; ++i)
    {
        const auto y = plot.getY() + plot.getHeight() * static_cast<float> (i) / 4.0f;
        g.setColour (juce::Colour (0xff243039));
        g.drawHorizontalLine (static_cast<int> (y), plot.getX(), plot.getRight());
    }

    if (points.size() < 2)
        return;

    auto drawFilled = [&] (auto selector, auto normalise, juce::Colour colour, float alpha)
    {
        auto path = makeHistoryPath (plot, points, selector, normalise);
        auto fill = path;
        fill.lineTo (plot.getRight(), plot.getBottom());
        fill.lineTo (plot.getX(), plot.getBottom());
        fill.closeSubPath();
        g.setColour (colour.withAlpha (alpha));
        g.fillPath (fill);
        g.setColour (colour);
        g.strokePath (path, juce::PathStrokeType (1.8f));
    };

    drawFilled ([] (const auto& p) { return p.inputDb; }, levelToNorm, cyan, 0.10f);
    drawFilled ([] (const auto& p) { return p.outputDb; }, levelToNorm, green, 0.10f);
    drawFilled ([] (const auto& p) { return p.riderDb; }, gainToNorm, yellow, 0.08f);
    drawFilled ([] (const auto& p) { return p.peakDb; }, gainToNorm, magenta, 0.08f);

    g.setColour (muted);
    g.setFont (juce::FontOptions (9.0f));
    g.drawText ("SHADED HISTORY", plot.toNearestInt(), juce::Justification::topLeft);
}

void SantosLevelerAudioProcessorEditor::HistoryComponent::drawLayers (
    juce::Graphics& g, juce::Rectangle<float> r, const std::vector<SantosHistoryPoint>& points)
{
    auto content = r.reduced (18.0f, 14.0f);
    const float gap = 5.0f;
    const float bandH = (content.getHeight() - gap * 3.0f) / 4.0f;

    const juce::String names[] = { "INPUT", "RIDER", "PEAK", "OUTPUT" };
    const juce::Colour colours[] = { cyan, yellow, magenta, green };

    for (int band = 0; band < 4; ++band)
    {
        auto bandArea = juce::Rectangle<float> (content.getX(), content.getY() + band * (bandH + gap), content.getWidth(), bandH);
        g.setColour (panel2);
        g.fillRoundedRectangle (bandArea, 4.0f);
        g.setColour (border.withAlpha (0.35f));
        g.drawRoundedRectangle (bandArea, 4.0f, 1.0f);

        auto plot = bandArea.reduced (54.0f, 6.0f);
        plot.translate (42.0f, 0.0f);

        if (points.size() >= 2)
        {
            juce::Path path;
            if (band == 0)
                path = makeHistoryPath (plot, points, [] (const auto& p) { return p.inputDb; }, levelToNorm);
            else if (band == 1)
                path = makeHistoryPath (plot, points, [] (const auto& p) { return p.riderDb; }, gainToNorm);
            else if (band == 2)
                path = makeHistoryPath (plot, points, [] (const auto& p) { return p.peakDb; }, gainToNorm);
            else
                path = makeHistoryPath (plot, points, [] (const auto& p) { return p.outputDb; }, levelToNorm);

            g.setColour (colours[band]);
            g.strokePath (path, juce::PathStrokeType (1.8f));
        }

        g.setColour (colours[band]);
        g.setFont (juce::FontOptions (9.5f));
        g.drawText (names[band], static_cast<int> (bandArea.getX() + 6.0f), static_cast<int> (bandArea.getY()), 52,
                    static_cast<int> (bandArea.getHeight()), juce::Justification::centredLeft);
    }
}

void SantosLevelerAudioProcessorEditor::HistoryComponent::drawSpectrum (
    juce::Graphics& g, juce::Rectangle<float> r)
{
    processor.copySpectrumInput (spectrumSamples);
    std::fill (fftData.begin(), fftData.end(), 0.0f);

    std::copy (spectrumSamples.begin(), spectrumSamples.end(), fftData.begin());
    window.multiplyWithWindowingTable (fftData.data(), static_cast<int> (SantosLevelerAudioProcessor::spectrumSize));
    fft.performFrequencyOnlyForwardTransform (fftData.data());

    auto plot = r.reduced (32.0f, 28.0f);
    const auto sampleRate = std::max (1.0, processor.getCurrentSampleRateForDisplay());
    const auto maxFrequency = std::min (20000.0, sampleRate * 0.5);

    for (int i = 0; i <= 4; ++i)
    {
        const auto y = plot.getY() + plot.getHeight() * static_cast<float> (i) / 4.0f;
        g.setColour (juce::Colour (0xff26343e));
        g.drawHorizontalLine (static_cast<int> (y), plot.getX(), plot.getRight());
    }

    juce::Path spectrumPath;
    constexpr int displayBins = 220;

    for (int i = 0; i < displayBins; ++i)
    {
        const auto proportion = static_cast<double> (i) / static_cast<double> (displayBins - 1);
        const auto freq = 20.0 * std::pow (maxFrequency / 20.0, proportion);
        const auto bin = juce::jlimit (1, static_cast<int> (SantosLevelerAudioProcessor::spectrumSize / 2 - 1),
                                      static_cast<int> (freq * SantosLevelerAudioProcessor::spectrumSize / sampleRate));
        const auto magnitude = fftData[static_cast<std::size_t> (bin)]
            / static_cast<float> (SantosLevelerAudioProcessor::spectrumSize);
        const auto db = juce::Decibels::gainToDecibels (magnitude, -90.0f);
        const auto norm = juce::jlimit (0.0f, 1.0f, (db + 90.0f) / 90.0f);
        const auto x = plot.getX() + static_cast<float> (proportion) * plot.getWidth();
        const auto y = plot.getBottom() - norm * plot.getHeight();

        if (i == 0)
            spectrumPath.startNewSubPath (x, y);
        else
            spectrumPath.lineTo (x, y);
    }

    auto fill = spectrumPath;
    fill.lineTo (plot.getRight(), plot.getBottom());
    fill.lineTo (plot.getX(), plot.getBottom());
    fill.closeSubPath();

    g.setColour (cyan.withAlpha (0.12f));
    g.fillPath (fill);
    g.setColour (cyan);
    g.strokePath (spectrumPath, juce::PathStrokeType (1.8f));

    const double labels[] = { 20.0, 100.0, 1000.0, 5000.0, 20000.0 };
    const juce::String labelText[] = { "20", "100", "1k", "5k", "20k" };
    g.setFont (juce::FontOptions (8.5f));
    g.setColour (muted);
    for (int i = 0; i < 5; ++i)
    {
        const auto f = std::min (labels[i], maxFrequency);
        const auto xNorm = std::log (f / 20.0) / std::log (maxFrequency / 20.0);
        const auto x = plot.getX() + static_cast<float> (xNorm) * plot.getWidth();
        g.drawText (labelText[i], static_cast<int> (x - 18.0f), static_cast<int> (plot.getBottom() + 3.0f), 36, 12,
                    juce::Justification::centred);
    }

    auto metrics = r.removeFromTop (42.0f).reduced (120.0f, 4.0f);
    const auto cellW = metrics.getWidth() / 4.0f;
    drawValueText (g, { metrics.getX(), metrics.getY(), cellW, metrics.getHeight() }, "INPUT",
                   juce::String (processor.getInputMeterDb(), 1), cyan);
    drawValueText (g, { metrics.getX() + cellW, metrics.getY(), cellW, metrics.getHeight() }, "RIDER",
                   juce::String (processor.getRiderDb(), 1), yellow);
    drawValueText (g, { metrics.getX() + cellW * 2.0f, metrics.getY(), cellW, metrics.getHeight() }, "PEAK",
                   juce::String (processor.getPeakReductionDb(), 1), magenta);
    drawValueText (g, { metrics.getX() + cellW * 3.0f, metrics.getY(), cellW, metrics.getHeight() }, "OUTPUT",
                   juce::String (processor.getOutputMeterDb(), 1), green);
}

void SantosLevelerAudioProcessorEditor::HistoryComponent::drawRadar (
    juce::Graphics& g, juce::Rectangle<float> r, const std::vector<SantosHistoryPoint>&)
{
    auto area = r.reduced (38.0f, 18.0f);
    const auto centre = area.getCentre();
    const auto radius = std::min (area.getWidth(), area.getHeight()) * 0.38f;

    for (int ring = 1; ring <= 4; ++ring)
    {
        g.setColour (border.withAlpha (0.35f));
        g.drawEllipse (juce::Rectangle<float> (radius * 2.0f * ring / 4.0f,
                                                radius * 2.0f * ring / 4.0f).withCentre (centre), 1.0f);
    }

    const float input = levelToNorm (processor.getInputMeterDb());
    const float rider = std::abs (processor.getRiderDb()) / 12.0f;
    const float peak = std::abs (processor.getPeakReductionDb()) / 12.0f;
    const float output = levelToNorm (processor.getOutputMeterDb());
    const float values[] = { input, rider, peak, output };
    const juce::Colour colours[] = { cyan, yellow, magenta, green };
    const juce::String names[] = { "INPUT", "RIDER", "PEAK", "OUTPUT" };

    juce::Path polygon;
    for (int i = 0; i < 4; ++i)
    {
        const auto angle = -juce::MathConstants<float>::halfPi + i * juce::MathConstants<float>::halfPi;
        const auto end = centre + juce::Point<float> (std::cos (angle), std::sin (angle)) * radius;
        g.setColour (border.withAlpha (0.55f));
        g.drawLine ({ centre, end }, 1.0f);

        const auto p = centre + juce::Point<float> (std::cos (angle), std::sin (angle)) * radius * values[i];
        if (i == 0) polygon.startNewSubPath (p); else polygon.lineTo (p);

        auto labelPoint = centre + juce::Point<float> (std::cos (angle), std::sin (angle)) * (radius + 22.0f);
        g.setColour (colours[i]);
        g.setFont (juce::FontOptions (9.0f));
        g.drawText (names[i], static_cast<int> (labelPoint.x - 30.0f), static_cast<int> (labelPoint.y - 8.0f), 60, 16,
                    juce::Justification::centred);
    }
    polygon.closeSubPath();
    g.setColour (cyan.withAlpha (0.10f));
    g.fillPath (polygon);
    g.setColour (text.withAlpha (0.9f));
    g.strokePath (polygon, juce::PathStrokeType (2.0f));
}

void SantosLevelerAudioProcessorEditor::HistoryComponent::drawBroadcast (
    juce::Graphics& g, juce::Rectangle<float> r)
{
    auto area = r.reduced (28.0f, 18.0f);
    const float values[] = {
        levelToNorm (processor.getInputMeterDb()),
        gainToNorm (processor.getRiderDb()),
        gainToNorm (processor.getPeakReductionDb()),
        levelToNorm (processor.getOutputMeterDb())
    };
    const juce::Colour colours[] = { cyan, yellow, magenta, green };
    const juce::String names[] = { "INPUT", "RIDER", "PEAK", "OUTPUT" };
    const juce::String readouts[] = {
        juce::String (processor.getInputMeterDb(), 1) + " dBFS",
        juce::String (processor.getRiderDb(), 1) + " dB",
        juce::String (processor.getPeakReductionDb(), 1) + " dB",
        juce::String (processor.getOutputMeterDb(), 1) + " dBFS"
    };

    const auto columnW = area.getWidth() / 4.0f;
    for (int i = 0; i < 4; ++i)
    {
        auto column = juce::Rectangle<float> (area.getX() + i * columnW, area.getY(), columnW, area.getHeight());
        auto bar = column.reduced (columnW * 0.33f, 28.0f);
        bar.removeFromBottom (24.0f);
        g.setColour (juce::Colour (0xff26343e));
        g.fillRoundedRectangle (bar, 5.0f);

        auto fill = bar;
        fill.removeFromTop (bar.getHeight() * (1.0f - values[i]));
        g.setColour (colours[i].withAlpha (0.85f));
        g.fillRoundedRectangle (fill, 5.0f);

        g.setColour (colours[i]);
        g.setFont (juce::FontOptions (10.0f));
        g.drawText (names[i], column.removeFromTop (18.0f).toNearestInt(), juce::Justification::centred);
        g.setFont (juce::FontOptions (11.0f));
        g.drawText (readouts[i], static_cast<int> (column.getX()), static_cast<int> (area.getBottom() - 22.0f),
                    static_cast<int> (column.getWidth()), 20, juce::Justification::centred);
    }
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
      outputAttachment (p.apvts, "output", outputSlider),
      viewAttachment (p.apvts, "view", viewSelector)
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

    viewLabel.setText ("VIEW", juce::dontSendNotification);
    styleLabel (viewLabel, 10.0f, muted, juce::Justification::centredRight);
    addAndMakeVisible (viewLabel);

    viewSelector.addItemList ({ "Classic", "Area", "Layers", "Spectrum", "Radar", "Broadcast" }, 1);
    viewSelector.setJustificationType (juce::Justification::centred);
    viewSelector.onChange = [this]
    {
        history.setViewMode (viewSelector.getSelectedItemIndex());
    };
    addAndMakeVisible (viewSelector);

    inputLegendLabel.setText ("INPUT", juce::dontSendNotification);
    styleLabel (inputLegendLabel, 9.5f, cyan, juce::Justification::centred);
    addAndMakeVisible (inputLegendLabel);

    riderLegendLabel.setText ("RIDER", juce::dontSendNotification);
    styleLabel (riderLegendLabel, 9.5f, yellow, juce::Justification::centred);
    addAndMakeVisible (riderLegendLabel);

    peakLegendLabel.setText ("PEAK", juce::dontSendNotification);
    styleLabel (peakLegendLabel, 9.5f, magenta, juce::Justification::centred);
    addAndMakeVisible (peakLegendLabel);

    outputLegendLabel.setText ("OUTPUT", juce::dontSendNotification);
    styleLabel (outputLegendLabel, 9.5f, green, juce::Justification::centred);
    addAndMakeVisible (outputLegendLabel);

    addAndMakeVisible (history);
    addAndMakeVisible (inputMeter);
    addAndMakeVisible (outputMeter);

    history.setViewMode (viewSelector.getSelectedItemIndex());
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

    viewLabel.setBounds (w - 360, 34, 48, 22);
    viewSelector.setBounds (w - 308, 34, 150, 24);

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
        knobs[i]->setBounds (x, knobY, knobW, knobH);
        labels[i]->setBounds (x, knobY + knobH + 2, knobW, 20);
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
    historyLabel.setBounds (historyX, lowerTop, 90, 20);

    const int legendWidth = 54;
    const int legendGap = 2;
    const int legendTotalWidth = legendWidth * 4 + legendGap * 3;
    const int legendStartX = historyX + historyW - legendTotalWidth;
    inputLegendLabel.setBounds (legendStartX, lowerTop, legendWidth, 20);
    riderLegendLabel.setBounds (legendStartX + (legendWidth + legendGap), lowerTop, legendWidth, 20);
    peakLegendLabel.setBounds (legendStartX + (legendWidth + legendGap) * 2, lowerTop, legendWidth, 20);
    outputLegendLabel.setBounds (legendStartX + (legendWidth + legendGap) * 3, lowerTop, legendWidth, 20);

    history.setBounds (historyX, lowerTop + 24, historyW, std::max (150, lowerBottom - lowerTop - 28));

    const int meterGap = 34;
    const int meterW = (w - 72 - meterGap) / 2;
    inputMeter.setBounds (36, meterY, meterW, meterHeight);
    outputMeter.setBounds (36 + meterW + meterGap, meterY, meterW, meterHeight);
}

void SantosLevelerAudioProcessorEditor::timerCallback()
{
    history.setViewMode (viewSelector.getSelectedItemIndex());
    repaint();
    history.repaint();
    inputMeter.repaint();
    outputMeter.repaint();
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
