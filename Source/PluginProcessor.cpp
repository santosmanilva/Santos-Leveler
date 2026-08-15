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
constexpr auto paramBypass        = "bypass";

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
                                       juce::NormalisableRange<float> (-36.0f, -12.0f, 0.5f), -19.0f,
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
                                       juce::NormalisableRange<float> (-12.0f, 0.0f, 0.5f), -12.0f,
                                       juce::AudioParameterFloatAttributes().withLabel ("dB")));

    layout.add (std::make_unique<APF> (juce::ParameterID { paramRangeUp, 1 }, "Range Up",
                                       juce::NormalisableRange<float> (0.0f, 12.0f, 0.5f), 9.0f,
                                       juce::AudioParameterFloatAttributes().withLabel ("dB")));

    layout.add (std::make_unique<APF> (juce::ParameterID { paramOutput, 1 }, "Output",
                                       juce::NormalisableRange<float> (-12.0f, 12.0f, 0.5f), 0.0f,
                                       juce::AudioParameterFloatAttributes().withLabel ("dB")));

    layout.add (std::make_unique<APB> (juce::ParameterID { paramBypass, 1 }, "Bypass", false));

    return layout;
}

void SantosLevelerAudioProcessor::prepareToPlay(double sampleRate, int)
{
    engine.prepare(sampleRate, getTotalNumInputChannels());
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
    setLatencySamples(currentLookaheadSamples);

    const auto bypassed = apvts.getRawParameterValue(paramBypass)->load() >= 0.5f;
    bypassMix = bypassed ? 1.0f : 0.0f;
    const auto bypassSmoothingSeconds = 0.010;
    bypassSmoothingAlpha = static_cast<float> (
        1.0 - std::exp(-1.0 / (bypassSmoothingSeconds * std::max(1.0, sampleRate))));

    gainMatchDryPower = 0.0f;
    gainMatchWetPower = 0.0f;
    gainMatchGain = 1.0f;
    gainMatchTargetGain = 1.0f;

    const auto gainMatchPowerSeconds = 0.750;
    gainMatchPowerAlpha = static_cast<float> (
        1.0 - std::exp(-1.0 / (gainMatchPowerSeconds * std::max(1.0, sampleRate))));

    const auto gainMatchGainSeconds = 0.250;
    gainMatchGainAlpha = static_cast<float> (
        1.0 - std::exp(-1.0 / (gainMatchGainSeconds * std::max(1.0, sampleRate))));

    history.clear();
    historyCounter = 0;
    historyPeriodSamples = std::max(1, static_cast<int> (std::round(sampleRate / 60.0)));
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

    const auto numInputChannels = getTotalNumInputChannels();
    const auto numOutputChannels = getTotalNumOutputChannels();
    const auto numSamples = buffer.getNumSamples();

    for (auto ch = numInputChannels; ch < numOutputChannels; ++ch)
        buffer.clear(ch, 0, numSamples);

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
    p.rangeUpDb = apvts.getRawParameterValue(paramRangeUp)->load();
    p.outputDb = apvts.getRawParameterValue(paramOutput)->load();

    const auto bypassTarget = apvts.getRawParameterValue(paramBypass)->load() >= 0.5f ? 1.0f : 0.0f;

    const auto requestedLookaheadSamples = std::clamp(static_cast<int> (std::round(currentSampleRate * static_cast<double> (p.lookaheadMs) * 0.001)), 0, maxLookaheadSamples);
    if (lookaheadTransitionSamplesRemaining == 0 && requestedLookaheadSamples != targetLookaheadSamples)
    {
        targetLookaheadSamples = requestedLookaheadSamples;
        lookaheadTransitionSamplesRemaining = lookaheadTransitionLengthSamples;
        setLatencySamples(targetLookaheadSamples);
    }

    bool playing = true;
    bool transportKnown = false;
    if (auto* hostPlayHead = getPlayHead())
    {
        if (auto position = hostPlayHead->getPosition())
        {
            transportKnown = true;
            playing = position->getIsPlaying();
        }
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

            delayedL = delayedL + (targetL - delayedL) * progress;
            delayedR = delayedR + (targetR - delayedR) * progress;

            const auto currentHistoryDb = historyInputDbBuffer[static_cast<std::size_t> (currentReadPosition)];
            const auto targetHistoryDb = historyInputDbBuffer[static_cast<std::size_t> (targetReadPosition)];
            historyAlignedInputDb = currentHistoryDb + (targetHistoryDb - currentHistoryDb) * progress;
        }
        else
        {
            historyAlignedInputDb = historyInputDbBuffer[static_cast<std::size_t> (currentReadPosition)];
        }

        const float dryL = delayedL;
        const float dryR = delayedR;

        telemetry = engine.processSampleLookahead(detectorL, detectorR, delayedL, delayedR, p);
        historyInputDbBuffer[static_cast<std::size_t> (lookaheadWritePosition)] = telemetry.inputDb;

        const auto dryPower = numInputChannels > 1
            ? 0.5f * (dryL * dryL + dryR * dryR)
            : dryL * dryL;
        const auto wetPower = numInputChannels > 1
            ? 0.5f * (delayedL * delayedL + delayedR * delayedR)
            : delayedL * delayedL;

        gainMatchDryPower += gainMatchPowerAlpha * (dryPower - gainMatchDryPower);
        gainMatchWetPower += gainMatchPowerAlpha * (wetPower - gainMatchWetPower);

        constexpr float gainMatchMinPower = 3.16227766e-6f; // -55 dBFS RMS
        if (telemetry.gateActive
            && gainMatchDryPower > gainMatchMinPower
            && gainMatchWetPower > gainMatchMinPower)
        {
            const auto gainRatio = std::sqrt(gainMatchWetPower / gainMatchDryPower);
            const auto gainRatioDb = 20.0f * std::log10(std::max(gainRatio, 1.0e-6f));
            const auto safeGainMatchDb = std::clamp(gainRatioDb, -12.0f, 12.0f);
            gainMatchTargetGain = std::pow(10.0f, safeGainMatchDb / 20.0f);
        }

        gainMatchGain += gainMatchGainAlpha * (gainMatchTargetGain - gainMatchGain);

        bypassMix += bypassSmoothingAlpha * (bypassTarget - bypassMix);
        const auto wetMix = 1.0f - bypassMix;
        const auto matchedDryL = dryL * gainMatchGain;
        const auto matchedDryR = dryR * gainMatchGain;

        left[i] = delayedL * wetMix + matchedDryL * bypassMix;
        if (right != nullptr)
            right[i] = delayedR * wetMix + matchedDryR * bypassMix;

        if (lookaheadTransitionSamplesRemaining > 0)
        {
            --lookaheadTransitionSamplesRemaining;
            if (lookaheadTransitionSamplesRemaining == 0)
                currentLookaheadSamples = targetLookaheadSamples;
        }

        ++lookaheadWritePosition;
        if (lookaheadWritePosition >= lookaheadBufferSize) lookaheadWritePosition = 0;

        if (++historyCounter >= historyPeriodSamples)
        {
            historyCounter = 0;
            if (!transportKnown || playing)
            {
                const auto telemetrySnapshot = telemetry;
                const auto minCorrectionDb = std::clamp(p.rangeDownDb, -12.0f, 0.0f);
                const auto maxCorrectionDb = std::clamp(p.rangeUpDb, 0.0f, 12.0f);

                SantosHistoryPoint snapshot;
                snapshot.inputDb = historyAlignedInputDb;
                snapshot.fastDb = telemetrySnapshot.fastDb;
                snapshot.slowDb = telemetrySnapshot.slowDb;
                snapshot.controlDb = telemetrySnapshot.controlDb;
                snapshot.rawRiderDb = std::clamp(
                    p.targetDb - telemetrySnapshot.controlDb,
                    minCorrectionDb,
                    maxCorrectionDb);
                snapshot.requestedRiderDb = telemetrySnapshot.requestedRiderDb;
                snapshot.effectiveRiderDb = telemetrySnapshot.effectiveRiderDb;
                snapshot.riderDb = telemetrySnapshot.riderDb;
                snapshot.peakEnvelopeDb = telemetrySnapshot.peakEnvelopeDb;
                snapshot.peakReductionDb = telemetrySnapshot.peakReductionDb;
                snapshot.peakDb = telemetrySnapshot.peakDb;
                snapshot.outputDb = telemetrySnapshot.outputDb;
                snapshot.gateActive = telemetrySnapshot.gateActive;
                history.push(snapshot);
            }
        }
    }

    inputMeterDb.store(telemetry.inputDb, std::memory_order_relaxed);
    outputMeterDb.store(telemetry.outputDb, std::memory_order_relaxed);
    riderMeterDb.store(telemetry.riderDb, std::memory_order_relaxed);
    riderActive.store(telemetry.riderActive, std::memory_order_relaxed);
}

void SantosLevelerAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    if (auto state = apvts.copyState(); state.isValid())
    {
        juce::MemoryOutputStream stream (destData, false);
        state.writeToStream (stream);
    }
}

void SantosLevelerAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    if (auto state = juce::ValueTree::readFromData (data, static_cast<std::size_t> (sizeInBytes)); state.isValid())
        apvts.replaceState (state);
}

juce::AudioProcessorEditor* SantosLevelerAudioProcessor::createEditor()
{
    return new SantosLevelerAudioProcessorEditor (*this);
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new SantosLevelerAudioProcessor();
}
