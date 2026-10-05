#include "PluginEditor.h"
#include "UI/Controls.h"
#include "UI/Displays.h"
#include "UI/HeaderButtons.h"

using namespace SantosUI;

namespace
{
// Faceplate, module frames and engraved titles. Static, so it is cached as an image.
class Backdrop final : public juce::Component
{
public:
    Backdrop()
    {
        setBufferedToImage (true);
        setInterceptsMouseClicks (false, false);
    }

    static juce::Rectangle<float> levels()      { return { 26.0f,  88.0f, 206.0f, 314.0f }; }
    static juce::Rectangle<float> response()    { return { 244.0f, 88.0f, 684.0f, 314.0f }; }
    static juce::Rectangle<float> output()      { return { 940.0f, 88.0f, 344.0f, 314.0f }; }
    static juce::Rectangle<float> leveler()     { return { 26.0f, 414.0f, 680.0f, 206.0f }; }
    static juce::Rectangle<float> compressor()  { return { 718.0f, 414.0f, 566.0f, 206.0f }; }

    void paint (juce::Graphics& g) override
    {
        drawPlate (g, getLocalBounds().toFloat());

        g.setFont (font (22.0f, true).withExtraKerningFactor (0.2f));
        g.setColour (colour::ink);
        g.drawText ("SANTOS", 60, 30, 140, 30, juce::Justification::centredLeft, false);
        g.setColour (colour::accent);
        g.drawText ("LEVELER", 184, 30, 170, 30, juce::Justification::centredLeft, false);
        drawCaption (g, "Voice auto level rider", { 62.0f, 62.0f, 300.0f, 14.0f }, juce::Justification::centredLeft, colour::dim, 8.5f);

        drawRecess (g, levels(), "Levels", colour::ledBlue);
        drawRecess (g, response(), "Live response", colour::accent);
        drawRecess (g, output(), "Output", colour::ledGreen);
        drawRecess (g, leveler(), "Leveler", colour::ledOrange);
        drawRecess (g, compressor(), "Compressor", colour::ledViolet);

        g.setColour (colour::faint);
        g.setFont (font (9.0f));
        g.drawText (juce::String ("v") + JucePlugin_VersionString, juce::Rectangle<float> (compressor().getRight() - 120.0f, compressor().getBottom() - 24.0f, 104.0f, 14.0f),
                    juce::Justification::centredRight, false);
    }
};
} // namespace

class SantosLevelerAudioProcessorEditor::Content final : public juce::Component
{
public:
    explicit Content (SantosLevelerAudioProcessor& p)
        : processor (p),
          presetNavigator (p),
          riderStatus (p, HeaderStatus::Mode::rider),
          latencyStatus (p, HeaderStatus::Mode::latency),
          inputVu (p, VuMeter::Source::input, "Input"),
          outputVu (p, VuMeter::Source::levelerOutput, "Leveler out"),
          screen (p),
          legend (p),
          stats (p),
          outputPanel (p),
          gate (p.apvts, "gate", "Gate", " dB", 1),
          target (p.apvts, "target", "Target", " dB", 1),
          speed (p.apvts, "speed", "Speed", " ms", 0),
          detect (p.apvts, "detect", "Detect", " ms", 0),
          lookahead (p.apvts, "lookahead", "Lookahead", " ms", 0),
          hold (p.apvts, "hold", "Hold", " ms", 0),
          release (p.apvts, "release", "Release", " ms", 0),
          peak (p.apvts, "peakThreshold", "Peak", " dBFS", 1, colour::danger),
          rangeDown (p.apvts, "rangeDown", "Rng dn", " dB", 1),
          downStrength (p.apvts, "downStrength", "Dn str", " %", 0),
          rangeUp (p.apvts, "rangeUp", "Rng up", " dB", 1),
          upStrength (p.apvts, "upStrength", "Up str", " %", 0),
          levelerOut (p.apvts, "output", "Out", " dB", 1),
          compThreshold (p.apvts, "compThreshold", "Threshold", " dB", 1),
          compRatio (p.apvts, "compRatio", "Ratio", " :1", 1),
          compAttack (p.apvts, "compAttack", "Attack", " ms", 1),
          compRelease (p.apvts, "compRelease", "Release", " ms", 0),
          compMakeup (p.apvts, "compMakeup", "Makeup", " dB", 1),
          ceiling (p.apvts, "ceiling", "Ceiling", " dBTP", 1, colour::danger),
          intensity (p.apvts, "intensity", "Intensity", " %", 0, 70),
          compAttachment (p.apvts, "compEnabled", compSwitch),
          bypassAttachment (p.apvts, "bypass", bypassButton)
    {
        processor.ensureABStatesInitialised();

        addAndMakeVisible (backdrop);

        aButton.setButtonText ("A");
        bButton.setButtonText ("B");
        bypassButton.setButtonText ("Bypass");
        bypassButton.setClickingTogglesState (true);
        bypassButton.setColour (juce::TextButton::buttonOnColourId, colour::danger);
        bypassButton.setTooltip ("Latency-aligned bypass");
        aButton.onClick = [this] { processor.selectABState (false); updateABButtons(); };
        bButton.onClick = [this] { processor.selectABState (true);  updateABButtons(); };
        aButton.setTooltip ("Settings A");
        bButton.setTooltip ("Settings B");
        compSwitch.setTooltip ("Compressor on / off");
        updateABButtons();

        for (juce::Component* child : std::initializer_list<juce::Component*> {
                 &presetNavigator, &aButton, &bButton, &bypassButton, &riderStatus, &latencyStatus, &aboutButton,
                 &inputVu, &outputVu, &screen, &legend, &stats, &outputPanel,
                 &gate, &target, &speed, &detect, &lookahead, &hold, &release, &peak,
                 &rangeDown, &downStrength, &rangeUp, &upStrength, &levelerOut,
                 &compSwitch, &compThreshold, &compRatio, &compAttack, &compRelease, &compMakeup, &ceiling, &intensity })
            addAndMakeVisible (child);
    }

