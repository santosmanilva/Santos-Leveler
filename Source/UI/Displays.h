#pragma once

#include "Theme.h"
#include "../PluginProcessor.h"

namespace SantosUI
{
namespace
{
    float parameterValue (SantosLevelerAudioProcessor& processor, const char* id)
    {
        if (auto* value = processor.apvts.getRawParameterValue (id))
            return value->load();
        return 0.0f;
    }

    juce::String formatDb (float value, int decimals = 1)
    {
        if (value <= -99.0f)
            return juce::String::fromUTF8 ("\xe2\x88\x92\xe2\x88\x9e");   // minus infinity
        return juce::String (value, decimals);
    }

    void drawDashedHorizontal (juce::Graphics& g, float y, float x1, float x2, juce::Colour c)
    {
        g.setColour (c);
        for (float x = x1; x < x2; x += 8.0f)
            g.drawLine (x, y, juce::jmin (x + 4.0f, x2), y, 1.0f);
    }
}

// Vertical peak meter with a numeric read-out (used for INPUT and LEVELER OUT).
class LevelMeterCard final : public juce::Component
{
public:
    enum class Source { input, levelerOutput };

    LevelMeterCard (SantosLevelerAudioProcessor& p, Source s, juce::String caption)
        : processor (p), source (s), name (std::move (caption)) {}

    void paint (juce::Graphics& g) override
    {
        const auto db = source == Source::input ? processor.getInputMeterDb() : processor.getOutputMeterDb();
        hold.update (db);

        auto area = getLocalBounds().toFloat();
        drawCard (g, area);

        drawCaption (g, name, area.removeFromTop (30).withTrimmedTop (12), juce::Justification::centred);
        g.setColour (colour::text);
        g.setFont (font (15.0f, true));
        g.drawText (formatDb (db), area.removeFromTop (22).toNearestInt(), juce::Justification::centred, false);
        g.setColour (colour::textFaint);
        g.setFont (font (10.0f));
        g.drawText ("dBFS", area.removeFromTop (14).toNearestInt(), juce::Justification::centred, false);

        area.removeFromTop (8);
        area.removeFromBottom (14);
        auto scale = area.removeFromLeft (26);
        area.removeFromLeft (4);
        auto bar = area.removeFromLeft (14);

        for (const float mark : { 0.0f, -12.0f, -24.0f, -36.0f, -48.0f, -60.0f })
        {
            const auto y = bar.getBottom() - meterDbToNorm (mark) * bar.getHeight();
            g.setColour (colour::textFaint);
            g.setFont (font (9.0f));
            g.drawText (juce::String (static_cast<int> (mark)),
                        juce::Rectangle<float> (scale.getX(), y - 6.0f, scale.getWidth(), 12.0f),
                        juce::Justification::centredRight, false);
        }

        drawVerticalBar (g, bar, db, hold.heldDb);
    }

private:
    static void drawVerticalBar (juce::Graphics& g, juce::Rectangle<float> bar, float db, float heldDb)
    {
        g.setColour (colour::window);
        g.fillRoundedRectangle (bar, 3.0f);

        const auto norm = meterDbToNorm (db);
        if (norm > 0.002f)
        {
            juce::ColourGradient gradient (colour::signal, 0.0f, bar.getBottom(), colour::danger, 0.0f, bar.getY(), false);
            gradient.addColour (meterDbToNorm (-14.0f), colour::signal);
            gradient.addColour (meterDbToNorm (-10.0f), colour::accent);
            gradient.addColour (meterDbToNorm (-4.0f), colour::accent);
            gradient.addColour (meterDbToNorm (-1.5f), colour::danger);
            g.setGradientFill (gradient);
            const auto height = juce::jmax (3.0f, norm * bar.getHeight());
            g.fillRoundedRectangle (bar.withTop (bar.getBottom() - height), 3.0f);
        }

        const auto held = meterDbToNorm (heldDb);
        if (held > 0.0f)
        {
            const auto y = bar.getBottom() - held * bar.getHeight();
            g.setColour (colour::text);
            g.fillRect (bar.getX() - 2.0f, y - 1.0f, bar.getWidth() + 4.0f, 2.0f);
        }
    }

    SantosLevelerAudioProcessor& processor;
    Source source;
    juce::String name;
    PeakHold hold;
};

// Final output: peak, true peak, compressor and limiter reduction, plus loudness.
class OutputPanel final : public juce::Component
{
public:
    explicit OutputPanel (SantosLevelerAudioProcessor& p) : processor (p)
    {
        resetButton.setButtonText ("Reset");
        resetButton.onClick = [this] { processor.requestLoudnessReset(); };
        resetButton.setTooltip ("Reset integrated loudness and true peak");
        addAndMakeVisible (resetButton);
    }

