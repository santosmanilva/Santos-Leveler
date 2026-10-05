#include "PluginEditor.h"
#include "UI/Controls.h"
#include "UI/Displays.h"
#include "UI/HeaderButtons.h"

using namespace SantosUI;

class SantosLevelerAudioProcessorEditor::Content final : public juce::Component
{
public:
    explicit Content (SantosLevelerAudioProcessor& p)
        : processor (p),
          presetButton (p),
          intensity (p.apvts, "intensity", "Intensity", " %", 0),
          inputMeter (p, LevelMeterCard::Source::input, "Input"),
          outputMeter (p, LevelMeterCard::Source::levelerOutput, "Output"),
          graph (p),
          outputPanel (p),
          gate (p.apvts, "gate", "Gate", " dB", 1),
          target (p.apvts, "target", "Target", " dB", 1),
          speed (p.apvts, "speed", "Speed", " ms", 0),
          detect (p.apvts, "detect", "Detect", " ms", 0),
          lookahead (p.apvts, "lookahead", "Lookahead", " ms", 0),
          hold (p.apvts, "hold", "Hold", " ms", 0),
          release (p.apvts, "release", "Release", " ms", 0),
          peak (p.apvts, "peakThreshold", "Peak", " dBFS", 1, colour::danger),
          rangeDown (p.apvts, "rangeDown", "Range down", " dB", 1),
          downStrength (p.apvts, "downStrength", "Down strength", " %", 0),
          rangeUp (p.apvts, "rangeUp", "Range up", " dB", 1),
          upStrength (p.apvts, "upStrength", "Up strength", " %", 0),
          levelerOut (p.apvts, "output", "Leveler out", " dB", 1),
          compThreshold (p.apvts, "compThreshold", "Threshold", " dB", 1),
          compRatio (p.apvts, "compRatio", "Ratio", " :1", 1),
          compAttack (p.apvts, "compAttack", "Attack", " ms", 1),
          compRelease (p.apvts, "compRelease", "Release", " ms", 0),
          compMakeup (p.apvts, "compMakeup", "Makeup", " dB", 1),
          ceiling (p.apvts, "ceiling", "TP ceiling", " dBTP", 1),
          compAttachment (p.apvts, "compEnabled", compSwitch),
          bypassAttachment (p.apvts, "bypass", bypassButton)
    {
        processor.ensureABStatesInitialised();

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

        for (juce::Component* child : std::initializer_list<juce::Component*> { &intensity, &presetButton, &aButton, &bButton,
                                         &bypassButton, &aboutButton, &inputMeter, &outputMeter, &graph, &outputPanel,
                                         &gate, &target, &speed, &detect, &lookahead, &hold, &release, &peak,
                                         &rangeDown, &downStrength, &rangeUp, &upStrength, &levelerOut,
                                         &compSwitch, &compThreshold, &compRatio, &compAttack, &compRelease,
                                         &compMakeup, &ceiling })
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
        graph.repaint();
        inputMeter.repaint();
        outputMeter.repaint();
        outputPanel.repaint();
        repaint (0, 0, static_cast<int> (layout::designWidth), 60);
    }

    void paint (juce::Graphics& g) override
    {
        g.fillAll (colour::window);

        // --- header -----------------------------------------------------------
        g.setColour (colour::text);
        g.setFont (font (22.0f, true));
        g.drawText ("Santos Leveler", juce::Rectangle<float> (22.0f, 8.0f, 230.0f, 28.0f), juce::Justification::centredLeft, false);
        drawCaption (g, "Voice auto level rider", { 23.0f, 36.0f, 230.0f, 14.0f }, juce::Justification::centredLeft, colour::textFaint, 10.0f);

        const auto riding = processor.getRiderActive();
        const auto status = juce::Rectangle<float> (262.0f, 18.0f, 92.0f, 24.0f);
        g.setColour (riding ? colour::accent.withAlpha (0.14f) : colour::cardHigh);
        g.fillRoundedRectangle (status, 12.0f);
        g.setColour (riding ? colour::accent : colour::textFaint);
        g.fillEllipse (juce::Rectangle<float> (8.0f, 8.0f).withCentre ({ status.getX() + 14.0f, status.getCentreY() }));
        drawCaption (g, riding ? "Riding" : "Idle", status.withTrimmedLeft (26.0f), juce::Justification::centredLeft,
                     riding ? colour::accent : colour::textFaint, 10.5f);

        g.setColour (colour::edge);
        g.fillRect (0.0f, 60.0f, layout::designWidth, 1.0f);

        // --- cards ------------------------------------------------------------
        drawCard (g, levelerCard);
        drawCard (g, rangeCard);
        drawCard (g, dynamicsCard);

        drawCaption (g, "Compressor", { dynamicsCard.getX() + 20.0f, dynamicsCard.getY() + 10.0f, 200.0f, 24.0f },
                     juce::Justification::centredLeft, colour::textDim, 12.0f);

        // --- footer -----------------------------------------------------------
        g.setColour (colour::textFaint);
        g.setFont (font (11.0f));
        g.drawText (juce::String ("Santos Leveler v") + JucePlugin_VersionString + "  " + juce::String::charToString (0x00b7) + "  Auto level rider technology",
                    juce::Rectangle<float> (16.0f, 606.0f, 600.0f, 22.0f), juce::Justification::centredLeft, false);
        g.drawText ("Designed & developed by Santos",
                    juce::Rectangle<float> (layout::designWidth - 16.0f - 400.0f, 606.0f, 400.0f, 22.0f), juce::Justification::centredRight, false);
    }