    void updateABButtons()
    {
        const auto useB = processor.isABStateB();
        aButton.setToggleState (! useB, juce::dontSendNotification);
        bButton.setToggleState (useB, juce::dontSendNotification);
    }

    void refresh()
    {
        updateABButtons();
        presetNavigator.refresh();
        riderStatus.repaint();
        latencyStatus.repaint();
        inputVu.refresh();
        outputVu.refresh();
        screen.refresh();
        stats.repaint();
        outputPanel.repaint();
    }

    void resized() override
    {
        const auto all = juce::Rectangle<int> (0, 0, static_cast<int> (layout::designWidth), static_cast<int> (layout::designHeight));
        backdrop.setBounds (all);

        // header
        presetNavigator.setBounds (480, 30, 230, 38);
        aButton.setBounds (736, 26, 78, 46);
        bButton.setBounds (820, 26, 78, 46);
        bypassButton.setBounds (904, 26, 112, 46);
        riderStatus.setBounds (368, 30, 100, 40);
        latencyStatus.setBounds (1090, 36, 100, 36);
        aboutButton.setBounds (1236, 34, 32, 32);

        // levels and live response
        inputVu.setBounds (36, 128, 186, 128);
        outputVu.setBounds (36, 268, 186, 128);
        screen.setBounds (254, 126, 664, 204);
        legend.setBounds (672, 92, 244, 20);
        stats.setBounds (272, 342, 640, 50);

        // output
        outputPanel.setBounds (962, 136, 300, 260);

        // leveler
        const juce::Array<KnobUnit*> levelerKnobs { &gate, &target, &speed, &detect, &lookahead, &hold, &release, &peak };
        for (int i = 0; i < levelerKnobs.size(); ++i)
            levelerKnobs[i]->setBounds (37 + 82 * i, 452, KnobUnit::width, 112);

        const juce::Array<SliderUnit*> sliders { &rangeDown, &downStrength, &rangeUp, &upStrength, &levelerOut };
        for (int i = 0; i < sliders.size(); ++i)
            sliders[i]->setBounds (46 + 132 * i, 566, 108, 34);

        // compressor
        const juce::Array<KnobUnit*> compKnobs { &compThreshold, &compRatio, &compAttack, &compRelease, &compMakeup, &ceiling };
        for (int i = 0; i < compKnobs.size(); ++i)
            compKnobs[i]->setBounds (731 + 90 * i, 452, KnobUnit::width, 112);

        compSwitch.setBounds (1196, 420, 76, 24);
        intensity.setBounds (744, 566, 270, 34);
    }

private:
    SantosLevelerAudioProcessor& processor;

    Backdrop backdrop;
    PresetNavigator presetNavigator;
    HeaderStatus riderStatus, latencyStatus;
    AboutButton aboutButton;
    juce::TextButton aButton, bButton, bypassButton;

    VuMeter inputVu, outputVu;
    ScreenFrame screen;
    LegendBar legend;
    StatsRow stats;
    OutputPanel outputPanel;

    KnobUnit gate, target, speed, detect, lookahead, hold, release, peak;
    SliderUnit rangeDown, downStrength, rangeUp, upStrength, levelerOut;
    KnobUnit compThreshold, compRatio, compAttack, compRelease, compMakeup, ceiling;
    SliderUnit intensity;

    juce::ToggleButton compSwitch;
    juce::AudioProcessorValueTreeState::ButtonAttachment compAttachment;
    juce::AudioProcessorValueTreeState::ButtonAttachment bypassAttachment;
};

SantosLevelerAudioProcessorEditor::SantosLevelerAudioProcessorEditor (SantosLevelerAudioProcessor& p)
    : AudioProcessorEditor (&p), processor (p)
{
    setLookAndFeel (&lookAndFeel);

    content = std::make_unique<Content> (p);
    addAndMakeVisible (*content);
    content->sendLookAndFeelChange();   // controls were built before they joined this look-and-feel

    constexpr int width = static_cast<int> (layout::designWidth), height = static_cast<int> (layout::designHeight);
    setResizeLimits (width / 2, height / 2, width * 2, height * 2);
    getConstrainer()->setFixedAspectRatio (layout::designWidth / layout::designHeight);
    setResizable (true, true);
    setSize (width, height);

    startTimerHz (30);
}

SantosLevelerAudioProcessorEditor::~SantosLevelerAudioProcessorEditor()
{
    stopTimer();
    setLookAndFeel (nullptr);
}

void SantosLevelerAudioProcessorEditor::paint (juce::Graphics& g)
{
    g.fillAll (colour::backdrop);
}

void SantosLevelerAudioProcessorEditor::resized()
{
    content->setBounds (0, 0, static_cast<int> (layout::designWidth), static_cast<int> (layout::designHeight));
    content->setTransform (juce::AffineTransform::scale (static_cast<float> (getWidth()) / layout::designWidth));
}

void SantosLevelerAudioProcessorEditor::timerCallback()
{
    content->refresh();
}