    void resized() override
    {
        resetButton.setBounds (getWidth() - 14 - 62, 10, 62, 24);
    }

    void paint (juce::Graphics& g) override
    {
        auto area = getLocalBounds().toFloat();
        drawCard (g, area);
        area.reduce (16.0f, 0.0f);

        auto title = area.removeFromTop (40);
        drawCaption (g, "Output", title.withTrimmedTop (4), juce::Justification::centredLeft, colour::textDim, 12.0f);

        const auto outDb = processor.getFinalOutputMeterDb();
        hold.update (outDb);

        // --- peak --------------------------------------------------------------
        auto row = area.removeFromTop (32);
        drawRowLabel (g, row, "Peak", formatDb (outDb) + " dBFS", colour::text);
        drawHorizontalBar (g, row.removeFromBottom (10), meterDbToNorm (outDb), levelColour (outDb), meterDbToNorm (hold.heldDb));

        // --- true peak -----------------------------------------------------------
        const auto tp = processor.getOutputTruePeakDbTP();
        g.setColour (colour::textDim);
        g.setFont (font (11.0f));
        g.drawText ("True peak", area.removeFromTop (22).toNearestInt().withTrimmedTop (2), juce::Justification::centredLeft, false);
        g.setColour (tp > -1.0f ? colour::danger : colour::text);
        g.setFont (font (11.5f, true));
        g.drawText (formatDb (tp) + " dBTP", juce::Rectangle<float> (area.getX(), area.getY() - 22.0f, area.getWidth(), 22.0f).toNearestInt().withTrimmedTop (2),
                    juce::Justification::centredRight, false);

        // --- gain reduction --------------------------------------------------------
        const auto compGr = juce::jlimit (-18.0f, 0.0f, processor.getCompressorReductionDb());
        row = area.removeFromTop (32);
        drawRowLabel (g, row, "Compressor", formatDb (compGr) + " dB", colour::danger);
        drawHorizontalBar (g, row.removeFromBottom (10), -compGr / 18.0f, colour::danger, -1.0f);

        const auto limGr = juce::jlimit (-12.0f, 0.0f, processor.getTruePeakReductionDb());
        row = area.removeFromTop (32);
        drawRowLabel (g, row, "Limiter", formatDb (limGr) + " dB", colour::danger);
        drawHorizontalBar (g, row.removeFromBottom (10), -limGr / 12.0f, colour::danger, -1.0f);

        // --- loudness ------------------------------------------------------------
        area.removeFromTop (8);
        g.setColour (colour::edge);
        g.fillRect (area.getX(), area.getY(), area.getWidth(), 1.0f);
        area.removeFromTop (10);

        const float values[] { processor.getMomentaryLufs(), processor.getShortTermLufs(), processor.getIntegratedLufs() };
        const char* names[] { "Momentary", "Short-term", "Integrated" };
        const auto cellWidth = area.getWidth() / 3.0f;
        for (int i = 0; i < 3; ++i)
        {
            auto cell = juce::Rectangle<float> (area.getX() + cellWidth * static_cast<float> (i), area.getY(), cellWidth - 10.0f, 66.0f);
            drawCaption (g, names[i], cell.removeFromTop (14), juce::Justification::centredLeft, colour::textFaint, 9.5f);
            g.setColour (colour::text);
            g.setFont (font (22.0f, true));
            g.drawText (formatDb (values[i]), cell.removeFromTop (28).toNearestInt(), juce::Justification::centredLeft, false);
            g.setColour (colour::textFaint);
            g.setFont (font (10.0f));
            g.drawText ("LUFS", cell.removeFromTop (13).toNearestInt(), juce::Justification::centredLeft, false);
            cell.removeFromTop (4);
            drawHorizontalBar (g, cell.removeFromTop (5), juce::jlimit (0.0f, 1.0f, (values[i] + 35.0f) / 30.0f), colour::steel, -1.0f);
        }
    }

private:
    static void drawRowLabel (juce::Graphics& g, juce::Rectangle<float> row, const juce::String& label,
                              const juce::String& value, juce::Colour valueColour)
    {
        auto top = row.withHeight (20.0f);
        g.setColour (colour::textDim);
        g.setFont (font (11.0f));
        g.drawText (label, top.toNearestInt(), juce::Justification::centredLeft, false);
        g.setColour (valueColour);
        g.setFont (font (11.5f, true));
        g.drawText (value, top.toNearestInt(), juce::Justification::centredRight, false);
    }

