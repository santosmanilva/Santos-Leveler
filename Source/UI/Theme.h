#pragma once

#include <JuceHeader.h>

// Visual identity of Santos Leveler.
//
// One interactive accent (amber) on a graphite surface. Red is reserved for
// gain reduction / clipping and sage green for healthy signal level, so every
// colour on screen carries a meaning. To re-skin the plugin change the values
// in this file only.
namespace SantosUI
{
namespace colour
{
    inline const juce::Colour window    { 0xff0d0f12 };
    inline const juce::Colour card      { 0xff14171b };
    inline const juce::Colour cardHigh  { 0xff1b1f24 };
    inline const juce::Colour edge      { 0xff23282e };
    inline const juce::Colour track     { 0xff2a3037 };

    inline const juce::Colour text      { 0xffe9ebee };
    inline const juce::Colour textDim   { 0xff9aa2ac };
    inline const juce::Colour textFaint { 0xff5f6670 };

    inline const juce::Colour accent    { 0xffe8a33d };   // interactive / rider
    inline const juce::Colour signal    { 0xff7fbf9f };   // healthy level
    inline const juce::Colour danger    { 0xffe5575b };   // gain reduction, clip
    inline const juce::Colour steel     { 0xff8794a3 };   // input trace
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

// Small upper-case section label, e.g. "LIVE RESPONSE".
inline void drawCaption (juce::Graphics& g, const juce::String& text, juce::Rectangle<float> area,
                         juce::Justification justification = juce::Justification::centredLeft,
                         juce::Colour colour = colour::textDim, float height = 11.0f)
{
    g.setColour (colour);
    g.setFont (font (height, true).withExtraKerningFactor (0.09f));
    g.drawText (text.toUpperCase(), area, justification, false);
}

inline void drawCard (juce::Graphics& g, juce::Rectangle<float> area, float radius = 10.0f)
{
    g.setColour (colour::card);
    g.fillRoundedRectangle (area, radius);
    g.setColour (colour::edge);
    g.drawRoundedRectangle (area.reduced (0.5f), radius, 1.0f);
}

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

inline juce::Colour levelColour (float db)
{
    if (db > -3.0f)  return colour::danger;
    if (db > -12.0f) return colour::accent;
    return colour::signal;
}

// Peak-hold ballistics shared by every meter.
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
} // namespace SantosUI
