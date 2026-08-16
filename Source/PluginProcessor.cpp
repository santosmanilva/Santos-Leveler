#include "PluginProcessor.h"
#include "PluginEditor.h"

namespace
{
constexpr auto paramTarget        = "target";
constexpr auto paramGate          = "gate";
constexpr auto paramSpeed         = "speed";
constexpr auto paramDetect        = "detect";
constexpr auto paramLookahead     = "lookahead";
constexpr auto paramHold          = "hold";
constexpr auto paramRelease       = "release";
constexpr auto paramPeakThreshold = "peakThreshold";
constexpr auto paramRangeDown     = "rangeDown";
constexpr auto paramRangeUp       = "rangeUp";
constexpr auto paramOutput        = "output";
constexpr auto paramDownStrength  = "downStrength";
constexpr auto paramUpStrength    = "upStrength";
constexpr auto paramIntensity     = "intensity";
constexpr auto paramCompEnabled   = "compEnabled";
constexpr auto paramCompThreshold = "compThreshold";
constexpr auto paramCompRatio     = "compRatio";
constexpr auto paramCompAttack    = "compAttack";
constexpr auto paramCompRelease   = "compRelease";
constexpr auto paramCompMakeup    = "compMakeup";
constexpr auto paramCeiling       = "ceiling";
constexpr auto paramBypass        = "bypass";

constexpr std::array<const char*, 21> abParameterIds {
    paramTarget, paramGate, paramSpeed, paramDetect, paramLookahead, paramHold, paramRelease,
    paramPeakThreshold, paramRangeDown, paramRangeUp, paramOutput, paramDownStrength,
    paramUpStrength, paramIntensity, paramCompEnabled, paramCompThreshold, paramCompRatio,
    paramCompAttack, paramCompRelease, paramCompMakeup, paramCeiling
};

juce::Identifier abPropertyName (const char* prefix, std::size_t index)
{
    return juce::Identifier (juce::String (prefix) + juce::String (static_cast<int> (index)));
}

juce::NormalisableRange<float> skewedRange (float start, float end, float centre, float interval)
{
    juce::NormalisableRange<float> r (start, end, interval);
    r.setSkewForCentre (centre);
    return r;
}
}

SantosLevelerAudioProcessor::SantosLevelerAudioProcessor()
    : AudioProcessor (BusesProperties()
                        .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                        .withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "PARAMETERS", createParameterLayout())
{
}

