#pragma once

// Centralized DSP constants for Santos Leveler
// Avoids magic numbers scattered throughout the codebase.

namespace SantosConstants
{
    // Gate hysteresis: close threshold is 3 dB below open threshold
    constexpr float gateHysteresisDb = 3.0f;

    // Gate close grace period in milliseconds
    constexpr float gateCloseGraceMs = 80.0f;

    // Peak envelope detector release time (ms)
    constexpr float peakEnvelopeReleaseMs = 80.0f;

    // Transient preservation thresholds
    constexpr float preserveStartDeltaDb = 3.0f;    // Fast-slow delta where preservation begins
    constexpr float preserveFullDeltaDb = 12.0f;    // Fast-slow delta where preservation is max
    constexpr float maxPreserveAmount = 0.35f;      // Maximum fraction of downward correction preserved (35%)

    // Rider smoothing epsilon (dB) - below this, direction is considered "near zero"
    constexpr float smoothingEpsilonDb = 0.05f;

    // Peak 2 limiter parameters
    constexpr float peakAttackMs = 1.0f;
    constexpr float peakReleaseFastMs = 70.0f;
    constexpr float peakReleaseSlowMs = 160.0f;
    constexpr float maxPeakReductionDb = 9.0f;     // Maximum peak reduction in dB

    // Output trim smoothing time constant (ms)
    constexpr float outputTrimSmoothingMs = 30.0f;

    // Control loop rate (Hz) - gate, hold, and correction computation run at this rate
    constexpr float controlLoopRateHz = 240.0f;

    // Compressor soft knee width (dB)
    constexpr float compressorKneeDb = 6.0f;

    // True peak limiter
    constexpr float truePeakLookaheadMs = 1.0f;     // 1 ms lookahead
    constexpr float truePeakHoldExtraSamples = 6;   // Extra hold samples for FIR group delay
    constexpr float truePeakReleaseMs = 120.0f;     // Release time for true peak limiter

    // Bypass crossfade time (seconds)
    constexpr float bypassCrossfadeSeconds = 0.010f;

    // History push rate (Hz)
    constexpr float historyPushRateHz = 60.0f;

    // Loudness meter
    constexpr float loudnessAbsoluteGateDb = -70.0f;   // Absolute gate for integrated loudness
    constexpr float loudnessRelativeGateOffsetDb = -10.0f; // Relative gate offset from mean
    constexpr float loudnessIntegratedUpdateIntervalSec = 1.0f; // Update integrated LUFS every 1 second

    // Parameter bounds
    constexpr float minTargetDb = -36.0f;
    constexpr float maxTargetDb = -6.0f;
    constexpr float minGateDb = -70.0f;
    constexpr float maxGateDb = -25.0f;
    constexpr float minSpeedMs = 2.0f;
    constexpr float maxSpeedMs = 250.0f;
    constexpr float minDetectMs = 1.0f;
    constexpr float maxDetectMs = 100.0f;
    constexpr float minLookaheadMs = 0.0f;
    constexpr float maxLookaheadMs = 100.0f;
    constexpr float minHoldMs = 0.0f;
    constexpr float maxHoldMs = 1000.0f;
    constexpr float minReleaseMs = 50.0f;
    constexpr float maxReleaseMs = 3000.0f;
    constexpr float minPeakThresholdDb = -18.0f;
    constexpr float maxPeakThresholdDb = -1.0f;
    constexpr float minRangeDownDb = -16.0f;
    constexpr float maxRangeDownDb = 0.0f;
    constexpr float minRangeUpDb = 0.0f;
    constexpr float maxRangeUpDb = 16.0f;
    constexpr float minOutputDb = -12.0f;
    constexpr float maxOutputDb = 12.0f;
    constexpr float minStrengthPercent = 0.0f;
    constexpr float maxStrengthPercent = 100.0f;
    constexpr float minIntensityPercent = 0.0f;
    constexpr float maxIntensityPercent = 100.0f;
    constexpr float minCompThresholdDb = -36.0f;
    constexpr float maxCompThresholdDb = 0.0f;
    constexpr float minCompRatio = 1.0f;
    constexpr float maxCompRatio = 10.0f;
    constexpr float minCompAttackMs = 0.5f;
    constexpr float maxCompAttackMs = 100.0f;
    constexpr float minCompReleaseMs = 20.0f;
    constexpr float maxCompReleaseMs = 1000.0f;
    constexpr float minCompMakeupDb = 0.0f;
    constexpr float maxCompMakeupDb = 12.0f;
    constexpr float minCeilingDbTP = -9.0f;
    constexpr float maxCeilingDbTP = -1.0f;
} // namespace SantosConstants