    static void drawHorizontalBar (juce::Graphics& g, juce::Rectangle<float> bar, float norm, juce::Colour fill, float holdNorm)
    {
        bar = bar.withSizeKeepingCentre (bar.getWidth(), juce::jmin (bar.getHeight(), 6.0f));
        g.setColour (colour::window);
        g.fillRoundedRectangle (bar, 3.0f);

        norm = juce::jlimit (0.0f, 1.0f, norm);
        if (norm > 0.003f)
        {
            g.setColour (fill);
            g.fillRoundedRectangle (bar.withWidth (juce::jmax (4.0f, bar.getWidth() * norm)), 3.0f);
        }

        if (holdNorm > 0.0f)
        {
            g.setColour (colour::text);
            g.fillRect (bar.getX() + bar.getWidth() * juce::jlimit (0.0f, 1.0f, holdNorm) - 1.0f, bar.getY() - 2.0f, 2.0f, bar.getHeight() + 4.0f);
        }
    }

    SantosLevelerAudioProcessor& processor;
    juce::TextButton resetButton;
    PeakHold hold;
};

// Live history of levels and gain, with toggleable traces.
class LiveGraph final : public juce::Component
{
public:
    explicit LiveGraph (SantosLevelerAudioProcessor& p) : processor (p) {}

    void paint (juce::Graphics& g) override
    {
        auto area = getLocalBounds().toFloat();
        drawCard (g, area);

        auto inner = area.reduced (18.0f, 14.0f);
        auto header = inner.removeFromTop (24.0f);
        drawCaption (g, "Live response", header, juce::Justification::centredLeft, colour::textDim, 12.0f);
        paintLegend (g, header);

        auto stats = inner.removeFromBottom (58.0f);
        inner.removeFromBottom (8.0f);
        paintStats (g, stats);

        inner.removeFromTop (6.0f);
        auto plot = inner.withTrimmedLeft (32.0f);
        const auto gainHeight = plot.getHeight() * 0.28f;
        auto gainLane = plot.removeFromBottom (gainHeight);
        plot.removeFromBottom (14.0f);
        const auto levels = plot;

        paintLevels (g, levels, inner.getX());
        paintGain (g, gainLane, inner.getX());

        if (processor.hasHostTransport() && ! processor.getTransportPlaying())
        {
            g.setColour (colour::card.withAlpha (0.72f));
            g.fillRoundedRectangle (levels.getUnion (gainLane), 6.0f);
            drawCaption (g, "Paused", levels.getUnion (gainLane), juce::Justification::centred, colour::text, 13.0f);
        }
    }

    void mouseUp (const juce::MouseEvent& e) override
    {
        for (int i = 0; i < 4; ++i)
            if (legendCells[static_cast<std::size_t> (i)].contains (e.position))
            {
                setTraceVisible (i, ! isTraceVisible (i));
                repaint();
                return;
            }
    }

private:
    static constexpr const char* traceNames[] { "Input", "Output", "Rider", "Peak" };
    static const juce::Identifier& traceId (int index)
    {
        static const juce::Identifier ids[] { "graphInputVisible", "graphOutputVisible", "graphRiderVisible", "graphPeakVisible" };
        return ids[index];
    }

    bool isTraceVisible (int index) const
    {
        return static_cast<bool> (processor.apvts.state.getProperty (traceId (index), true));
    }

    void setTraceVisible (int index, bool visible)
    {
        processor.apvts.state.setProperty (traceId (index), visible, nullptr);
    }

    static juce::Colour traceColour (int index)
    {
        const juce::Colour colours[] { colour::steel, colour::text, colour::accent, colour::danger };
        return colours[index];
    }

    void paintLegend (juce::Graphics& g, juce::Rectangle<float> header)
    {
        const auto cellWidth = 82.0f;
        auto cells = header.removeFromRight (cellWidth * 4.0f);
        for (int i = 0; i < 4; ++i)
        {
            auto cell = cells.removeFromLeft (cellWidth);
            legendCells[static_cast<std::size_t> (i)] = cell;
            const auto on = isTraceVisible (i);
            const auto c = traceColour (i);
            g.setColour (on ? c : colour::textFaint.withAlpha (0.5f));
            g.fillRoundedRectangle (cell.getX() + 6.0f, cell.getCentreY() - 1.5f, 14.0f, 3.0f, 1.5f);
            g.setColour (on ? colour::textDim : colour::textFaint.withAlpha (0.6f));
            g.setFont (font (11.5f));
            g.drawText (traceNames[i], cell.withTrimmedLeft (26.0f), juce::Justification::centredLeft, false);
        }
    }

