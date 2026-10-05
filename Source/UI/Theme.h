#pragma once

#include <JuceHeader.h>

// Visual identity of Santos Leveler: a textured hardware faceplate with recessed
// modules, LED-ring knobs and glowing pads, in a single violet accent.
//
// Violet means "active / interactive", red means gain reduction or a hot signal,
// green means a healthy level. To re-skin the plugin change the values in this
// file only.
namespace SantosUI
{
namespace colour
{
    // accent
    inline const juce::Colour accent    { 0xffa86bff };
    inline const juce::Colour danger    { 0xffff5a5f };
    inline const juce::Colour signal    { 0xff7fd49a };
    inline const juce::Colour steel     { 0xff9aa6b2 };

    // faceplate and recessed modules
    inline const juce::Colour backdrop  { 0xff0b0b0c };
    inline const juce::Colour plateTop  { 0xff323235 };
    inline const juce::Colour plateBot  { 0xff232326 };
    inline const juce::Colour plateEdge { 0xff46464b };
    inline const juce::Colour recessTop { 0xff0e0e10 };
    inline const juce::Colour recessBot { 0xff1b1b1e };
    inline const juce::Colour bevel     { 0xff3b3b3f };
    inline const juce::Colour black     { 0xff050506 };
    inline const juce::Colour well      { 0xff08080a };
    inline const juce::Colour wellEdge  { 0xff2d2d31 };

    // text
    inline const juce::Colour ink       { 0xffececec };
    inline const juce::Colour dim       { 0xff8e8e94 };
    inline const juce::Colour faint     { 0xff6f6f75 };