juce::AudioProcessorValueTreeState::ParameterLayout SantosLevelerAudioProcessor::createParameterLayout()
{
    using APF = juce::AudioParameterFloat;
    using APB = juce::AudioParameterBool;
    juce::AudioProcessorValueTreeState::ParameterLayout layout;

    layout.add (std::make_unique<APF> (juce::ParameterID { paramTarget, 1 }, "Target",
                                       juce::NormalisableRange<float> (-36.0f, -6.0f, 0.5f), -19.0f,
                                       juce::AudioParameterFloatAttributes().withLabel ("dB")));
    layout.add (std::make_unique<APF> (juce::ParameterID { paramGate, 1 }, "Gate",
                                       juce::NormalisableRange<float> (-70.0f, -25.0f, 0.5f), -45.0f,
                                       juce::AudioParameterFloatAttributes().withLabel ("dB")));
    layout.add (std::make_unique<APF> (juce::ParameterID { paramSpeed, 1 }, "Speed",
                                       skewedRange (2.0f, 250.0f, 20.0f, 1.0f), 15.0f,
                                       juce::AudioParameterFloatAttributes().withLabel ("ms")));
    layout.add (std::make_unique<APF> (juce::ParameterID { paramDetect, 1 }, "Detect",
                                       skewedRange (1.0f, 100.0f, 10.0f, 1.0f), 8.0f,
                                       juce::AudioParameterFloatAttributes().withLabel ("ms")));
    layout.add (std::make_unique<APF> (juce::ParameterID { paramLookahead, 1 }, "Lookahead",
                                       juce::NormalisableRange<float> (0.0f, 100.0f, 1.0f), 30.0f,
                                       juce::AudioParameterFloatAttributes().withLabel ("ms")));
    layout.add (std::make_unique<APF> (juce::ParameterID { paramHold, 1 }, "Hold",
                                       juce::NormalisableRange<float> (0.0f, 1000.0f, 10.0f), 50.0f,
                                       juce::AudioParameterFloatAttributes().withLabel ("ms")));
    layout.add (std::make_unique<APF> (juce::ParameterID { paramRelease, 1 }, "Release",
                                       skewedRange (50.0f, 3000.0f, 500.0f, 10.0f), 500.0f,
                                       juce::AudioParameterFloatAttributes().withLabel ("ms")));
    layout.add (std::make_unique<APF> (juce::ParameterID { paramPeakThreshold, 1 }, "Peak Threshold",
                                       juce::NormalisableRange<float> (-18.0f, -1.0f, 0.5f), -9.0f,
                                       juce::AudioParameterFloatAttributes().withLabel ("dBFS")));
    layout.add (std::make_unique<APF> (juce::ParameterID { paramRangeDown, 1 }, "Range Down",
                                       juce::NormalisableRange<float> (-16.0f, 0.0f, 0.5f), -12.0f,
                                       juce::AudioParameterFloatAttributes().withLabel ("dB")));
    layout.add (std::make_unique<APF> (juce::ParameterID { paramRangeUp, 1 }, "Range Up",
                                       juce::NormalisableRange<float> (0.0f, 16.0f, 0.5f), 9.0f,
                                       juce::AudioParameterFloatAttributes().withLabel ("dB")));
    layout.add (std::make_unique<APF> (juce::ParameterID { paramOutput, 1 }, "Leveler Out",
                                       juce::NormalisableRange<float> (-12.0f, 12.0f, 0.5f), 0.0f,
                                       juce::AudioParameterFloatAttributes().withLabel ("dB")));
    layout.add (std::make_unique<APF> (juce::ParameterID { paramDownStrength, 1 }, "Down Strength",
                                       juce::NormalisableRange<float> (0.0f, 100.0f, 1.0f), 100.0f,
                                       juce::AudioParameterFloatAttributes().withLabel ("%")));
    layout.add (std::make_unique<APF> (juce::ParameterID { paramUpStrength, 1 }, "Up Strength",
                                       juce::NormalisableRange<float> (0.0f, 100.0f, 1.0f), 100.0f,
                                       juce::AudioParameterFloatAttributes().withLabel ("%")));
    layout.add (std::make_unique<APF> (juce::ParameterID { paramIntensity, 1 }, "Intensity",
                                       juce::NormalisableRange<float> (0.0f, 100.0f, 1.0f), 100.0f,
                                       juce::AudioParameterFloatAttributes().withLabel ("%")));

    layout.add (std::make_unique<APB> (juce::ParameterID { paramCompEnabled, 1 }, "Compressor", false));
    layout.add (std::make_unique<APF> (juce::ParameterID { paramCompThreshold, 1 }, "Comp Threshold",
                                       juce::NormalisableRange<float> (-36.0f, 0.0f, 0.5f), -18.0f,
                                       juce::AudioParameterFloatAttributes().withLabel ("dB")));
    layout.add (std::make_unique<APF> (juce::ParameterID { paramCompRatio, 1 }, "Comp Ratio",
                                       juce::NormalisableRange<float> (1.0f, 10.0f, 0.1f), 3.0f,
                                       juce::AudioParameterFloatAttributes().withLabel (":1")));
    layout.add (std::make_unique<APF> (juce::ParameterID { paramCompAttack, 1 }, "Comp Attack",
                                       skewedRange (0.5f, 100.0f, 10.0f, 0.5f), 10.0f,
                                       juce::AudioParameterFloatAttributes().withLabel ("ms")));
    layout.add (std::make_unique<APF> (juce::ParameterID { paramCompRelease, 1 }, "Comp Release",
                                       skewedRange (20.0f, 1000.0f, 120.0f, 5.0f), 120.0f,
                                       juce::AudioParameterFloatAttributes().withLabel ("ms")));
    layout.add (std::make_unique<APF> (juce::ParameterID { paramCompMakeup, 1 }, "Comp Makeup",
                                       juce::NormalisableRange<float> (0.0f, 12.0f, 0.5f), 0.0f,
                                       juce::AudioParameterFloatAttributes().withLabel ("dB")));
    layout.add (std::make_unique<APF> (juce::ParameterID { paramCeiling, 1 }, "Ceiling",
                                       juce::NormalisableRange<float> (-9.0f, -1.0f, 0.5f), -1.0f,
                                       juce::AudioParameterFloatAttributes().withLabel ("dBTP")));
    layout.add (std::make_unique<APB> (juce::ParameterID { paramBypass, 1 }, "Bypass", false));
    return layout;
}

