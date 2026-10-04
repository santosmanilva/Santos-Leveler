#include "../Source/LevelerEngine.h"

#include <algorithm>
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

static float runConstantSignalPeak (SantosLevelerEngine& engine,
                                    SantosLevelerEngine::Parameters params,
                                    float amplitude,
                                    int samples)
{
    float peak = 0.0f;
    for (int i = 0; i < samples; ++i)
    {
        float l = amplitude;
        float r = amplitude;
        engine.processSample (l, r, params);
        peak = std::max (peak, std::abs (l));
    }
    return peak;
}

int main()
{
    constexpr double sr = 48000.0;
    SantosLevelerEngine engine;
    engine.prepare (sr, 2);

    SantosLevelerEngine::Parameters p;
    p.targetDb = -20.0f;
    p.gateDb = -70.0f;
    p.attackMs = 12.0f;
    p.releaseMs = 45.0f;
    p.detectMs = 8.0f;
    p.rangeUpDb = 12.0f;
    p.rangeDownDb = -12.0f;
    // Lookahead off and a ceiling far above these test signals: these four
    // tests isolate the original rider behaviour (Target/Range/Gate/Output),
    // unaffected by the newer lookahead/limiter stage below.
    p.lookaheadMs = 0.0f;
    p.ceilingDb = 0.0f;
    p.outputDb = 0.0f;

    float y = 0.0f;
    const float inMinus32 = SantosLevelerEngine::dbToGain (-32.0f);
    runConstantSignal (engine, p, inMinus32, static_cast<int> (sr * 0.5), y);

    const auto outDb = SantosLevelerEngine::gainToDb (std::abs (y));
    // With a 12 dB error and full Range Up, the output should converge near -20 dB.
    assert (outDb > -21.0f && outDb < -19.0f);

    engine.reset();
    p.rangeUpDb = 0.0f;
    runConstantSignal (engine, p, inMinus32, static_cast<int> (sr * 0.5), y);
    const auto noBoostDb = SantosLevelerEngine::gainToDb (std::abs (y));
    assert (noBoostDb > -32.5f && noBoostDb < -31.5f);

    engine.reset();
    p.rangeUpDb = 12.0f;
    p.gateDb = -25.0f; // input is below gate, so rider must return to unity
    runConstantSignal (engine, p, inMinus32, static_cast<int> (sr * 0.5), y);
    const auto gatedDb = SantosLevelerEngine::gainToDb (std::abs (y));
    assert (gatedDb > -32.5f && gatedDb < -31.5f);

    engine.reset();
    p.gateDb = -70.0f;
    p.outputDb = -6.0f;
    p.targetDb = -32.0f; // no rider correction expected
    runConstantSignal (engine, p, inMinus32, static_cast<int> (sr * 0.5), y);
    const auto trimDb = SantosLevelerEngine::gainToDb (std::abs (y));
    assert (trimDb > -38.5f && trimDb < -37.5f);

    // Safety limiter: push the rider hard toward a loud target with a tight
    // Ceiling and no lookahead to pre-empt it. The output must never cross
    // the ceiling, however aggressively the rider tries to raise gain.
    engine.reset();
    SantosLevelerEngine::Parameters limiterTest;
    limiterTest.targetDb = 0.0f;
    limiterTest.gateDb = -70.0f;
    limiterTest.attackMs = 2.0f;
    limiterTest.releaseMs = 2.0f;
    limiterTest.detectMs = 8.0f;
    limiterTest.rangeUpDb = 12.0f;
    limiterTest.rangeDownDb = -12.0f;
    limiterTest.lookaheadMs = 0.0f;
    limiterTest.ceilingDb = -6.0f;
    limiterTest.outputDb = 0.0f;
    const auto loudInput = SantosLevelerEngine::dbToGain (-3.0f);
    const auto peak = runConstantSignalPeak (engine, limiterTest, loudInput, static_cast<int> (sr * 0.5));
    const auto ceilingLinear = SantosLevelerEngine::dbToGain (-6.0f);
    assert (peak <= ceilingLinear + 0.001f);

    std::cout << "SANTOS LEVELER DSP tests passed.\n";
    return 0;
}