    // module LEDs
    inline const juce::Colour ledBlue   { 0xff4aa3ff };
    inline const juce::Colour ledOrange { 0xffff9a3d };
    inline const juce::Colour ledGreen  { 0xff9be15d };
    inline const juce::Colour ledViolet { 0xff9b7bff };
}

namespace layout
{
    constexpr float designWidth = 1310.0f;
    constexpr float designHeight = 640.0f;
}

inline juce::Font font (float height, bool bold = false)
{
    return juce::Font (juce::FontOptions (height).withStyle (bold ? "Bold" : "Regular"));
}

// Upper-case engraved label with a little letter-spacing.
inline void drawCaption (juce::Graphics& g, const juce::String& text, juce::Rectangle<float> area,
                         juce::Justification justification = juce::Justification::centredLeft,
                         juce::Colour c = colour::dim, float height = 10.0f, bool bold = true)
{
    g.setColour (c);
    g.setFont (font (height, bold).withExtraKerningFactor (0.1f));
    g.drawText (text.toUpperCase(), area, justification, false);
}

// ---- surfaces --------------------------------------------------------------------------

inline const juce::Image& grainTile()
{
    static const juce::Image tile = []
    {
        juce::Image image (juce::Image::ARGB, 160, 160, true);
        juce::Random random (4711);
        for (int y = 0; y < 160; ++y)
            for (int x = 0; x < 160; ++x)
                image.setPixelAt (x, y, juce::Colour::fromFloatRGBA (1.0f, 1.0f, 1.0f, random.nextFloat() * 0.085f));
        return image;
    }();
    return tile;
}

inline void drawScrew (juce::Graphics& g, juce::Point<float> centre)
{
    g.setColour (colour::backdrop);
    g.fillEllipse (juce::Rectangle<float> (14.0f, 14.0f).withCentre (centre));
    g.setColour (juce::Colour (0xff1b1b1d));
    g.fillEllipse (juce::Rectangle<float> (13.0f, 13.0f).withCentre (centre));
    g.setGradientFill (juce::ColourGradient (juce::Colour (0xff6a6a6f), centre.x, centre.y - 5.0f,
                                             juce::Colour (0xff2a2a2d), centre.x, centre.y + 5.0f, false));
    g.fillEllipse (juce::Rectangle<float> (10.0f, 10.0f).withCentre (centre.translated (0.0f, -0.5f)));
    g.setColour (juce::Colour (0xff161617));
    g.drawLine (centre.x - 3.4f, centre.y + 0.8f, centre.x + 3.4f, centre.y - 1.8f, 1.6f);
}

inline void drawPlate (juce::Graphics& g, juce::Rectangle<float> bounds)
{
    g.fillAll (colour::backdrop);
    const auto plate = bounds.reduced (8.0f);
    g.setGradientFill (juce::ColourGradient (colour::plateTop, 0.0f, plate.getY(), colour::plateBot, 0.0f, plate.getBottom(), false));
    g.fillRoundedRectangle (plate, 18.0f);
    g.setTiledImageFill (grainTile(), 0, 0, 1.0f);
    g.fillRoundedRectangle (plate, 18.0f);
    g.setColour (colour::plateEdge);
    g.drawRoundedRectangle (plate.reduced (0.5f), 18.0f, 1.0f);
    g.setColour (juce::Colour (0xff0c0c0d));
    g.drawRoundedRectangle (plate.reduced (6.0f), 14.0f, 2.0f);

    for (const auto& p : { juce::Point<float> (32.0f, 32.0f), juce::Point<float> (bounds.getRight() - 32.0f, 32.0f),
                           juce::Point<float> (32.0f, bounds.getBottom() - 32.0f),
                           juce::Point<float> (bounds.getRight() - 32.0f, bounds.getBottom() - 32.0f) })
        drawScrew (g, p);
}

// Engraved recessed frame with a coloured LED square and a title.
inline void drawRecess (juce::Graphics& g, juce::Rectangle<float> area, const juce::String& title = {},
                        juce::Colour led = colour::accent)
{
    g.setGradientFill (juce::ColourGradient (colour::recessTop, 0.0f, area.getY(), colour::recessBot, 0.0f, area.getBottom(), false));
    g.fillRoundedRectangle (area, 12.0f);
    g.setColour (colour::black);
    g.drawRoundedRectangle (area, 12.0f, 2.0f);
    g.setColour (colour::bevel);
    g.drawRoundedRectangle (area.reduced (1.5f), 11.0f, 1.0f);

    if (title.isEmpty())
        return;

    g.setColour (led.withAlpha (0.25f));
    g.fillRoundedRectangle (area.getX() + 14.0f, area.getY() + 11.0f, 13.0f, 13.0f, 3.0f);
    g.setColour (led);
    g.fillRoundedRectangle (area.getX() + 16.0f, area.getY() + 13.0f, 9.0f, 9.0f, 2.0f);
    drawCaption (g, title, { area.getX() + 34.0f, area.getY() + 9.0f, area.getWidth() - 60.0f, 16.0f },
                 juce::Justification::centredLeft, colour::ink, 11.5f);
    g.setColour (juce::Colour (0xff2c2c30));
    g.fillRect (area.getX() + 12.0f, area.getY() + 34.0f, area.getWidth() - 24.0f, 1.5f);
    g.setColour (juce::Colours::black.withAlpha (0.6f));
    g.fillRect (area.getX() + 12.0f, area.getY() + 35.5f, area.getWidth() - 24.0f, 1.0f);
}

inline void drawLed (juce::Graphics& g, juce::Point<float> centre, float radius, bool on, juce::Colour c)
{
    if (on)
        for (int i = 3; i >= 1; --i)
        {
            g.setColour (c.withAlpha (0.10f));
            g.fillEllipse (juce::Rectangle<float> (radius * (2.0f + 1.1f * static_cast<float> (i)),
                                                    radius * (2.0f + 1.1f * static_cast<float> (i))).withCentre (centre));
        }

    g.setColour (juce::Colour (0xff0a0a0b));
    g.fillEllipse (juce::Rectangle<float> (radius * 2.0f + 4.0f, radius * 2.0f + 4.0f).withCentre (centre));
    g.setColour (on ? c : juce::Colour (0xff3a2a3c));
    g.fillEllipse (juce::Rectangle<float> (radius * 2.0f, radius * 2.0f).withCentre (centre));
    if (on)
    {
        g.setColour (juce::Colours::white.withAlpha (0.55f));
        g.fillEllipse (juce::Rectangle<float> (radius * 0.7f, radius * 0.7f).withCentre (centre.translated (-radius * 0.3f, -radius * 0.35f)));
    }
}

// ---- seven-segment read-out -------------------------------------------------------------

inline void drawSevenSegment (juce::Graphics& g, juce::Rectangle<float> area, const juce::String& text,
                              juce::Colour on, juce::Colour off)
{
    static const char* segments[] = { "abcdef", "bc", "abdeg", "abcdg", "bcfg", "acdfg", "acdefg", "abc", "abcdefg", "abcdfg" };
    const auto h = area.getHeight(), w = h * 0.5f, t = h * 0.11f, gap = h * 0.22f;
    auto x = area.getX();

    const auto segment = [&] (bool lit, float x1, float y1, float x2, float y2)
    {
        juce::Path p;
        p.startNewSubPath (x1, y1);
        p.lineTo (x2, y2);
        g.setColour (lit ? on : off);
        g.strokePath (p, juce::PathStrokeType (t, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
    };

    for (auto ch : text)
    {
        if (ch == '.')
        {
            g.setColour (on);
            g.fillEllipse (x - t * 0.1f, area.getBottom() - t * 1.2f, t * 1.3f, t * 1.3f);
            x += t * 2.4f;
            continue;
        }

        juce::String lit;
        if (ch >= '0' && ch <= '9') lit = segments[ch - '0'];
        else if (ch == '-')         lit = "g";

        const auto y = area.getY();
        const auto has = [&] (char c) { return lit.containsChar (c); };
        segment (has ('a'), x + t, y + t * 0.5f, x + w - t, y + t * 0.5f);
        segment (has ('b'), x + w - t * 0.5f, y + t, x + w - t * 0.5f, y + h * 0.5f - t * 0.4f);
        segment (has ('c'), x + w - t * 0.5f, y + h * 0.5f + t * 0.4f, x + w - t * 0.5f, y + h - t);
        segment (has ('d'), x + t, y + h - t * 0.5f, x + w - t, y + h - t * 0.5f);
        segment (has ('e'), x + t * 0.5f, y + h * 0.5f + t * 0.4f, x + t * 0.5f, y + h - t);
        segment (has ('f'), x + t * 0.5f, y + t, x + t * 0.5f, y + h * 0.5f - t * 0.4f);
        segment (has ('g'), x + t, y + h * 0.5f, x + w - t, y + h * 0.5f);
        x += w + gap;
    }
}

// ---- meters ------------------------------------------------------------------------------

// Meter scale: expanded around the useful -36 ... 0 dBFS range.
inline float meterDbToNorm (float db)
{
    db = juce::jlimit (-60.0f, 0.0f, db);
    constexpr float a = 7.0f / 34.0f, b = 6.0f / 34.0f, c = 9.0f / 34.0f, d = 12.0f / 34.0f;
    if (db <= -36.0f) return (db + 60.0f) / 24.0f * a;
    if (db <= -24.0f) return a + (db + 36.0f) / 12.0f * b;
    if (db <= -12.0f) return a + b + (db + 24.0f) / 12.0f * c;
    return a + b + c + (db + 12.0f) / 12.0f * d;
}

// Peak-hold ballistics shared by the bar meters.
struct PeakHold
{
    void update (float currentDb)
    {
        const auto now = juce::Time::getMillisecondCounterHiRes();
        if (currentDb >= heldDb)
        {
            heldDb = currentDb;
            holdUntil = now + 1000.0;
        }
        else if (now > holdUntil)
        {
            const auto dt = lastUpdate > 0.0 ? static_cast<float> ((now - lastUpdate) * 0.001) : 0.0f;
            heldDb = juce::jmax (currentDb, heldDb - 24.0f * dt);
        }
        lastUpdate = now;
    }

    float heldDb = -100.0f;
    double holdUntil = 0.0;
    double lastUpdate = 0.0;
};

inline juce::String formatDb (float value, int decimals = 1)
{
    if (value <= -99.0f)
        return juce::String::fromUTF8 ("\xe2\x88\x92\xe2\x88\x9e");   // minus infinity
    return juce::String (value, decimals);
}
} // namespace SantosUI
