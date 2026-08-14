#include "PluginProcessor.h"
#include "PluginEditor.h"

namespace
{
constexpr auto paramTarget    = "target";
constexpr auto paramGate      = "gate";
constexpr auto paramSpeed     = "speed";
constexpr auto paramDetect    = "detect";
constexpr auto paramLookahead = "lookahead";
constexpr auto paramHold      = "hold";
constexpr auto paramRelease   = "release";
constexpr auto paramRangeDown = "rangeDown";
constexpr auto paramRangeUp   = "rangeUp";
constexpr auto paramOutput    = "output";

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
    juce::AudioProcessorValueTreeState::ParameterLayout layout;

    layout.add (std::make_unique<APF> (juce::ParameterID { paramTarget, 1 }, "Target",
                                       juce::NormalisableRange<float> (-36.0f, -12.0f, 0.5f), -20.0f,
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

    layout.add(std::make_unique<APF>(juce::ParameterID{ paramLookahead, 1 }, "Lookahead",
                                      juce::NormalisableRange<float>(0.0f, 100.0f, 1.0f), 30.0f,
                                      juce::AudioParameterFloatAttributes().withLabel("ms")));
    layout.add(std::make_unique<APF>(juce::ParameterID{ paramHold, 1 }, "Hold",
                                      juce::NormalisableRange<float>(0.0f, 1000.0f, 10.0f),
                                      100.0f,
                                      juce::AudioParameterFloatAttributes().withLabel("ms")));

    layout.add(std::make_unique<APF>(juce::ParameterID{ paramRelease, 1 }, "Release",
                                      skewedRange(50.0f, 3000.0f, 500.0f, 10.0f),
                                      500.0f,
                                      juce::AudioParameterFloatAttributes().withLabel("ms")));

    layout.add (std::make_unique<APF> (juce::ParameterID { paramRangeDown, 1 }, "Range Down",
                                       juce::NormalisableRange<float> (-12.0f, 0.0f, 0.5f), -9.0f,
                                       juce::AudioParameterFloatAttributes().withLabel ("dB")));

    layout.add (std::make_unique<APF> (juce::ParameterID { paramRangeUp, 1 }, "Range Up",
                                       juce::NormalisableRange<float> (0.0f, 12.0f, 0.5f), 9.0f,
                                       juce::AudioParameterFloatAttributes().withLabel ("dB")));

    layout.add (std::make_unique<APF> (juce::ParameterID { paramOutput, 1 }, "Output",
                                       juce::NormalisableRange<float> (-12.0f, 12.0f, 0.5f), 0.0f,
                                       juce::AudioParameterFloatAttributes().withLabel ("dB")));

    return layout;
}

void SantosLevelerAudioProcessor::prepareToPlay(double sampleRate, int)
{
    engine.prepare(sampleRate, getTotalNumInputChannels());

    currentSampleRate = sampleRate;

    // Reserva suficiente memoria para un máximo de 100 ms de Lookahead.
    maxLookaheadSamples = std::max(
        0,
        static_cast<int> (std::ceil(sampleRate * 0.100)));

    lookaheadBufferSize = maxLookaheadSamples + 1;

    lookaheadBuffer.setSize(
        std::max(1, getTotalNumInputChannels()),
        lookaheadBufferSize);

    lookaheadBuffer.clear();
    lookaheadWritePosition = 0;

    const auto lookaheadMs =
        apvts.getRawParameterValue(paramLookahead)->load();

    currentLookaheadSamples = std::clamp(
        static_cast<int> (
            std::round(sampleRate
                * static_cast<double> (lookaheadMs)
                * 0.001)),
        0,
        maxLookaheadSamples);

    // Comunicamos al DAW la latencia introducida por el Lookahead.
    setLatencySamples(currentLookaheadSamples);

    history.clear();
    historyCounter = 0;

    historyPeriodSamples = std::max(
        1,
        static_cast<int> (std::round(sampleRate / 60.0)));
}
void SantosLevelerAudioProcessor::releaseResources()
{
}

bool SantosLevelerAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    const auto in = layouts.getMainInputChannelSet();
    const auto out = layouts.getMainOutputChannelSet();

    if (in != out)
        return false;

    return in == juce::AudioChannelSet::mono() || in == juce::AudioChannelSet::stereo();
}

void SantosLevelerAudioProcessor::processBlock(
    juce::AudioBuffer<float>& buffer,
    juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;

    const auto numInputChannels = getTotalNumInputChannels();
    const auto numOutputChannels = getTotalNumOutputChannels();
    const auto numSamples = buffer.getNumSamples();

    for (auto ch = numInputChannels;
        ch < numOutputChannels;
        ++ch)
    {
        buffer.clear(ch, 0, numSamples);
    }

    SantosLevelerEngine::Parameters p;

    p.targetDb =
        apvts.getRawParameterValue(paramTarget)->load();

    p.gateDb =
        apvts.getRawParameterValue(paramGate)->load();

    p.speedMs =
        apvts.getRawParameterValue(paramSpeed)->load();

    p.detectMs =
        apvts.getRawParameterValue(paramDetect)->load();

    p.lookaheadMs =
        apvts.getRawParameterValue(paramLookahead)->load();

    p.holdMs =
        apvts.getRawParameterValue(paramHold)->load();

    p.releaseMs =
        apvts.getRawParameterValue(paramRelease)->load();

    p.rangeDownDb =
        apvts.getRawParameterValue(paramRangeDown)->load();

    p.rangeUpDb =
        apvts.getRawParameterValue(paramRangeUp)->load();

    p.outputDb =
        apvts.getRawParameterValue(paramOutput)->load();


    // ------------------------------------------------------------
    // LOOKAHEAD
    // ------------------------------------------------------------

    const auto lookaheadMs =
        apvts.getRawParameterValue(paramLookahead)->load();

    const auto requestedLookaheadSamples = std::clamp(
        static_cast<int> (
            std::round(currentSampleRate
                * static_cast<double> (lookaheadMs)
                * 0.001)),
        0,
        maxLookaheadSamples);

    if (requestedLookaheadSamples != currentLookaheadSamples)
    {
        currentLookaheadSamples = requestedLookaheadSamples;

        // Informa al DAW para que actualice la compensación de latencia.
        setLatencySamples(currentLookaheadSamples);
    }


    // ------------------------------------------------------------
    // TRANSPORTE
    // ------------------------------------------------------------

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

    transportPlaying.store(
        playing,
        std::memory_order_relaxed);

    hostTransportKnown.store(
        transportKnown,
        std::memory_order_relaxed);


    // ------------------------------------------------------------
    // AUDIO
    // ------------------------------------------------------------

    auto* left =
        buffer.getWritePointer(0);

    auto* right =
        numInputChannels > 1
        ? buffer.getWritePointer(1)
        : nullptr;

    auto* delayLeft =
        lookaheadBuffer.getWritePointer(0);

    auto* delayRight =
        numInputChannels > 1
        ? lookaheadBuffer.getWritePointer(1)
        : nullptr;

    SantosLevelerEngine::Telemetry telemetry;


    for (int i = 0; i < numSamples; ++i)
    {
        // Señal que ve el detector AHORA.
        const float detectorL = left[i];

        const float detectorR =
            right != nullptr
            ? right[i]
            : detectorL;


        // Guardamos la señal actual en el buffer circular.
        delayLeft[lookaheadWritePosition] = detectorL;

        if (delayRight != nullptr)
            delayRight[lookaheadWritePosition] = detectorR;


        // Calculamos qué muestra corresponde al audio retrasado.
        auto readPosition =
            lookaheadWritePosition - currentLookaheadSamples;

        if (readPosition < 0)
            readPosition += lookaheadBufferSize;


        // Esta es la señal que realmente llegará a la salida.
        float delayedL = delayLeft[readPosition];

        float delayedR =
            delayRight != nullptr
            ? delayRight[readPosition]
            : delayedL;


        // El detector ve la señal actual.
        // La ganancia se aplica a la señal retrasada.
        telemetry = engine.processSampleLookahead(
            detectorL,
            detectorR,
            delayedL,
            delayedR,
            p);


        left[i] = delayedL;

        if (right != nullptr)
            right[i] = delayedR;


        // Avanzamos el buffer circular.
        ++lookaheadWritePosition;

        if (lookaheadWritePosition >= lookaheadBufferSize)
            lookaheadWritePosition = 0;


        // History
        if (++historyCounter >= historyPeriodSamples)
        {
            historyCounter = 0;

            if (!transportKnown || playing)
            {
                history.push({
                    telemetry.inputDb,
                    telemetry.riderDb,
                    telemetry.outputDb
                    });
            }
        }
    }


    inputMeterDb.store(
        telemetry.inputDb,
        std::memory_order_relaxed);

    outputMeterDb.store(
        telemetry.outputDb,
        std::memory_order_relaxed);

    riderMeterDb.store(
        telemetry.riderDb,
        std::memory_order_relaxed);

    riderActive.store(
        telemetry.riderActive,
        std::memory_order_relaxed);
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
