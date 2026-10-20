#pragma once

#include "PluginProcessor.h"
#include "gui/DrumKitView.h"
#include "gui/DrumLookAndFeel.h"
#include "gui/MixerView.h"

class OpenDrumsEditor final : public juce::AudioProcessorEditor, private juce::Timer
{
public:
    explicit OpenDrumsEditor (OpenDrumsProcessor&);
    ~OpenDrumsEditor() override;

    void resized() override;

private:
    static constexpr int designWidth = 1000;
    static constexpr int designHeight = 640;
    static constexpr int headerHeight = 50;
    static constexpr int infoHeight = 60;

    // Everything is laid out at the design size inside this component, which
    // is then scaled as a whole when the editor window is resized.
    class Surface final : public juce::Component
    {
    public:
        void paint (juce::Graphics&) override;
        bool learning = false;
    };

    void timerCallback() override;
    void showPage (bool kit);
    void showPieceMenu (int piece);
    void updateLearnState();
    static juce::String noteName (int note);
    juce::String describeNotes (int articulation) const;

    OpenDrumsProcessor& processor;
    DrumLookAndFeel lookAndFeel;
    juce::TooltipWindow tooltips { this, 600 };

    Surface surface;
    juce::TextButton kitTab { "KIT" }, mixerTab { "MIXER" };
    juce::Slider masterKnob;
    juce::Label masterLabel;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> masterAttachment;

    DrumKitView kitView;
    MixerView mixerView;

    juce::Label lastHitLabel, helpLabel, velocityLabel;
    juce::Slider velocitySlider;
    juce::TextButton cancelLearnButton { "Cancel" };

    juce::String statusText;
    int lastLearnArticulation = -2;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (OpenDrumsEditor)
};
