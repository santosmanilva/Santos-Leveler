#pragma once

#include "Theme.h"
#include "../PluginProcessor.h"

namespace SantosUI
{
namespace detail
{
    inline float parameterValue (SantosLevelerAudioProcessor& processor, const char* id)
    {
        if (auto* value = processor.apvts.getRawParameterValue (id))
            return value->load();
        return 0.0f;
    }

    inline const juce::Identifier& traceId (int index)
    {
        static const juce::Identifier ids[] { "graphInputVisible", "graphOutputVisible", "graphRiderVisible", "graphPeakVisible" };
        return ids[index];
    }

    inline bool traceVisible (SantosLevelerAudioProcessor& processor, int index)
    {
        return static_cast<bool> (processor.apvts.state.getProperty (traceId (index), true));
    }

    inline juce::Colour traceColour (int index)
    {
        const juce::Colour colours[] { colour::steel, colour::ink, colour::accent, colour::danger };
        return colours[index];
    }
}

// ---- analogue VU meter ---------------------------------------------------------------------

class VuMeter final : public juce::Component
{
public:
    enum class Source { input, levelerOutput };

    VuMeter (SantosLevelerAudioProcessor& p, Source s, juce::String caption)
        : face (std::move (caption)), needle (p, s)
    {
        face.setBufferedToImage (true);
        face.setInterceptsMouseClicks (false, false);
        needle.setInterceptsMouseClicks (false, false);
        addAndMakeVisible (face);
        addAndMakeVisible (needle);
    }

    void resized() override
    {
        face.setBounds (getLocalBounds());
        needle.setBounds (getLocalBounds());
    }

    void refresh() { needle.repaint(); }

private:
    // Geometry shared by the face and the needle.
    struct Geometry
    {
        explicit Geometry (juce::Rectangle<float> b)
            : bounds (b), face (b.reduced (9.0f)), pivot (b.getCentreX(), b.getY() + b.getHeight() * 1.02f),
              radius (b.getHeight() * 0.84f) {}

        static float angleFor (float db) { return -48.0f + 96.0f * meterDbToNorm (db); }
        juce::Point<float> point (float distance, float degrees) const
        {
            const auto radians = juce::degreesToRadians (degrees);
            return pivot + juce::Point<float> (std::sin (radians), -std::cos (radians)) * distance;
        }

        juce::Rectangle<float> bounds, face;
        juce::Point<float> pivot;
        float radius;
    };

    class Face final : public juce::Component
    {
    public:
        explicit Face (juce::String text) : caption (std::move (text)) {}

