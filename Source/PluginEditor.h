#pragma once

#include <JuceHeader.h>
#include <array>
#include "PluginProcessor.h"

class SantosLevelerAudioProcessorEditor final : public juce::AudioProcessorEditor,
                                                 private juce::Timer
{
public:
    explicit SantosLevelerAudioProcessorEditor (SantosLevelerAudioProcessor&);
    ~SantosLevelerAudioProcessorEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;
    void parentHierarchyChanged() override { ensurePresetButtonVisible(); }
    void visibilityChanged() override { ensurePresetButtonVisible(); }

private:
    class SantosLookAndFeel final : public juce::LookAndFeel_V4
    {
    public:
        SantosLookAndFeel();
        void drawRotarySlider (juce::Graphics&, int x, int y, int width, int height,
                               float sliderPos, float rotaryStartAngle, float rotaryEndAngle,
                               juce::Slider&) override;
        void drawLinearSlider (juce::Graphics&, int x, int y, int width, int height,
                               float sliderPos, float minSliderPos, float maxSliderPos,
                               juce::Slider::SliderStyle, juce::Slider&) override;
    };

    class HistoryComponent final : public juce::Component
    {
    public:
        explicit HistoryComponent (SantosLevelerAudioProcessor& p) : processor (p) {}
        void paint (juce::Graphics&) override;
    private:
        SantosLevelerAudioProcessor& processor;
    };

    class MeterComponent final : public juce::Component
    {
    public:
        enum class Source { input, levelerOutput };
        MeterComponent (SantosLevelerAudioProcessor& p, Source s, juce::String text, juce::Colour c)
            : processor (p), source (s), label (std::move (text)), colour (c) {}
        void paint (juce::Graphics&) override;
    private:
        SantosLevelerAudioProcessor& processor;
        Source source;
        juce::String label;
        juce::Colour colour;
        float heldPeakDb = -100.0f;
        double peakHoldUntilMs = 0.0;
        double lastPeakUpdateMs = 0.0;
    };

    class FinalMeterComponent final : public juce::Component
    {
    public:
        explicit FinalMeterComponent (SantosLevelerAudioProcessor& p) : processor (p) {}
        void paint (juce::Graphics&) override;
    private:
        SantosLevelerAudioProcessor& processor;
        float heldPeakDb = -100.0f;
        double peakHoldUntilMs = 0.0;
        double lastPeakUpdateMs = 0.0;
    };

    class LoudnessMeterComponent final : public juce::Component
    {
    public:
        explicit LoudnessMeterComponent (SantosLevelerAudioProcessor& p) : processor (p) {}
        void paint (juce::Graphics&) override;
    private:
        SantosLevelerAudioProcessor& processor;
    };

    class AboutButton final : public juce::Button,
                              private juce::ComponentListener
    {
    public:
        AboutButton() : juce::Button ("About Santos Leveler")
        {
            setMouseCursor (juce::MouseCursor::PointingHandCursor);
            setTooltip ("About Santos Leveler");
        }

        ~AboutButton() override
        {
            if (observedParent != nullptr)
                observedParent->removeComponentListener (this);
        }

        void paintButton (juce::Graphics&, bool, bool) override {}

        void clicked() override
        {
            const juce::String message =
                "Version 1.0.0\n\n"
                "Voice Auto Level Rider\n"
                "VST3 Audio Plugin - Windows x64\n\n"
                "Designed & Developed by\n"
                "José Antonio Santos Santos\n\n"
                "Developed with assistance from ChatGPT by OpenAI\n\n"
                "FREEWARE\n"
                "Free of charge for personal and professional use\n\n"
                "GitHub\n"
                "github.com/santosmanilva/SANTOS-LEVELER\n\n"
                "Contact\n"
                "santos.manilva@gmail.com\n\n"
                "Built with JUCE\n"
                "True Peak - LUFS M/S/I - Voice Auto Level Rider\n\n"
                "© 2026 José Antonio Santos Santos\n"
                "All rights reserved.";

            juce::AlertWindow::showMessageBoxAsync (juce::MessageBoxIconType::InfoIcon,
                                                    "Santos Leveler",
                                                    message,
                                                    "CLOSE",
                                                    observedParent);
        }

        void parentHierarchyChanged() override
        {
            if (observedParent != nullptr)
                observedParent->removeComponentListener (this);
            observedParent = getParentComponent();
            if (observedParent != nullptr)
            {
                observedParent->addComponentListener (this);
                updateBounds();
            }
        }

    private:
        void componentMovedOrResized (juce::Component&, bool, bool wasResized) override
        {
            if (wasResized)
                updateBounds();
        }

        void updateBounds()
        {
            if (observedParent == nullptr)
                return;
            const auto sx = static_cast<float> (observedParent->getWidth()) / 2100.0f;
            const auto sy = static_cast<float> (observedParent->getHeight()) / 1024.0f;
            setBounds (juce::roundToInt (30.0f * sx), juce::roundToInt (14.0f * sy),
                       juce::roundToInt (365.0f * sx), juce::roundToInt (70.0f * sy));
        }

        juce::Component* observedParent = nullptr;
    };

    class FactoryPresetButton final : public juce::TextButton,
                                      private juce::ComponentListener
    {
    public:
        explicit FactoryPresetButton (SantosLevelerAudioProcessor& p) : processor (p)
        {
            setButtonText ("PRESET");
            setColour (juce::TextButton::buttonColourId, juce::Colour (0xff101a20));
            setColour (juce::TextButton::buttonOnColourId, juce::Colour (0xff18323b));
            setColour (juce::TextButton::textColourOffId, juce::Colour (0xff19ccf4));
            setColour (juce::TextButton::textColourOnId, juce::Colour (0xffeef4f6));
            onClick = [this] { showMenu(); };
        }

        ~FactoryPresetButton() override
        {
            if (observedParent != nullptr)
                observedParent->removeComponentListener (this);
        }

        void parentHierarchyChanged() override
        {
            if (observedParent != nullptr)
                observedParent->removeComponentListener (this);
            observedParent = getParentComponent();
            if (observedParent != nullptr)
            {
                observedParent->addComponentListener (this);
                updateBounds();
            }
        }

    private:
        struct Preset
        {
            const char* name;
            std::array<float, 21> values;
        };

        static constexpr std::array<const char*, 21> parameterIds {{
            "target", "gate", "speed", "detect", "lookahead", "hold", "release",
            "peakThreshold", "rangeDown", "rangeUp", "output", "downStrength",
            "upStrength", "intensity", "compEnabled", "compThreshold", "compRatio",
            "compAttack", "compRelease", "compMakeup", "ceiling"
        }};

        static const std::array<Preset, 5>& presets()
        {
            static const std::array<Preset, 5> data {{
                { "Default",   {{ -19.0f, -45.0f, 15.0f,  8.0f, 30.0f,  50.0f, 500.0f, -9.0f, -12.0f,  9.0f, 0.0f, 100.0f, 100.0f, 100.0f, 0.0f, -18.0f, 3.0f, 10.0f, 120.0f, 0.0f, -1.0f }} },
                { "Gentle",    {{ -19.0f, -45.0f, 30.0f, 15.0f, 30.0f, 100.0f, 800.0f, -8.0f,  -8.0f,  6.0f, 0.0f,  65.0f,  65.0f,  70.0f, 0.0f, -18.0f, 2.5f, 15.0f, 160.0f, 0.0f, -1.0f }} },
                { "Natural",   {{ -19.0f, -45.0f, 22.0f, 10.0f, 30.0f,  70.0f, 650.0f, -8.5f, -10.0f,  7.0f, 0.0f,  80.0f,  80.0f,  82.0f, 0.0f, -18.0f, 2.5f, 12.0f, 150.0f, 0.0f, -1.0f }} },
                { "Broadcast", {{ -19.0f, -45.0f, 15.0f,  8.0f, 30.0f,  50.0f, 500.0f, -9.0f, -12.0f,  9.0f, 0.0f, 100.0f, 100.0f, 100.0f, 1.0f, -18.0f, 3.0f, 10.0f, 120.0f, 0.0f, -1.0f }} },
                { "Tight",     {{ -18.0f, -45.0f, 10.0f,  5.0f, 40.0f,  40.0f, 350.0f, -9.0f, -14.0f, 12.0f, 0.0f, 100.0f, 100.0f, 100.0f, 1.0f, -20.0f, 4.0f,  6.0f, 100.0f, 0.0f, -1.0f }} }
            }};
            return data;
        }

        static juce::File presetFolder()
        {
            auto folder = juce::File::getSpecialLocation (juce::File::userDocumentsDirectory)
                              .getChildFile ("Santos Leveler Presets");
            folder.createDirectory();
            return folder;
        }

        void showMenu()
        {
            juce::PopupMenu menu;
            const auto& p = presets();
            for (int i = 0; i < static_cast<int> (p.size()); ++i)
                menu.addItem (i + 1, p[static_cast<std::size_t> (i)].name);

            menu.addSeparator();
            menu.addItem (100, "Save Preset...");
            menu.addItem (101, "Load Preset...");

            auto safeThis = juce::Component::SafePointer<FactoryPresetButton> (this);
            menu.showMenuAsync (juce::PopupMenu::Options().withTargetComponent (this),
                                [safeThis] (int result)
                                {
                                    if (safeThis == nullptr)
                                        return;
                                    if (result >= 1 && result <= 5)
                                        safeThis->applyPreset (result - 1);
                                    else if (result == 100)
                                        safeThis->savePreset();
                                    else if (result == 101)
                                        safeThis->loadPreset();
                                });
        }

        void applyPreset (int index)
        {
            const auto& p = presets();
            if (index < 0 || index >= static_cast<int> (p.size()))
                return;

            const auto& values = p[static_cast<std::size_t> (index)].values;
            for (std::size_t i = 0; i < parameterIds.size(); ++i)
                setParameterValue (parameterIds[i], values[i]);
        }

        void setParameterValue (const juce::String& id, float value)
        {
            if (auto* parameter = processor.apvts.getParameter (id))
            {
                parameter->beginChangeGesture();
                parameter->setValueNotifyingHost (juce::jlimit (0.0f, 1.0f, parameter->convertTo0to1 (value)));
                parameter->endChangeGesture();
            }
        }

        void savePreset()
        {
            auto initial = presetFolder().getChildFile ("My Preset.slpreset");
            fileChooser = std::make_unique<juce::FileChooser> ("Save Santos Leveler preset",
                                                               initial,
                                                               "*.slpreset",
                                                               true,
                                                               false,
                                                               observedParent);

            auto safeThis = juce::Component::SafePointer<FactoryPresetButton> (this);
            const auto chooserFlags = juce::FileBrowserComponent::saveMode
                                    | juce::FileBrowserComponent::canSelectFiles
                                    | juce::FileBrowserComponent::warnAboutOverwriting;
            fileChooser->launchAsync (chooserFlags, [safeThis] (const juce::FileChooser& chooser)
            {
                if (safeThis == nullptr)
                    return;

                auto file = chooser.getResult();
                if (file == juce::File())
                    return;
                if (! file.hasFileExtension ("slpreset"))
                    file = file.withFileExtension ("slpreset");

                juce::XmlElement root ("SANTOS_LEVELER_PRESET");
                root.setAttribute ("version", 1);
                for (const auto* id : parameterIds)
                {
                    if (auto* value = safeThis->processor.apvts.getRawParameterValue (id))
                    {
                        auto* parameter = root.createNewChildElement ("PARAM");
                        parameter->setAttribute ("id", id);
                        parameter->setAttribute ("value", static_cast<double> (value->load()));
                    }
                }

                if (! root.writeTo (file))
                    juce::AlertWindow::showMessageBoxAsync (juce::MessageBoxIconType::WarningIcon,
                                                            "Santos Leveler",
                                                            "The preset could not be saved.");
            });
        }

        void loadPreset()
        {
            fileChooser = std::make_unique<juce::FileChooser> ("Load Santos Leveler preset",
                                                               presetFolder(),
                                                               "*.slpreset",
                                                               true,
                                                               false,
                                                               observedParent);

            auto safeThis = juce::Component::SafePointer<FactoryPresetButton> (this);
            const auto chooserFlags = juce::FileBrowserComponent::openMode
                                    | juce::FileBrowserComponent::canSelectFiles;
            fileChooser->launchAsync (chooserFlags, [safeThis] (const juce::FileChooser& chooser)
            {
                if (safeThis == nullptr)
                    return;

                const auto file = chooser.getResult();
                if (file == juce::File())
                    return;

                auto xml = juce::XmlDocument::parse (file);
                if (xml == nullptr || ! xml->hasTagName ("SANTOS_LEVELER_PRESET"))
                {
                    juce::AlertWindow::showMessageBoxAsync (juce::MessageBoxIconType::WarningIcon,
                                                            "Santos Leveler",
                                                            "This is not a valid Santos Leveler preset.");
                    return;
                }

                for (auto* child = xml->getFirstChildElement(); child != nullptr; child = child->getNextElement())
                {
                    if (! child->hasTagName ("PARAM"))
                        continue;

                    const auto id = child->getStringAttribute ("id");
                    const auto value = static_cast<float> (child->getDoubleAttribute ("value"));
                    for (const auto* allowedId : parameterIds)
                    {
                        if (id == allowedId)
                        {
                            safeThis->setParameterValue (id, value);
                            break;
                        }
                    }
                }
            });
        }

        void componentMovedOrResized (juce::Component&, bool, bool wasResized) override
        {
            if (wasResized)
                updateBounds();
        }

        void updateBounds()
        {
            if (observedParent == nullptr)
                return;
            const auto sx = static_cast<float> (observedParent->getWidth()) / 2100.0f;
            const auto sy = static_cast<float> (observedParent->getHeight()) / 1024.0f;
            setBounds (juce::roundToInt (400.0f * sx), juce::roundToInt (29.0f * sy),
                       juce::roundToInt (82.0f * sx), juce::roundToInt (40.0f * sy));
        }

        SantosLevelerAudioProcessor& processor;
        juce::Component* observedParent = nullptr;
        std::unique_ptr<juce::FileChooser> fileChooser;
    };

    void timerCallback() override;
    void configureKnob (juce::Slider&, juce::Label&, const juce::String& name,
                        const juce::String& suffix, int decimals, juce::Colour accent);
    void configureFader (juce::Slider&, juce::Label&, const juce::String& name,
                         const juce::String& suffix, int decimals, juce::Colour accent);
    void configureHeaderButton (juce::TextButton& button, const juce::String& text);
    void updateABButtons();
    void ensurePresetButtonVisible()
    {
        if (aboutButton.getParentComponent() != this)
            addAndMakeVisible (aboutButton);
        if (presetButton.getParentComponent() != this)
            addAndMakeVisible (presetButton);
        aboutButton.toFront (false);
    }

    SantosLevelerAudioProcessor& processor;
    SantosLookAndFeel lookAndFeel;

    juce::Slider targetKnob, gateKnob, speedKnob, detectKnob, lookaheadKnob, holdKnob, releaseKnob, peakThresholdKnob;
    juce::Slider rangeDownSlider, downStrengthSlider, rangeUpSlider, upStrengthSlider, outputSlider;
    juce::Slider intensitySlider;
    juce::Slider compThresholdKnob, compRatioKnob, compAttackKnob, compReleaseKnob, compMakeupKnob, ceilingKnob;

    juce::Label targetLabel, gateLabel, speedLabel, detectLabel, lookaheadLabel, holdLabel, releaseLabel, peakThresholdLabel;
    juce::Label rangeDownLabel, downStrengthLabel, rangeUpLabel, upStrengthLabel, outputLabel;
    juce::Label intensityLabel;
    juce::Label compThresholdLabel, compRatioLabel, compAttackLabel, compReleaseLabel, compMakeupLabel, ceilingLabel;
    juce::Label titleLabel, subtitleLabel;

    AboutButton aboutButton;
    FactoryPresetButton presetButton { processor };
    juce::TextButton resetLoudnessButton;
    juce::TextButton aButton;
    juce::TextButton bButton;
    juce::TextButton bypassButton;
    juce::TextButton compButton;

    HistoryComponent history;
    MeterComponent inputMeter;
    MeterComponent outputMeter;
    FinalMeterComponent finalMeter;
    LoudnessMeterComponent loudnessMeter;

    juce::AudioProcessorValueTreeState::SliderAttachment targetAttachment;
    juce::AudioProcessorValueTreeState::SliderAttachment gateAttachment;
    juce::AudioProcessorValueTreeState::SliderAttachment speedAttachment;
    juce::AudioProcessorValueTreeState::SliderAttachment detectAttachment;
    juce::AudioProcessorValueTreeState::SliderAttachment lookaheadAttachment;
    juce::AudioProcessorValueTreeState::SliderAttachment holdAttachment;
    juce::AudioProcessorValueTreeState::SliderAttachment releaseAttachment;
    juce::AudioProcessorValueTreeState::SliderAttachment peakThresholdAttachment;
    juce::AudioProcessorValueTreeState::SliderAttachment rangeDownAttachment;
    juce::AudioProcessorValueTreeState::SliderAttachment downStrengthAttachment;
    juce::AudioProcessorValueTreeState::SliderAttachment rangeUpAttachment;
    juce::AudioProcessorValueTreeState::SliderAttachment upStrengthAttachment;
    juce::AudioProcessorValueTreeState::SliderAttachment outputAttachment;
    juce::AudioProcessorValueTreeState::SliderAttachment intensityAttachment;
    juce::AudioProcessorValueTreeState::SliderAttachment compThresholdAttachment;
    juce::AudioProcessorValueTreeState::SliderAttachment compRatioAttachment;
    juce::AudioProcessorValueTreeState::SliderAttachment compAttackAttachment;
    juce::AudioProcessorValueTreeState::SliderAttachment compReleaseAttachment;
    juce::AudioProcessorValueTreeState::SliderAttachment compMakeupAttachment;
    juce::AudioProcessorValueTreeState::SliderAttachment ceilingAttachment;
    juce::AudioProcessorValueTreeState::ButtonAttachment compEnabledAttachment;
    juce::AudioProcessorValueTreeState::ButtonAttachment bypassAttachment;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SantosLevelerAudioProcessorEditor)
};