    void resized() override
    {
        constexpr float margin = 16.0f, gap = 12.0f, top = 72.0f;
        constexpr float rightX = 964.0f, rightW = layout::designWidth - margin - rightX;

        // header controls, right to left
        aboutButton.setBounds (1260, 15, 30, 30);
        bypassButton.setBounds (1158, 14, 92, 32);
        bButton.setBounds (1110, 14, 38, 32);
        aButton.setBounds (1072, 14, 38, 32);
        presetButton.setBounds (948, 14, 112, 32);
        intensity.setBounds (754, 8, 170, 44);

        // row A: meters and the live graph
        const auto rowA = juce::Rectangle<int> (16, static_cast<int> (top), 936, 292);
        inputMeter.setBounds (rowA.withWidth (72));
        outputMeter.setBounds (rowA.withLeft (rowA.getRight() - 72));
        graph.setBounds (rowA.withTrimmedLeft (72 + static_cast<int> (gap)).withTrimmedRight (72 + static_cast<int> (gap)));

        // row B: leveler knobs
        levelerCard = { margin, 372.0f, 936.0f, 136.0f };
        {
            const juce::Array<KnobUnit*> knobs { &gate, &target, &speed, &detect, &lookahead, &hold, &release, &peak };
            constexpr int unitWidth = 108, unitHeight = 108;
            const auto startX = static_cast<int> (levelerCard.getX()) + (936 - unitWidth * 8) / 2;
            for (int i = 0; i < knobs.size(); ++i)
                knobs[i]->setBounds (startX + i * unitWidth, static_cast<int> (levelerCard.getY()) + 14, unitWidth, unitHeight);
        }

        // row C: range / strength / output
        rangeCard = { margin, 520.0f, 936.0f, 76.0f };
        {
            const juce::Array<SliderUnit*> sliders { &rangeDown, &downStrength, &rangeUp, &upStrength, &levelerOut };
            constexpr int unitWidth = 168, unitGap = 20;
            const auto startX = static_cast<int> (rangeCard.getX()) + (936 - unitWidth * 5 - unitGap * 4) / 2;
            for (int i = 0; i < sliders.size(); ++i)
                sliders[i]->setBounds (startX + i * (unitWidth + unitGap), static_cast<int> (rangeCard.getY()) + 16, unitWidth, 44);
        }

        // right column: compressor / limiter, output
        dynamicsCard = { rightX, top, rightW, 266.0f };
        compSwitch.setBounds (static_cast<int> (dynamicsCard.getRight()) - 16 - 48, static_cast<int> (dynamicsCard.getY()) + 10, 48, 24);
        {
            const juce::Array<KnobUnit*> knobs { &compThreshold, &compRatio, &compAttack, &compRelease, &compMakeup, &ceiling };
            constexpr int unitWidth = 100, unitHeight = 108;
            const auto startX = static_cast<int> (dynamicsCard.getX()) + (static_cast<int> (rightW) - unitWidth * 3) / 2;
            for (int i = 0; i < knobs.size(); ++i)
                knobs[i]->setBounds (startX + (i % 3) * unitWidth,
                                     static_cast<int> (dynamicsCard.getY()) + 40 + (i / 3) * unitHeight, unitWidth, unitHeight);
        }
        outputPanel.setBounds (static_cast<int> (rightX), 346, static_cast<int> (rightW), 250);
    }

private:
    SantosLevelerAudioProcessor& processor;
    juce::Rectangle<float> levelerCard, rangeCard, dynamicsCard;

    PresetButton presetButton;
    AboutButton aboutButton;
    juce::TextButton aButton, bButton, bypassButton;
    SliderUnit intensity;

    LevelMeterCard inputMeter, outputMeter;
    LiveGraph graph;
    OutputPanel outputPanel;

    KnobUnit gate, target, speed, detect, lookahead, hold, release, peak;
    SliderUnit rangeDown, downStrength, rangeUp, upStrength, levelerOut;
    KnobUnit compThreshold, compRatio, compAttack, compRelease, compMakeup, ceiling;

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
    g.fillAll (colour::window);
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
