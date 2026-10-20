#pragma once

#include "PluginProcessor.h"
#include <UiData.h>
#include "gui/DrumKitView.h"
#include "gui/DrumLookAndFeel.h"
#include "gui/MixerView.h"

class TabifyDrumsEditor final : public juce::AudioProcessorEditor, private juce::Timer
{
public:
    explicit TabifyDrumsEditor (TabifyDrumsProcessor&);
    ~TabifyDrumsEditor() override;

    void resized() override;

private:
    static constexpr int designWidth = 1000;
    static constexpr int headerHeight = 50;
    static constexpr int infoHeight = 60;
    static constexpr int kitHeight = designWidth * DrumKitView::imageHeight / DrumKitView::imageWidth;
    static constexpr int designHeight = headerHeight + kitHeight + infoHeight;

    // Everything is laid out at the design size inside this component, which
    // is then scaled as a whole when the editor window is resized.
    class Surface final : public juce::Component
    {
    public:
        void paint (juce::Graphics&) override;
        bool learning = false;
        juce::Image logo = juce::ImageCache::getFromMemory (UiData::logo_png, UiData::logo_pngSize);
    };

    void timerCallback() override;
    void showPage (bool kit);
    void showPieceMenu (int piece);
    void updateLearnState();
    static juce::String noteName (int note);
    juce::String describeNotes (int articulation) const;

    TabifyDrumsProcessor& processor;
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

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (TabifyDrumsEditor)
};