SantosLevelerAudioProcessor::ABState SantosLevelerAudioProcessor::captureCurrentABState() const
{
    ABState state {};
    for (std::size_t i = 0; i < abParameterIds.size(); ++i)
        if (auto* value = apvts.getRawParameterValue (abParameterIds[i])) state[i] = value->load();
    return state;
}

void SantosLevelerAudioProcessor::applyABState (const ABState& state)
{
    for (std::size_t i = 0; i < abParameterIds.size(); ++i)
        if (auto* parameter = apvts.getParameter (abParameterIds[i]))
        {
            parameter->beginChangeGesture();
            parameter->setValueNotifyingHost (parameter->convertTo0to1 (state[i]));
            parameter->endChangeGesture();
        }
}

void SantosLevelerAudioProcessor::ensureABStatesInitialised()
{
    const juce::ScopedLock lock (abStateLock);
    if (abStatesInitialised) return;
    const auto current = captureCurrentABState();
    abStateA = current; abStateB = current;
    abStateBSelected.store (false, std::memory_order_relaxed);
    abStatesInitialised = true;
}

void SantosLevelerAudioProcessor::selectABState (bool useB)
{
    ABState stateToApply {};
    {
        const juce::ScopedLock lock (abStateLock);
        if (! abStatesInitialised)
        {
            const auto current = captureCurrentABState();
            abStateA = current; abStateB = current;
            abStateBSelected.store (false, std::memory_order_relaxed);
            abStatesInitialised = true;
        }
        const auto currentlyUsingB = abStateBSelected.load (std::memory_order_relaxed);
        if (currentlyUsingB == useB) return;
        if (currentlyUsingB) abStateB = captureCurrentABState(); else abStateA = captureCurrentABState();
        abStateBSelected.store (useB, std::memory_order_relaxed);
        stateToApply = useB ? abStateB : abStateA;
    }
    applyABState (stateToApply);
}

void SantosLevelerAudioProcessor::prepareToPlay(double sampleRate, int)
{
    engine.prepare(sampleRate, getTotalNumInputChannels());
    voiceCompressor.prepare(sampleRate);
    truePeakLimiter.prepare(sampleRate, getTotalNumInputChannels());
    truePeakLimiter.setCeilingDbTP(apvts.getRawParameterValue(paramCeiling)->load());
    loudnessMeter.prepare(sampleRate, getTotalNumInputChannels());
    currentSampleRate = sampleRate;

    maxLookaheadSamples = std::max(0, static_cast<int> (std::ceil(sampleRate * 0.100)));
    lookaheadBufferSize = maxLookaheadSamples + 1;
    lookaheadBuffer.setSize(std::max(1, getTotalNumInputChannels()), lookaheadBufferSize);
    lookaheadBuffer.clear();
    historyInputDbBuffer.assign(static_cast<std::size_t> (lookaheadBufferSize), -100.0f);
    lookaheadWritePosition = 0;

    const auto lookaheadMs = apvts.getRawParameterValue(paramLookahead)->load();
    currentLookaheadSamples = std::clamp(static_cast<int> (std::round(sampleRate * static_cast<double> (lookaheadMs) * 0.001)), 0, maxLookaheadSamples);
    targetLookaheadSamples = currentLookaheadSamples;
    lookaheadTransitionLengthSamples = std::max(1, static_cast<int> (std::round(sampleRate * 0.008)));
    lookaheadTransitionSamplesRemaining = 0;
    setLatencySamples(currentLookaheadSamples + truePeakLimiter.getLatencySamples());

    const auto bypassed = apvts.getRawParameterValue(paramBypass)->load() >= 0.5f;
    bypassMix = bypassed ? 1.0f : 0.0f;
    const auto bypassSmoothingSeconds = 0.010;
    bypassSmoothingAlpha = static_cast<float> (1.0 - std::exp(-1.0 / (bypassSmoothingSeconds * std::max(1.0, sampleRate))));
    const auto finalMeterSeconds = 0.100;
    finalOutputMeterAlpha = static_cast<float> (1.0 - std::exp(-1.0 / (finalMeterSeconds * std::max(1.0, sampleRate))));
    finalOutputMeanSquare = 0.0f;

    history.clear();
    historyCounter = 0;
    historyPeriodSamples = std::max(1, static_cast<int> (std::round(sampleRate / 60.0)));
    compressorReductionDb.store(0.0f, std::memory_order_relaxed);
    finalOutputMeterDb.store(-100.0f, std::memory_order_relaxed);
    shortTermLufs.store(-100.0f, std::memory_order_relaxed);
    integratedLufs.store(-100.0f, std::memory_order_relaxed);
    outputTruePeakDbTP.store(-100.0f, std::memory_order_relaxed);
}