        void paint (juce::Graphics& g) override
        {
            const Geometry geo (getLocalBounds().toFloat());
            const auto b = geo.bounds;

            juce::Path outline;
            outline.addRoundedRectangle (b, 16.0f);
            juce::DropShadow (juce::Colours::black.withAlpha (0.65f), 6, { 0, 3 }).drawForPath (g, outline);
            g.setColour (juce::Colour (0xff0a0a0b));
            g.fillRoundedRectangle (b, 16.0f);
            g.setGradientFill (juce::ColourGradient (juce::Colour (0xff8d8d92), b.getX(), b.getY(), juce::Colour (0xff050506), b.getRight(), b.getBottom(), false));
            g.fillRoundedRectangle (b.reduced (3.0f), 14.0f);
            g.setGradientFill (juce::ColourGradient (juce::Colour (0xfff6f3ea), 0.0f, geo.face.getY(), juce::Colour (0xffd6d1c0), 0.0f, geo.face.getBottom(), false));
            g.fillRoundedRectangle (geo.face, 10.0f);

            g.saveState();
            juce::Path clip;
            clip.addRoundedRectangle (geo.face, 10.0f);
            g.reduceClipRegion (clip);

            const auto red = juce::Colour (0xffc8383a), ink = juce::Colour (0xff222222);
            const auto redStart = Geometry::angleFor (-3.0f);

            juce::Path scale;
            scale.addCentredArc (geo.pivot.x, geo.pivot.y, geo.radius, geo.radius, 0.0f,
                                 juce::degreesToRadians (-48.0f), juce::degreesToRadians (redStart), true);
            g.setColour (ink);
            g.strokePath (scale, juce::PathStrokeType (1.6f));
            juce::Path hot;
            hot.addCentredArc (geo.pivot.x, geo.pivot.y, geo.radius, geo.radius, 0.0f,
                               juce::degreesToRadians (redStart), juce::degreesToRadians (48.0f), true);
            g.setColour (red);
            g.strokePath (hot, juce::PathStrokeType (5.0f));

            for (int db = -48; db <= 0; db += 3)
            {
                const auto angle = Geometry::angleFor (static_cast<float> (db));
                g.setColour (db >= -3 ? red : ink);
                g.drawLine ({ geo.point (geo.radius - 1.0f, angle), geo.point (geo.radius - (db % 12 == 0 ? 9.0f : 5.0f), angle) }, 1.2f);
            }

            g.setFont (font (8.5f, true));
            for (const int db : { -40, -30, -20, -12, -6, -3, 0 })
            {
                g.setColour (db >= -3 ? red : ink);
                const auto p = geo.point (geo.radius - 19.0f, Geometry::angleFor (static_cast<float> (db)));
                g.drawText (db == 0 ? "0" : juce::String (db), juce::Rectangle<float> (26.0f, 12.0f).withCentre (p), juce::Justification::centred, false);
            }

            g.setColour (ink);
            g.setFont (font (12.0f, true).withExtraKerningFactor (0.15f));
            g.drawText ("VU", juce::Rectangle<float> (b.getX(), b.getY() + b.getHeight() * 0.50f, b.getWidth(), 16.0f), juce::Justification::centred, false);
            g.setColour (juce::Colour (0xff555555));
            g.setFont (font (8.5f, true).withExtraKerningFactor (0.14f));
            g.drawText (caption.toUpperCase(), juce::Rectangle<float> (b.getX(), b.getY() + b.getHeight() * 0.70f, b.getWidth(), 14.0f), juce::Justification::centred, false);
            g.restoreState();
        }

    private:
        juce::String caption;
    };

    class Needle final : public juce::Component
    {
    public:
        Needle (SantosLevelerAudioProcessor& p, Source s) : processor (p), source (s) {}

        void paint (juce::Graphics& g) override
        {
            const auto target = source == Source::input ? processor.getInputMeterDb() : processor.getOutputMeterDb();
            const auto now = juce::Time::getMillisecondCounterHiRes();
            const auto dt = lastPaint > 0.0 ? static_cast<float> (juce::jlimit (1.0, 100.0, now - lastPaint)) : 33.0f;
            lastPaint = now;
            const auto clamped = juce::jmax (-60.0f, target);
            if (first)
            {
                shown = clamped;   // do not sweep up from silence when the editor opens
                first = false;
            }
            const auto tau = clamped > shown ? 70.0f : 260.0f;   // quick rise, slower fall like a real VU
            shown += (clamped - shown) * (1.0f - std::exp (-dt / tau));

            const Geometry geo (getLocalBounds().toFloat());
            g.saveState();
            juce::Path clip;
            clip.addRoundedRectangle (geo.face, 10.0f);
            g.reduceClipRegion (clip);

            const auto angle = Geometry::angleFor (shown);
            const auto tip = geo.point (geo.radius + 4.0f, angle);
            g.setColour (juce::Colours::black.withAlpha (0.28f));
            g.drawLine ({ geo.pivot.translated (2.0f, 3.0f), tip.translated (2.0f, 3.0f) }, 2.2f);
            g.setColour (juce::Colour (0xff111111));
            g.drawLine ({ geo.pivot, tip }, 2.0f);

            g.setGradientFill (juce::ColourGradient (juce::Colours::white.withAlpha (0.10f), 0.0f, geo.face.getY(),
                                                     juce::Colours::white.withAlpha (0.0f), 0.0f, geo.face.getY() + geo.face.getHeight() * 0.45f, false));
            g.fillRect (geo.face);
            g.restoreState();
        }

    private:
        SantosLevelerAudioProcessor& processor;
        Source source;
        float shown = -60.0f;
        double lastPaint = 0.0;
        bool first = true;
    };

    Face face;
    Needle needle;
};

// ---- live response display ---------------------------------------------------------------

class LiveGraph final : public juce::Component
{
public:
    explicit LiveGraph (SantosLevelerAudioProcessor& p) : processor (p) { setOpaque (true); }

