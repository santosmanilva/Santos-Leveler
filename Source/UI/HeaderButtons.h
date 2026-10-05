#pragma once

#include "Theme.h"
#include "../PluginProcessor.h"

namespace SantosUI
{
// "Presets" drop-down: factory presets plus save / load of .slpreset files.
class PresetButton final : public juce::Button
{
public:
    explicit PresetButton (SantosLevelerAudioProcessor& p) : juce::Button ("Presets"), processor (p)
    {
        setMouseCursor (juce::MouseCursor::PointingHandCursor);
        onClick = [this] { showMenu(); };
    }

    void paintButton (juce::Graphics& g, bool highlighted, bool down) override
    {
        auto bounds = getLocalBounds().toFloat().reduced (0.5f);
        g.setColour (down ? colour::track : (highlighted ? colour::cardHigh.brighter (0.12f) : colour::cardHigh));
        g.fillRoundedRectangle (bounds, 6.0f);
        g.setColour (colour::edge);
        g.drawRoundedRectangle (bounds, 6.0f, 1.0f);

        g.setColour (highlighted ? colour::text : colour::textDim);
        g.setFont (font (11.5f, true).withExtraKerningFactor (0.08f));
        g.drawText ("PRESETS", bounds.withTrimmedRight (22.0f).withTrimmedLeft (4.0f), juce::Justification::centred, false);

        juce::Path chevron;
        const auto cx = bounds.getRight() - 15.0f, cy = bounds.getCentreY();
        chevron.startNewSubPath (cx - 4.0f, cy - 2.0f);
        chevron.lineTo (cx, cy + 2.0f);
        chevron.lineTo (cx + 4.0f, cy - 2.0f);
        g.strokePath (chevron, juce::PathStrokeType (1.6f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
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
            { "Default",   {{ -19.0f, -40.0f, 79.0f,  8.0f, 30.0f, 100.0f, 100.0f, -8.0f, -12.0f, 15.0f, 0.0f,  69.0f,  50.0f, 100.0f, 1.0f, -20.0f, 3.0f, 10.0f, 120.0f, 2.0f, -1.0f }} },
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
        const auto& list = presets();
        for (int i = 0; i < static_cast<int> (list.size()); ++i)
            menu.addItem (i + 1, list[static_cast<std::size_t> (i)].name);

        menu.addSeparator();
        menu.addItem (100, "Save Preset...");
        menu.addItem (101, "Load Preset...");

        auto safeThis = juce::Component::SafePointer<PresetButton> (this);
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
        const auto& list = presets();
        if (index < 0 || index >= static_cast<int> (list.size()))
            return;

        const auto& values = list[static_cast<std::size_t> (index)].values;
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
        fileChooser = std::make_unique<juce::FileChooser> ("Save Santos Leveler preset", initial, "*.slpreset",
                                                           true, false, getTopLevelComponent());

        auto safeThis = juce::Component::SafePointer<PresetButton> (this);
        const auto flags = juce::FileBrowserComponent::saveMode
                         | juce::FileBrowserComponent::canSelectFiles
                         | juce::FileBrowserComponent::warnAboutOverwriting;
        fileChooser->launchAsync (flags, [safeThis] (const juce::FileChooser& chooser)
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
                                                        "Santos Leveler", "The preset could not be saved.");
        });
    }

    void loadPreset()
    {
        fileChooser = std::make_unique<juce::FileChooser> ("Load Santos Leveler preset", presetFolder(), "*.slpreset",
                                                           true, false, getTopLevelComponent());

        auto safeThis = juce::Component::SafePointer<PresetButton> (this);
        const auto flags = juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectFiles;
        fileChooser->launchAsync (flags, [safeThis] (const juce::FileChooser& chooser)
        {
            if (safeThis == nullptr)
                return;

            const auto file = chooser.getResult();
            if (file == juce::File())
                return;

            constexpr juce::int64 maxPresetSizeBytes = 256 * 1024;
            if (! file.existsAsFile() || file.getSize() <= 0 || file.getSize() > maxPresetSizeBytes)
            {
                juce::AlertWindow::showMessageBoxAsync (juce::MessageBoxIconType::WarningIcon,
                                                        "Santos Leveler", "The preset file is empty or too large.");
                return;
            }

            auto xml = juce::XmlDocument::parse (file);
            if (xml == nullptr || ! xml->hasTagName ("SANTOS_LEVELER_PRESET") || xml->getIntAttribute ("version", -1) != 1)
            {
                juce::AlertWindow::showMessageBoxAsync (juce::MessageBoxIconType::WarningIcon,
                                                        "Santos Leveler", "This is not a valid Santos Leveler preset.");
                return;
            }

            for (auto* child = xml->getFirstChildElement(); child != nullptr; child = child->getNextElement())
            {
                if (! child->hasTagName ("PARAM"))
                    continue;

                const auto id = child->getStringAttribute ("id");
                const auto value = static_cast<float> (child->getDoubleAttribute ("value"));
                if (! std::isfinite (value))
                    continue;

                for (const auto* allowedId : parameterIds)
                    if (id == allowedId)
                    {
                        safeThis->setParameterValue (id, value);
                        break;
                    }
            }
        });
    }

    SantosLevelerAudioProcessor& processor;
    std::unique_ptr<juce::FileChooser> fileChooser;
};

// Small round "i" button that opens the about box.
class AboutButton final : public juce::Button
{
public:
    AboutButton() : juce::Button ("About Santos Leveler")
    {
        setMouseCursor (juce::MouseCursor::PointingHandCursor);
        setTooltip ("About Santos Leveler");
        onClick = [this] { showAbout(); };
    }

    void paintButton (juce::Graphics& g, bool highlighted, bool) override
    {
        auto bounds = getLocalBounds().toFloat().reduced (0.5f);
        g.setColour (highlighted ? colour::cardHigh.brighter (0.12f) : colour::cardHigh);
        g.fillEllipse (bounds);
        g.setColour (colour::edge);
        g.drawEllipse (bounds, 1.0f);
        g.setColour (highlighted ? colour::text : colour::textDim);
        g.setFont (font (13.0f, true));
        g.drawText ("i", bounds, juce::Justification::centred, false);
    }

private:
    void showAbout()
    {
        const auto dot = juce::String::charToString (0x00b7);
        const auto eAcute = juce::String::charToString (0x00e9);

        juce::String message;
        message << "Version " << JucePlugin_VersionString << "\n\n"
                << "Voice Auto Level Rider\n"
                << "VST3 Audio Plugin " << dot << " Windows x64\n\n"
                << "Designed & Developed by\n"
                << "Jos" << eAcute << " Antonio Santos Santos\n\n"
                << "OPEN SOURCE\n"
                << "GNU AGPL v3.0\n\n"
                << "GitHub\n"
                << "github.com/santosmanilva/Santos-Leveler\n\n"
                << "Support\n"
                << "github.com/santosmanilva/Santos-Leveler/issues\n\n"
                << "Built with JUCE\n"
                << "True Peak " << dot << " LUFS M/S/I " << dot << " Voice Auto Level Rider\n\n"
                << juce::String::charToString (0x00a9) << " 2026 Jos" << eAcute << " Antonio Santos Santos\n"
                << "Licensed under GNU AGPL v3.0.";

        juce::AlertWindow::showMessageBoxAsync (juce::MessageBoxIconType::InfoIcon, "Santos Leveler",
                                                message, "CLOSE", getTopLevelComponent());
    }
};
} // namespace SantosUI