void SantosLevelerAudioProcessor::releaseResources() {}

bool SantosLevelerAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    const auto in = layouts.getMainInputChannelSet();
    const auto out = layouts.getMainOutputChannelSet();
    if (in != out) return false;
    return in == juce::AudioChannelSet::mono() || in == juce::AudioChannelSet::stereo();
}

void SantosLevelerAudioProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;
    if (loudnessResetRequested.exchange(false, std::memory_order_acq_rel)) loudnessMeter.resetIntegratedAndTruePeak();

    const auto numInputChannels = getTotalNumInputChannels();
    const auto numOutputChannels = getTotalNumOutputChannels();
    const auto numSamples = buffer.getNumSamples();
    for (auto ch = numInputChannels; ch < numOutputChannels; ++ch) buffer.clear(ch, 0, numSamples);

    SantosLevelerEngine::Parameters p;
    p.targetDb = apvts.getRawParameterValue(paramTarget)->load();
    p.gateDb = apvts.getRawParameterValue(paramGate)->load();
    p.speedMs = apvts.getRawParameterValue(paramSpeed)->load();
    p.detectMs = apvts.getRawParameterValue(paramDetect)->load();
    p.lookaheadMs = apvts.getRawParameterValue(paramLookahead)->load();
    p.holdMs = apvts.getRawParameterValue(paramHold)->load();
    p.releaseMs = apvts.getRawParameterValue(paramRelease)->load();
    p.peakThresholdDb = apvts.getRawParameterValue(paramPeakThreshold)->load();
    p.rangeDownDb = apvts.getRawParameterValue(paramRangeDown)->load();
    p.downStrengthPercent = apvts.getRawParameterValue(paramDownStrength)->load();
    p.rangeUpDb = apvts.getRawParameterValue(paramRangeUp)->load();
    p.upStrengthPercent = apvts.getRawParameterValue(paramUpStrength)->load();
    p.intensityPercent = apvts.getRawParameterValue(paramIntensity)->load();
    p.outputDb = apvts.getRawParameterValue(paramOutput)->load();

    SantosVoiceCompressor::Parameters cp;
    cp.enabled = apvts.getRawParameterValue(paramCompEnabled)->load() >= 0.5f;
    cp.thresholdDb = apvts.getRawParameterValue(paramCompThreshold)->load();
    cp.ratio = apvts.getRawParameterValue(paramCompRatio)->load();
    cp.attackMs = apvts.getRawParameterValue(paramCompAttack)->load();
    cp.releaseMs = apvts.getRawParameterValue(paramCompRelease)->load();
    cp.makeupDb = apvts.getRawParameterValue(paramCompMakeup)->load();

    truePeakLimiter.setCeilingDbTP(apvts.getRawParameterValue(paramCeiling)->load());
    const auto bypassTarget = apvts.getRawParameterValue(paramBypass)->load() >= 0.5f ? 1.0f : 0.0f;

    const auto requestedLookaheadSamples = std::clamp(static_cast<int> (std::round(currentSampleRate * static_cast<double> (p.lookaheadMs) * 0.001)), 0, maxLookaheadSamples);
    if (lookaheadTransitionSamplesRemaining == 0 && requestedLookaheadSamples != targetLookaheadSamples)
    {
        targetLookaheadSamples = requestedLookaheadSamples;
        lookaheadTransitionSamplesRemaining = lookaheadTransitionLengthSamples;
        setLatencySamples(targetLookaheadSamples + truePeakLimiter.getLatencySamples());
    }

    bool playing = true;
    bool transportKnown = false;
    if (auto* hostPlayHead = getPlayHead()) if (auto position = hostPlayHead->getPosition())
    {
        transportKnown = true;
        playing = position->getIsPlaying();
    }
    transportPlaying.store(playing, std::memory_order_relaxed);
    hostTransportKnown.store(transportKnown, std::memory_order_relaxed);

    auto* left = buffer.getWritePointer(0);
    auto* right = numInputChannels > 1 ? buffer.getWritePointer(1) : nullptr;
    auto* delayLeft = lookaheadBuffer.getWritePointer(0);
    auto* delayRight = numInputChannels > 1 ? lookaheadBuffer.getWritePointer(1) : nullptr;

    SantosLevelerEngine::Telemetry telemetry;
    float historyAlignedInputDb = -100.0f;

    auto wrapReadPosition = [this] (int position)
    {
        while (position < 0) position += lookaheadBufferSize;
        while (position >= lookaheadBufferSize) position -= lookaheadBufferSize;
        return position;
    };

    for (int i = 0; i < numSamples; ++i)
    {
        const float detectorL = left[i];
        const float detectorR = right != nullptr ? right[i] : detectorL;
        delayLeft[lookaheadWritePosition] = detectorL;
        if (delayRight != nullptr) delayRight[lookaheadWritePosition] = detectorR;

        const auto currentReadPosition = wrapReadPosition(lookaheadWritePosition - currentLookaheadSamples);
        float delayedL = delayLeft[currentReadPosition];
        float delayedR = delayRight != nullptr ? delayRight[currentReadPosition] : delayedL;

        if (lookaheadTransitionSamplesRemaining > 0)
        {
            const auto targetReadPosition = wrapReadPosition(lookaheadWritePosition - targetLookaheadSamples);
            const float targetL = delayLeft[targetReadPosition];
            const float targetR = delayRight != nullptr ? delayRight[targetReadPosition] : targetL;
            const auto progress = 1.0f - static_cast<float> (lookaheadTransitionSamplesRemaining) / static_cast<float> (lookaheadTransitionLengthSamples);
            delayedL += (targetL - delayedL) * progress;
            delayedR += (targetR - delayedR) * progress;
            const auto currentHistoryDb = historyInputDbBuffer[static_cast<std::size_t> (currentReadPosition)];
            const auto targetHistoryDb = historyInputDbBuffer[static_cast<std::size_t> (targetReadPosition)];
            historyAlignedInputDb = currentHistoryDb + (targetHistoryDb - currentHistoryDb) * progress;
        }
        else historyAlignedInputDb = historyInputDbBuffer[static_cast<std::size_t> (currentReadPosition)];

        const float dryL = delayedL;
        const float dryR = delayedR;
        telemetry = engine.processSampleLookahead(detectorL, detectorR, delayedL, delayedR, p);
        historyInputDbBuffer[static_cast<std::size_t> (lookaheadWritePosition)] = telemetry.inputDb;

        voiceCompressor.process(delayedL, delayedR, cp);

        float limitedWetL = 0.0f, limitedWetR = 0.0f, limiterDryL = 0.0f, limiterDryR = 0.0f;
        truePeakLimiter.process(delayedL, delayedR, dryL, dryR,
                                limitedWetL, limitedWetR, limiterDryL, limiterDryR);

        bypassMix += bypassSmoothingAlpha * (bypassTarget - bypassMix);
        const auto wetMix = 1.0f - bypassMix;
        left[i] = limitedWetL * wetMix + limiterDryL * bypassMix;
        const auto finalR = right != nullptr ? limitedWetR * wetMix + limiterDryR * bypassMix : left[i];
        if (right != nullptr) right[i] = finalR;

        const auto square = numInputChannels > 1
            ? 0.5f * (left[i] * left[i] + finalR * finalR)
            : left[i] * left[i];
        finalOutputMeanSquare += finalOutputMeterAlpha * (square - finalOutputMeanSquare);

        if (!transportKnown || playing) loudnessMeter.processSample(left[i], finalR);

        if (lookaheadTransitionSamplesRemaining > 0)
        {
            --lookaheadTransitionSamplesRemaining;
            if (lookaheadTransitionSamplesRemaining == 0) currentLookaheadSamples = targetLookaheadSamples;
        }

        if (++lookaheadWritePosition >= lookaheadBufferSize) lookaheadWritePosition = 0;

        if (++historyCounter >= historyPeriodSamples)
        {
            historyCounter = 0;
            if (!transportKnown || playing)
            {
                const auto minCorrectionDb = std::clamp(p.rangeDownDb, -16.0f, 0.0f);
                const auto maxCorrectionDb = std::clamp(p.rangeUpDb, 0.0f, 16.0f);
                SantosHistoryPoint snapshot;
                snapshot.inputDb = historyAlignedInputDb;
                snapshot.fastDb = telemetry.fastDb;
                snapshot.slowDb = telemetry.slowDb;
                snapshot.controlDb = telemetry.controlDb;
                snapshot.rawRiderDb = std::clamp(p.targetDb - telemetry.controlDb, minCorrectionDb, maxCorrectionDb);
                snapshot.requestedRiderDb = telemetry.requestedRiderDb;
                snapshot.effectiveRiderDb = telemetry.effectiveRiderDb;
                snapshot.riderDb = telemetry.riderDb;
                snapshot.peakEnvelopeDb = telemetry.peakEnvelopeDb;
                snapshot.peakReductionDb = telemetry.peakReductionDb;
                snapshot.peakDb = telemetry.peakDb;
                snapshot.outputDb = telemetry.outputDb;
                snapshot.gateActive = telemetry.gateActive;
                history.push(snapshot);
            }
        }
    }

    inputMeterDb.store(telemetry.inputDb, std::memory_order_relaxed);
    outputMeterDb.store(telemetry.outputDb, std::memory_order_relaxed);
    const auto finalRms = std::sqrt(std::max(0.0f, finalOutputMeanSquare));
    const auto finalDb = finalRms <= 1.0e-8f ? -100.0f : std::max(-100.0f, 20.0f * std::log10(finalRms));
    finalOutputMeterDb.store(finalDb, std::memory_order_relaxed);
    riderMeterDb.store(telemetry.riderDb, std::memory_order_relaxed);
    compressorReductionDb.store(voiceCompressor.getGainReductionDb(), std::memory_order_relaxed);
    riderActive.store(telemetry.riderActive, std::memory_order_relaxed);
    shortTermLufs.store(loudnessMeter.getShortTermLufs(), std::memory_order_relaxed);
    integratedLufs.store(loudnessMeter.getIntegratedLufs(), std::memory_order_relaxed);
    outputTruePeakDbTP.store(loudnessMeter.getMaxTruePeakDbTP(), std::memory_order_relaxed);
}

void SantosLevelerAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    ABState stateA {}, stateB {};
    bool selectedB = false;
    {
        const juce::ScopedLock lock (abStateLock);
        if (! abStatesInitialised)
        {
            const auto current = captureCurrentABState();
            abStateA = current; abStateB = current;
            abStateBSelected.store (false, std::memory_order_relaxed);
            abStatesInitialised = true;
        }
        if (abStateBSelected.load (std::memory_order_relaxed)) abStateB = captureCurrentABState(); else abStateA = captureCurrentABState();
        stateA = abStateA; stateB = abStateB;
        selectedB = abStateBSelected.load (std::memory_order_relaxed);
    }

    if (auto state = apvts.copyState(); state.isValid())
    {
        state.setProperty ("abInitialised", true, nullptr);
        state.setProperty ("abSelectedB", selectedB, nullptr);
        for (std::size_t i = 0; i < abParameterIds.size(); ++i)
        {
            state.setProperty (abPropertyName ("abA", i), stateA[i], nullptr);
            state.setProperty (abPropertyName ("abB", i), stateB[i], nullptr);
        }
        juce::MemoryOutputStream stream (destData, false);
        state.writeToStream (stream);
    }
}

void SantosLevelerAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    if (auto state = juce::ValueTree::readFromData (data, static_cast<std::size_t> (sizeInBytes)); state.isValid())
    {
        apvts.replaceState (state);
        const auto current = captureCurrentABState();
        const juce::ScopedLock lock (abStateLock);
        const auto hasStoredAB = static_cast<bool> (state.getProperty ("abInitialised", false));
        if (hasStoredAB)
        {
            for (std::size_t i = 0; i < abParameterIds.size(); ++i)
            {
                abStateA[i] = static_cast<float> (state.getProperty (abPropertyName ("abA", i), current[i]));
                abStateB[i] = static_cast<float> (state.getProperty (abPropertyName ("abB", i), current[i]));
            }
            abStateBSelected.store (static_cast<bool> (state.getProperty ("abSelectedB", false)), std::memory_order_relaxed);
        }
        else
        {
            abStateA = current; abStateB = current;
            abStateBSelected.store (false, std::memory_order_relaxed);
        }
        abStatesInitialised = true;
    }
}

juce::AudioProcessorEditor* SantosLevelerAudioProcessor::createEditor() { return new SantosLevelerAudioProcessorEditor (*this); }
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() { return new SantosLevelerAudioProcessor(); }
