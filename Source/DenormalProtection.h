#pragma once

#include <cmath>

// Portable denormal protection for audio DSP.
// On x86 with DAZ/FTZ this is a no-op; on ARM and other platforms
// it prevents subnormal floats from causing CPU spikes.
inline float denormalize (float x) noexcept
{
    // Add a tiny signal-dependent offset that pushes subnormals to zero
    // without affecting normal audio range.
    constexpr float tiny = 1.0e-15f;
    return std::abs(x) < tiny ? 0.0f : x;
}

inline double denormalize (double x) noexcept
{
    constexpr double tiny = 1.0e-300;
    return std::abs(x) < tiny ? 0.0 : x;
}

// Apply to biquad state variables
inline void denormalizeBiquadState (float& z1, float& z2) noexcept
{
    constexpr float tiny = 1.0e-15f;
    if (std::abs(z1) < tiny) z1 = 0.0f;
    if (std::abs(z2) < tiny) z2 = 0.0f;
}

inline void denormalizeBiquadState (double& z1, double& z2) noexcept
{
    constexpr double tiny = 1.0e-300;
    if (std::abs(z1) < tiny) z1 = 0.0;
    if (std::abs(z2) < tiny) z2 = 0.0;
}