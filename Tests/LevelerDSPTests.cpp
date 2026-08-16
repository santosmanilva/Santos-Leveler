#include "../Source/LevelerEngine.h"
#include "../Source/LoudnessMeter.h"
#include "../Source/TruePeakLimiter.h"
#include "../Source/VoiceCompressor.h"

#include <cassert>
#include <cmath>
#include <iostream>

static void runConstantSignal (SantosLevelerEngine& engine,
                               SantosLevelerEngine::Parameters params,
                               float amplitude,
                               int samples,
                               float& lastLeft)
{
    for (int i = 0; i < samples; ++i)
    {
        float l = amplitude;
        float r = amplitude;
        engine.processSample (l, r, params);
        lastLeft = l;
    }
}

int main()
{
    constexpr double sr = 48000.0;
    SantosLevelerEngine engine;
    engine.prepare (sr, 2);

    SantosLevelerEngine::Parameters p;
    p.targetDb = -20.0f;
    p.gateDb = -70.0f;
    p.speedMs = 15.0f;
    p.detectMs = 8.0f;
    p.rangeUpDb = 12.0f;
    p.rangeDownDb = -12.0f;
    p.intensityPercent = 100.0f;
    p.outputDb = 0.0f;

    float y = 0.0f;
    const float inMinus32 = SantosLevelerEngine::dbToGain (-32.0f);
    runConstantSignal (engine, p, inMinus32, static_cast<int> (sr * 0.5), y);
    const auto outDb = SantosLevelerEngine::gainToDb (std::abs (y));
    assert (outDb > -21.0f && outDb < -19.0f);

    engine.reset();
    p.upStrengthPercent = 50.0f;
    runConstantSignal (engine, p, inMinus32, static_cast<int> (sr * 0.5), y);
    const auto halfUpDb = SantosLevelerEngine::gainToDb (std::abs (y));
    assert (halfUpDb > -26.5f && halfUpDb < -25.5f);

    engine.reset();
    p.upStrengthPercent = 100.0f;
    p.downStrengthPercent = 50.0f;
    p.peakThresholdDb = -1.0f;
    const float inMinus8 = SantosLevelerEngine::dbToGain (-8.0f);
    runConstantSignal (engine, p, inMinus8, static_cast<int> (sr * 0.5), y);
    const auto halfDownDb = SantosLevelerEngine::gainToDb (std::abs (y));
    assert (halfDownDb > -14.5f && halfDownDb < -13.5f);

    engine.reset();
    p.downStrengthPercent = 100.0f;
    p.rangeUpDb = 16.0f;
    p.rangeDownDb = -16.0f;
    p.targetDb = -16.0f;
    p.peakThresholdDb = -1.0f;
    runConstantSignal (engine, p, inMinus32, static_cast<int> (sr * 0.5), y);
    const auto extendedUpDb = SantosLevelerEngine::gainToDb (std::abs (y));
    assert (extendedUpDb > -17.0f && extendedUpDb < -15.0f);

    engine.reset();
    p.targetDb = -24.0f;
    runConstantSignal (engine, p, inMinus8, static_cast<int> (sr * 0.5), y);
    const auto extendedDownDb = SantosLevelerEngine::gainToDb (std::abs (y));
    assert (extendedDownDb > -25.0f && extendedDownDb < -23.0f);

    engine.reset();
    p.targetDb = -20.0f;
    p.rangeUpDb = 16.0f;
    p.rangeDownDb = -16.0f;
    p.peakThresholdDb = -9.0f;
    p.intensityPercent = 0.0f;
    runConstantSignal (engine, p, inMinus8, static_cast<int> (sr * 0.75), y);
    const auto zeroIntensityDb = SantosLevelerEngine::gainToDb (std::abs (y));
    const auto zeroIntensityTelemetry = engine.getTelemetry();
    assert (zeroIntensityDb > -8.5f && zeroIntensityDb < -7.5f);
    assert (std::abs (zeroIntensityTelemetry.riderDb) < 0.1f);
    assert (std::abs (zeroIntensityTelemetry.peakDb) < 0.1f);

    engine.reset();
    p.intensityPercent = 100.0f;
    p.rangeUpDb = 0.0f;
    runConstantSignal (engine, p, inMinus32, static_cast<int> (sr * 0.5), y);
    const auto noBoostDb = SantosLevelerEngine::gainToDb (std::abs (y));
    assert (noBoostDb > -32.5f && noBoostDb < -31.5f);

    engine.reset();
    p.rangeUpDb = 12.0f;
    p.gateDb = -25.0f;
    runConstantSignal (engine, p, inMinus32, static_cast<int> (sr * 0.5), y);
    const auto gatedDb = SantosLevelerEngine::gainToDb (std::abs (y));
    assert (gatedDb > -32.5f && gatedDb < -31.5f);

    engine.reset();
    p.gateDb = -70.0f;
    p.outputDb = -6.0f;
    p.targetDb = -32.0f;
    runConstantSignal (engine, p, inMinus32, static_cast<int> (sr * 0.5), y);
    const auto trimDb = SantosLevelerEngine::gainToDb (std::abs (y));
    assert (trimDb > -38.5f && trimDb < -37.5f);

    SantosVoiceCompressor compressor;
    compressor.prepare (sr);
    SantosVoiceCompressor::Parameters cp;
    cp.enabled = false;
    cp.thresholdDb = -18.0f;
    cp.ratio = 3.0f;
    cp.attackMs = 10.0f;
    cp.releaseMs = 120.0f;
    cp.makeupDb = 0.0f;
    float compL = 0.5f;
    float compR = 0.5f;
    for (int i = 0; i < static_cast<int> (sr * 0.25); ++i)
    {
        compL = 0.5f;
        compR = 0.5f;
        compressor.process (compL, compR, cp);
    }
    assert (std::abs (compL - 0.5f) < 1.0e-4f);

    cp.enabled = true;
    for (int i = 0; i < static_cast<int> (sr * 0.5); ++i)
    {
        compL = 0.5f;
        compR = 0.5f;
        compressor.process (compL, compR, cp);
    }
    assert (compressor.getGainReductionDb() < -2.0f);
    assert (compL < 0.5f);

    SantosTruePeakLimiter limiter;
    limiter.prepare (sr, 2);
    assert (limiter.getLatencySamples() == 48);

    constexpr float hotSignal = 1.2f;
    constexpr float dryReference = 0.5f;
    constexpr float ceilingMinus1Linear = 0.891250938f;
    float limitedL = 0.0f;
    float limitedR = 0.0f;
    float delayedDryL = 0.0f;
    float delayedDryR = 0.0f;

    for (int i = 0; i < static_cast<int> (sr * 0.25); ++i)
    {
        limiter.process (hotSignal, hotSignal,
                         dryReference, dryReference,
                         limitedL, limitedR,
                         delayedDryL, delayedDryR);
    }

    assert (std::abs (limitedL) <= ceilingMinus1Linear + 0.002f);
    assert (std::abs (limitedR) <= ceilingMinus1Linear + 0.002f);
    assert (std::abs (delayedDryL - dryReference) < 1.0e-6f);
    assert (std::abs (delayedDryR - dryReference) < 1.0e-6f);
    assert (limiter.getGainReductionDb() < -2.0f);

    limiter.reset();
    limiter.setCeilingDbTP (-3.0f);
    constexpr float ceilingMinus3Linear = 0.707945784f;
    for (int i = 0; i < static_cast<int> (sr * 0.25); ++i)
    {
        limiter.process (hotSignal, hotSignal,
                         dryReference, dryReference,
                         limitedL, limitedR,
                         delayedDryL, delayedDryR);
    }
    assert (std::abs (limitedL) <= ceilingMinus3Linear + 0.002f);
    assert (std::abs (limitedR) <= ceilingMinus3Linear + 0.002f);
    assert (std::abs (limiter.getCeilingDbTP() + 3.0f) < 1.0e-6f);

    SantosLoudnessMeter loudness;
    loudness.prepare (sr, 2);
    constexpr float pi = 3.14159265358979323846f;
    for (int i = 0; i < static_cast<int> (sr * 4.0); ++i)
    {
        const auto sample = 0.1f * std::sin (2.0f * pi * 1000.0f * static_cast<float> (i) / static_cast<float> (sr));
        loudness.processSample (sample, sample);
    }

    assert (loudness.getShortTermLufs() > -20.2f && loudness.getShortTermLufs() < -19.8f);
    assert (loudness.getIntegratedLufs() > -20.2f && loudness.getIntegratedLufs() < -19.8f);
    assert (loudness.getMaxTruePeakDbTP() > -20.2f && loudness.getMaxTruePeakDbTP() < -19.8f);

    loudness.resetIntegratedAndTruePeak();
    assert (loudness.getIntegratedLufs() <= -99.0f);
    assert (loudness.getMaxTruePeakDbTP() <= -99.0f);

    std::cout << "SANTOS LEVELER DSP tests passed.\n";
    return 0;
}
