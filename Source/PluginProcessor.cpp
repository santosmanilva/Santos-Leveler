#include "PluginProcessor.h"
#include "PluginEditor.h"

namespace
{
constexpr auto paramTarget    = "target";
constexpr auto paramGate      = "gate";
constexpr auto paramSpeed     = "speed";
constexpr auto paramDetect    = "detect";
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

void SantosLevelerAudioProcessor::prepareToPlay (double sampleRate, int)
{
    engine.prepare (sampleRate, getTotalNumInputChannels());
    history.clear();
    historyCounter = 0;
    historyPeriodSamples = std::max (1, static_cast<int> (std::round (sampleRate / 60.0)));
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

void SantosLevelerAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;

    const auto numInputChannels = getTotalNumInputChannels();
    const auto numOutputChannels = getTotalNumOutputChannels();
    const auto numSamples = buffer.getNumSamples();

    for (auto ch = numInputChannels; ch < numOutputChannels; ++ch)
        buffer.clear (ch, 0, numSamples);

    SantosLevelerEngine::Parameters p;
    p.targetDb    = apvts.getRawParameterValue (paramTarget)->load();
    p.gateDb      = apvts.getRawParameterValue (paramGate)->load();
    p.speedMs     = apvts.getRawParameterValue (paramSpeed)->load();
    p.detectMs    = apvts.getRawParameterValue (paramDetect)->load();
    p.rangeDownDb = apvts.getRawParameterValue (paramRangeDown)->load();
    p.rangeUpDb   = apvts.getRawParameterValue (paramRangeUp)->load();
    p.outputDb    = apvts.getRawParameterValue (paramOutput)->load();

    bool playing = true;
    bool transportKnown = false;
    if (auto* playHead = getPlayHead())
    {
        if (auto position = playHead->getPosition())
        {
            transportKnown = true;
            playing = position->getIsPlaying();
        }
    }

    transportPlaying.store (playing, std::memory_order_relaxed);
    hostTransportKnown.store (transportKnown, std::memory_order_relaxed);

    auto* left = buffer.getWritePointer (0);
    auto* right = numInputChannels > 1 ? buffer.getWritePointer (1) : nullptr;

    SantosLevelerEngine::Telemetry telemetry;

    for (int i = 0; i < numSamples; ++i)
    {
        float l = left[i];
        float r = right != nullptr ? right[i] : l;

        telemetry = engine.processSample (l, r, p);

        left[i] = l;
        if (right != nullptr)
            right[i] = r;

        if (++historyCounter >= historyPeriodSamples)
        {
            historyCounter = 0;
            // When the DAW reports transport state, History only advances during Play.
            // If no host transport exists (e.g. Standalone), it runs continuously.
            if (! transportKnown || playing)
                history.push ({ telemetry.inputDb, telemetry.riderDb, telemetry.outputDb });
        }
    }

    inputMeterDb.store (telemetry.inputDb, std::memory_order_relaxed);
    outputMeterDb.store (telemetry.outputDb, std::memory_order_relaxed);
    riderMeterDb.store (telemetry.riderDb, std::memory_order_relaxed);
    riderActive.store (telemetry.riderActive, std::memory_order_relaxed);
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