    void paint (juce::Graphics& g) override
    {
        const auto area = getLocalBounds().toFloat();
        g.fillAll (juce::Colour (0xff05090a));
        g.setColour (colour::accent.withAlpha (0.045f));
        g.fillRect (area);

        constexpr float axisWidth = 26.0f, gap = 8.0f;
        auto plot = area.withTrimmedLeft (axisWidth);
        const auto gainHeight = (plot.getHeight() - gap) * 0.28f;
        auto gainLane = plot.removeFromBottom (gainHeight);
        plot.removeFromBottom (gap);

        paintLevels (g, plot, area.getX());
        paintGain (g, gainLane, area.getX());

        if (processor.hasHostTransport() && ! processor.getTransportPlaying())
        {
            g.setColour (juce::Colours::black.withAlpha (0.6f));
            g.fillRect (area);
            drawCaption (g, "Paused", area, juce::Justification::centred, colour::ink, 13.0f);
        }

        // glass glare
        g.setGradientFill (juce::ColourGradient (juce::Colours::white.withAlpha (0.08f), 0.0f, 0.0f,
                                                 juce::Colours::white.withAlpha (0.0f), 0.0f, area.getHeight() * 0.4f, false));
        g.fillRect (area);
    }

private:
    static float levelY (juce::Rectangle<float> r, float db)
    {
        return r.getBottom() - juce::jlimit (0.0f, 1.0f, (db + 60.0f) / 60.0f) * r.getHeight();
    }

    static juce::Path buildPath (const std::vector<SantosHistoryPoint>& points, juce::Rectangle<float> r,
                                 const std::function<float (const SantosHistoryPoint&)>& yOf)
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

    static void dashedLine (juce::Graphics& g, float y, float x1, float x2, juce::Colour c)
    {
        g.setColour (c);
        for (auto x = x1; x < x2; x += 9.0f)
            g.drawLine (x, y, juce::jmin (x + 5.0f, x2), y, 1.0f);
    }

    void paintLevels (juce::Graphics& g, juce::Rectangle<float> r, float labelX)
    {
        g.setColour (juce::Colour (0xff0a1012).interpolatedWith (colour::accent, 0.05f));
        g.fillRoundedRectangle (r, 5.0f);

        for (const int mark : { 0, -12, -24, -36, -48 })
        {
            const auto y = levelY (r, static_cast<float> (mark));
            g.setColour (juce::Colour (0xff1c1b24));
            g.fillRect (r.getX(), y, r.getWidth(), 1.0f);
            g.setColour (juce::Colour (0xff6c7480));
            g.setFont (font (9.5f));
            g.drawText (juce::String (mark), juce::Rectangle<float> (labelX, juce::jlimit (r.getY(), r.getBottom() - 14.0f, y - 7.0f), 22.0f, 14.0f), juce::Justification::centredRight, false);
        }

        const auto points = processor.getHistory().copyLatest (320);
        if (points.size() < 2)
            return;

        const auto fillBelow = [&] (juce::Path p)
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

        if (detail::traceVisible (processor, 0))
        {
            g.setColour (colour::steel.withAlpha (0.13f));
            g.fillPath (fillBelow (inputPath));
            g.setColour (colour::steel);
            g.strokePath (inputPath, juce::PathStrokeType (1.5f, juce::PathStrokeType::curved));
        }

        const auto targetY = levelY (r, detail::parameterValue (processor, "target"));
        const auto peakY = levelY (r, detail::parameterValue (processor, "peakThreshold"));
        dashedLine (g, targetY, r.getX(), r.getRight(), colour::accent.withAlpha (0.85f));
        dashedLine (g, peakY, r.getX(), r.getRight(), colour::danger.withAlpha (0.7f));

        if (detail::traceVisible (processor, 1))
        {
            g.setColour (colour::ink.withAlpha (0.06f));
            g.fillPath (fillBelow (outputPath));
            g.setColour (colour::ink);
            g.strokePath (outputPath, juce::PathStrokeType (1.9f, juce::PathStrokeType::curved));
        }
        g.restoreState();

        const auto chip = [&] (const char* text, float width, float y, juce::Colour c)
        {
            const auto box = juce::Rectangle<float> (r.getX() + 6.0f, y - 7.0f, width, 14.0f);
            g.setColour (juce::Colour (0xff05090a).withAlpha (0.92f));
            g.fillRoundedRectangle (box, 3.0f);
            drawCaption (g, text, box, juce::Justification::centred, c, 8.5f);
        };
        chip ("TARGET", 46.0f, targetY, colour::accent);
        chip ("PEAK", 34.0f, peakY, colour::danger);
    }