    void paintStats (juce::Graphics& g, juce::Rectangle<float> stats)
    {
        const auto rider = processor.getRiderDb();
        const juce::String riderText = (rider > 0.05f ? "+" : "") + juce::String (rider, 1);
        const struct { const char* caption; juce::String value; const char* unit; juce::Colour colour; } cells[] {
            { "Rider",        riderText,                                                   "dB",   colour::accent },
            { "Target",       juce::String (parameterValue (processor, "target"), 1),      "dB",   colour::text },
            { "Peak limit",   juce::String (parameterValue (processor, "peakThreshold"), 1), "dBFS", colour::text },
            { "Leveler out",  formatDb (processor.getOutputMeterDb()),                     "dBFS", colour::text }
        };

        const auto cellWidth = stats.getWidth() / 4.0f;
        for (int i = 0; i < 4; ++i)
        {
            auto cell = juce::Rectangle<float> (stats.getX() + cellWidth * static_cast<float> (i), stats.getY(), cellWidth, stats.getHeight());
            if (i > 0)
            {
                g.setColour (colour::edge);
                g.fillRect (cell.getX(), cell.getY() + 8.0f, 1.0f, cell.getHeight() - 16.0f);
            }
            cell.reduce (16.0f, 0.0f);
            drawCaption (g, cells[i].caption, cell.removeFromTop (20.0f).withTrimmedTop (6.0f), juce::Justification::centredLeft, colour::textFaint, 10.0f);
            g.setColour (cells[i].colour);
            g.setFont (font (22.0f, true));
            const auto valueArea = cell.toNearestInt();
            g.drawText (cells[i].value, valueArea, juce::Justification::topLeft, false);
            const auto valueWidth = juce::GlyphArrangement::getStringWidth (font (22.0f, true), cells[i].value);
            g.setColour (colour::textFaint);
            g.setFont (font (11.0f));
            g.drawText (cells[i].unit, valueArea.withTrimmedLeft (static_cast<int> (valueWidth) + 5).withTrimmedTop (9), juce::Justification::topLeft, false);
        }
    }

    float levelY (juce::Rectangle<float> r, float db) const
    {
        return r.getBottom() - juce::jlimit (0.0f, 1.0f, (db + 60.0f) / 60.0f) * r.getHeight();
    }

    juce::Path buildPath (const std::vector<SantosHistoryPoint>& points, juce::Rectangle<float> r,
                          const std::function<float (const SantosHistoryPoint&)>& yOf) const
    {
        juce::Path path;
        for (std::size_t i = 0; i < points.size(); ++i)
        {
            const auto x = r.getX() + r.getWidth() * static_cast<float> (i) / static_cast<float> (points.size() - 1);
            const auto y = yOf (points[i]);
            if (i == 0) path.startNewSubPath (x, y); else path.lineTo (x, y);
        }
        return path;
    }

    void paintLevels (juce::Graphics& g, juce::Rectangle<float> r, float labelX)
    {
        g.setColour (colour::window);
        g.fillRoundedRectangle (r, 6.0f);

        for (const int mark : { 0, -12, -24, -36, -48 })
        {
            const auto y = levelY (r, static_cast<float> (mark));
            g.setColour (colour::edge);
            g.fillRect (r.getX(), y, r.getWidth(), 1.0f);
            g.setColour (colour::textFaint);
            g.setFont (font (10.0f));
            g.drawText (juce::String (mark), juce::Rectangle<float> (labelX, y - 7.0f, 26.0f, 14.0f), juce::Justification::centredRight, false);
        }

        const auto points = processor.getHistory().copyLatest (320);
        if (points.size() < 2)
            return;

        auto fillBelow = [&] (juce::Path p)
        {
            p.lineTo (r.getRight(), r.getBottom());
            p.lineTo (r.getX(), r.getBottom());
            p.closeSubPath();
            return p;
        };

        const auto inputPath = buildPath (points, r, [&] (const SantosHistoryPoint& p) { return levelY (r, p.inputDb); });
        const auto outputPath = buildPath (points, r, [&] (const SantosHistoryPoint& p) { return levelY (r, p.outputDb); });

        g.saveState();
        g.reduceClipRegion (r.toNearestInt());

        if (isTraceVisible (0))
        {
            g.setColour (colour::steel.withAlpha (0.12f));
            g.fillPath (fillBelow (inputPath));
            g.setColour (colour::steel);
            g.strokePath (inputPath, juce::PathStrokeType (1.5f, juce::PathStrokeType::curved));
        }

        // Reference lines: where the rider aims and where the peak stage starts to act.
        const auto targetY = levelY (r, parameterValue (processor, "target"));
        drawDashedHorizontal (g, targetY, r.getX(), r.getRight(), colour::accent.withAlpha (0.75f));
        const auto peakY = levelY (r, parameterValue (processor, "peakThreshold"));
        drawDashedHorizontal (g, peakY, r.getX(), r.getRight(), colour::danger.withAlpha (0.55f));

        if (isTraceVisible (1))
        {
            g.setColour (colour::text.withAlpha (0.07f));
            g.fillPath (fillBelow (outputPath));
            g.setColour (colour::text);
            g.strokePath (outputPath, juce::PathStrokeType (1.8f, juce::PathStrokeType::curved));
        }
        g.restoreState();

        const auto chip = [&] (const char* text, float y, juce::Colour c)
        {
            const auto box = juce::Rectangle<float> (r.getX() + 6.0f, y - 7.0f, 46.0f, 14.0f);
            g.setColour (colour::window.withAlpha (0.92f));
            g.fillRoundedRectangle (box, 3.0f);
            g.setColour (c);
            g.setFont (font (9.0f, true).withExtraKerningFactor (0.08f));
            g.drawText (text, box, juce::Justification::centred, false);
        };
        chip ("TARGET", targetY, colour::accent);
        chip ("PEAK", peakY, colour::danger);
    }