    void paintGain (juce::Graphics& g, juce::Rectangle<float> r, float labelX)
    {
        g.setColour (juce::Colour (0xff0a1012).interpolatedWith (colour::accent, 0.05f));
        g.fillRoundedRectangle (r, 5.0f);

        const auto gainY = [&] (float db)
        {
            return r.getBottom() - juce::jlimit (0.0f, 1.0f, (db + 12.0f) / 24.0f) * r.getHeight();
        };

        for (const int mark : { 12, 0, -12 })
        {
            const auto y = gainY (static_cast<float> (mark));
            g.setColour (mark == 0 ? juce::Colour (0xff4b4b55) : juce::Colour (0xff1c1b24));
            g.fillRect (r.getX(), y, r.getWidth(), 1.0f);
            g.setColour (juce::Colour (0xff6c7480));
            g.setFont (font (9.5f));
            g.drawText ((mark > 0 ? "+" : "") + juce::String (mark),
                        juce::Rectangle<float> (labelX, juce::jlimit (r.getY(), r.getBottom() - 14.0f, y - 7.0f), 22.0f, 14.0f), juce::Justification::centredRight, false);
        }

        drawCaption (g, "Gain dB", { r.getX() + 8.0f, r.getY() + 3.0f, 70.0f, 12.0f }, juce::Justification::centredLeft,
                     juce::Colour (0xff6c7480), 8.5f);

        const auto points = processor.getHistory().copyLatest (320);
        if (points.size() < 2)
            return;

        const auto zeroY = gainY (0.0f);
        const auto areaToZero = [&] (juce::Path line)
        {
            line.lineTo (r.getRight(), zeroY);
            line.lineTo (r.getX(), zeroY);
            line.closeSubPath();
            return line;
        };

        g.saveState();
        g.reduceClipRegion (r.toNearestInt());

        if (detail::traceVisible (processor, 2))
        {
            const auto rider = buildPath (points, r, [&] (const SantosHistoryPoint& p) { return gainY (p.riderDb); });
            g.setColour (colour::accent.withAlpha (0.22f));
            g.fillPath (areaToZero (rider));
            g.setColour (colour::accent);
            g.strokePath (rider, juce::PathStrokeType (1.8f, juce::PathStrokeType::curved));
        }

        if (detail::traceVisible (processor, 3))
        {
            const auto peak = buildPath (points, r, [&] (const SantosHistoryPoint& p) { return gainY (p.peakDb); });
            g.setColour (colour::danger.withAlpha (0.22f));
            g.fillPath (areaToZero (peak));

            // Only draw the line while the peak stage is actually reducing gain.
            juce::Path active;
            auto drawing = false;
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
};

// Recessed bezel around the live graph.
class ScreenFrame final : public juce::Component
{
public:
    explicit ScreenFrame (SantosLevelerAudioProcessor& p) : graph (p)
    {
        setBufferedToImage (true);
        addAndMakeVisible (graph);
    }

    void paint (juce::Graphics& g) override
    {
        const auto b = getLocalBounds().toFloat().reduced (2.0f);
        juce::Path outline;
        outline.addRoundedRectangle (b, 14.0f);
        juce::DropShadow (juce::Colours::black.withAlpha (0.65f), 6, { 0, 3 }).drawForPath (g, outline);
        g.setColour (juce::Colour (0xff070708));
        g.fillRoundedRectangle (b, 14.0f);
        g.setGradientFill (juce::ColourGradient (juce::Colour (0xff8d8d92), b.getX(), b.getY(), juce::Colour (0xff050506), b.getRight(), b.getBottom(), false));
        g.fillRoundedRectangle (b.reduced (3.0f), 12.0f);
        g.setColour (juce::Colour (0xff05090a));
        g.fillRoundedRectangle (b.reduced (8.0f), 8.0f);
    }

    void resized() override { graph.setBounds (getLocalBounds().reduced (14)); }
    void refresh() { graph.repaint(); }

private:
    LiveGraph graph;
};

// Toggleable legend shown in the live-response module header.
class LegendBar final : public juce::Component
{
public:
    explicit LegendBar (SantosLevelerAudioProcessor& p) : processor (p) {}

    void paint (juce::Graphics& g) override
    {
        static const char* names[] { "Input", "Output", "Rider", "Peak" };
        const auto cell = static_cast<float> (getWidth()) / 4.0f;
        for (int i = 0; i < 4; ++i)
        {
            const auto area = juce::Rectangle<float> (cell * static_cast<float> (i), 0.0f, cell, static_cast<float> (getHeight()));
            const auto on = detail::traceVisible (processor, i);
            g.setColour (on ? detail::traceColour (i) : colour::faint.withAlpha (0.5f));
            g.fillRoundedRectangle (area.getX() + 4.0f, area.getCentreY() - 1.5f, 11.0f, 3.0f, 1.5f);
            drawCaption (g, names[i], area.withTrimmedLeft (20.0f), juce::Justification::centredLeft,
                         on ? colour::dim : colour::faint.withAlpha (0.6f), 9.0f, false);
        }
    }

    void mouseUp (const juce::MouseEvent& e) override
    {
        const auto index = juce::jlimit (0, 3, static_cast<int> (e.position.x / (static_cast<float> (getWidth()) / 4.0f)));
        processor.apvts.state.setProperty (detail::traceId (index), ! detail::traceVisible (processor, index), nullptr);
        repaint();
    }

private:
    SantosLevelerAudioProcessor& processor;
};

// Four large read-outs under the display.
class StatsRow final : public juce::Component
{
public:
    explicit StatsRow (SantosLevelerAudioProcessor& p) : processor (p) {}

    void paint (juce::Graphics& g) override
    {
        const auto rider = processor.getRiderDb();
        const juce::String riderText = (rider > 0.05f ? "+" : "") + juce::String (rider, 1) + " dB";
        const struct { const char* caption; juce::String value; juce::Colour colour; } cells[] {
            { "Rider",       riderText,                                                                      colour::ink },
            { "Target",      juce::String (detail::parameterValue (processor, "target"), 1) + " dB",        colour::ink },
            { "Peak limit",  juce::String (detail::parameterValue (processor, "peakThreshold"), 1) + " dBFS", colour::danger },
            { "Leveler out", formatDb (processor.getOutputMeterDb()) + " dBFS",                              colour::ink }
        };

        const auto cellWidth = static_cast<float> (getWidth()) / 4.0f;
        for (int i = 0; i < 4; ++i)
        {
            auto cell = juce::Rectangle<float> (cellWidth * static_cast<float> (i), 0.0f, cellWidth, static_cast<float> (getHeight()));
            drawCaption (g, cells[i].caption, cell.removeFromTop (14.0f), juce::Justification::centredLeft, colour::dim, 8.5f);
            g.setColour (cells[i].colour);
            g.setFont (font (18.0f, true));
            g.drawText (cells[i].value, cell, juce::Justification::topLeft, false);
        }
    }

private:
    SantosLevelerAudioProcessor& processor;
};

// Output module contents: peak, true peak, gain reduction bars and the loudness LCDs.
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

    void resized() override { resetButton.setBounds (getWidth() - 84, getHeight() - 40, 84, 38); }

    void paint (juce::Graphics& g) override
    {
        const auto outDb = processor.getFinalOutputMeterDb();
        hold.update (outDb);

        const auto tp = processor.getOutputTruePeakDbTP();
        const auto compGr = juce::jlimit (-18.0f, 0.0f, processor.getCompressorReductionDb());
        const auto limGr = juce::jlimit (-12.0f, 0.0f, processor.getTruePeakReductionDb());

        struct Row { const char* label; juce::String value; float norm; juce::Colour fill; float hold; };
        const Row rows[] {
            { "Peak",       formatDb (outDb) + " dBFS", meterDbToNorm (outDb),            colour::signal, meterDbToNorm (hold.heldDb) },
            { "True peak",  formatDb (tp) + " dBTP",    meterDbToNorm (tp),               tp > -1.0f ? colour::danger : colour::signal, -1.0f },
            { "Compressor", formatDb (compGr) + " dB",  -compGr / 18.0f,                  colour::danger, -1.0f },
            { "Limiter",    formatDb (limGr) + " dB",   -limGr / 12.0f,                   colour::danger, -1.0f }
        };

        for (int i = 0; i < 4; ++i)
        {
            const auto y = 12.0f + 32.0f * static_cast<float> (i);
            drawCaption (g, rows[i].label, { 0.0f, y - 8.0f, 150.0f, 12.0f }, juce::Justification::centredLeft, colour::dim, 9.0f);
            g.setColour (colour::ink);
            g.setFont (font (11.5f, true));
            g.drawText (rows[i].value, juce::Rectangle<float> (static_cast<float> (getWidth()) - 150.0f, y - 8.0f, 150.0f, 12.0f), juce::Justification::centredRight, false);
            drawBar (g, { 0.0f, y + 7.0f, static_cast<float> (getWidth()), 7.0f }, rows[i].norm, rows[i].fill, rows[i].hold);
        }

        const float values[] { processor.getMomentaryLufs(), processor.getShortTermLufs(), processor.getIntegratedLufs() };
        const char* names[] { "Momentary", "Short-term", "Integrated" };
        for (int i = 0; i < 3; ++i)
        {
            const auto lcd = juce::Rectangle<float> (static_cast<float> (i) * 103.0f, 154.0f, 94.0f, 60.0f);
            g.setColour (juce::Colour (0xff06080a));
            g.fillRoundedRectangle (lcd, 6.0f);
            g.setColour (juce::Colour (0xff202428));
            g.drawRoundedRectangle (lcd.reduced (0.5f), 6.0f, 1.0f);
            drawCaption (g, names[i], lcd.withHeight (16.0f).translated (0.0f, 2.0f), juce::Justification::centred, juce::Colour (0xff79838a), 7.5f);

            const auto text = formatDb (values[i]);
            auto digits = lcd.withTrimmedTop (21.0f).withTrimmedBottom (16.0f).reduced (8.0f, 0.0f);
            const auto digitHeight = digits.getHeight();
            const auto dots = text.retainCharacters (".").length();
            const auto glyphs = text.length() - dots;
            const auto textWidth = static_cast<float> (glyphs) * digitHeight * 0.72f + static_cast<float> (dots) * digitHeight * 0.264f - digitHeight * 0.22f;
            drawSevenSegment (g, digits.withSizeKeepingCentre (juce::jmin (textWidth, digits.getWidth()), digitHeight), text,
                              colour::accent, juce::Colour (0xff14181a));
            drawCaption (g, "LUFS", lcd.withTrimmedTop (lcd.getHeight() - 15.0f), juce::Justification::centred, juce::Colour (0xff79838a), 7.5f, false);
        }
    }

private:
    static void drawBar (juce::Graphics& g, juce::Rectangle<float> bar, float norm, juce::Colour fill, float holdNorm)
    {
        g.setColour (juce::Colour (0xff0a0a0b));
        g.fillRoundedRectangle (bar, 3.0f);
        g.setColour (juce::Colour (0xff1d1d20));
        g.drawRoundedRectangle (bar.reduced (0.5f), 3.0f, 1.0f);

        norm = juce::jlimit (0.0f, 1.0f, norm);
        if (norm > 0.003f)
        {
            g.setColour (fill);
            g.fillRoundedRectangle (bar.reduced (1.0f).withWidth (juce::jmax (4.0f, (bar.getWidth() - 2.0f) * norm)), 2.0f);
        }

        if (holdNorm > 0.0f)
        {
            g.setColour (colour::ink);
            g.fillRect (bar.getX() + (bar.getWidth() - 2.0f) * juce::jlimit (0.0f, 1.0f, holdNorm), bar.getY() - 2.0f, 2.0f, bar.getHeight() + 4.0f);
        }
    }

    SantosLevelerAudioProcessor& processor;
    juce::TextButton resetButton;
    PeakHold hold;
};
} // namespace SantosUI