    void paintGain (juce::Graphics& g, juce::Rectangle<float> r, float labelX)
    {
        g.setColour (colour::window);
        g.fillRoundedRectangle (r, 6.0f);

        const auto gainY = [&] (float db)
        {
            return r.getBottom() - juce::jlimit (0.0f, 1.0f, (db + 12.0f) / 24.0f) * r.getHeight();
        };

        for (const int mark : { 12, 0, -12 })
        {
            const auto y = gainY (static_cast<float> (mark));
            g.setColour (mark == 0 ? colour::textFaint.withAlpha (0.55f) : colour::edge);
            g.fillRect (r.getX(), y - (mark == 0 ? 0.0f : 0.0f), r.getWidth(), 1.0f);
            g.setColour (colour::textFaint);
            g.setFont (font (10.0f));
            g.drawText ((mark > 0 ? "+" : "") + juce::String (mark),
                        juce::Rectangle<float> (labelX, y - 7.0f, 26.0f, 14.0f), juce::Justification::centredRight, false);
        }

        g.setColour (colour::textFaint);
        g.setFont (font (9.5f, true).withExtraKerningFactor (0.08f));
        g.drawText ("GAIN dB", juce::Rectangle<float> (r.getX() + 8.0f, r.getY() + 3.0f, 70.0f, 12.0f), juce::Justification::centredLeft, false);

        const auto points = processor.getHistory().copyLatest (320);
        if (points.size() < 2)
            return;

        const auto zeroY = gainY (0.0f);
        const auto areaBetween = [&] (juce::Path line)
        {
            line.lineTo (r.getRight(), zeroY);
            line.lineTo (r.getX(), zeroY);
            line.closeSubPath();
            return line;
        };

        g.saveState();
        g.reduceClipRegion (r.toNearestInt());

        if (isTraceVisible (2))
        {
            const auto rider = buildPath (points, r, [&] (const SantosHistoryPoint& p) { return gainY (p.riderDb); });
            g.setColour (colour::accent.withAlpha (0.20f));
            g.fillPath (areaBetween (rider));
            g.setColour (colour::accent);
            g.strokePath (rider, juce::PathStrokeType (1.8f, juce::PathStrokeType::curved));
        }

        if (isTraceVisible (3))
        {
            const auto peak = buildPath (points, r, [&] (const SantosHistoryPoint& p) { return gainY (p.peakDb); });
            g.setColour (colour::danger.withAlpha (0.22f));
            g.fillPath (areaBetween (peak));

            // Only draw the line while the peak stage is actually reducing gain.
            juce::Path active;
            bool drawing = false;
            for (std::size_t i = 0; i < points.size(); ++i)
            {
                const auto x = r.getX() + r.getWidth() * static_cast<float> (i) / static_cast<float> (points.size() - 1);
                if (points[i].peakDb < -0.05f)
                {
                    if (! drawing) active.startNewSubPath (x, zeroY);
                    active.lineTo (x, gainY (points[i].peakDb));
                    drawing = true;
                }
                else if (drawing)
                {
                    active.lineTo (x, zeroY);
                    drawing = false;
                }
            }
            g.setColour (colour::danger);
            g.strokePath (active, juce::PathStrokeType (1.5f, juce::PathStrokeType::curved));
        }
        g.restoreState();
    }

    SantosLevelerAudioProcessor& processor;
    std::array<juce::Rectangle<float>, 4> legendCells {};
};
} // namespace SantosUI
