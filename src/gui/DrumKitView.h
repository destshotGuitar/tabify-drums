#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

#include "KitDefinition.h"

// The drum kit picture: a top-down photo of the kit with hit zones and hit
// animations drawn over it. All geometry is in photo pixels; the view scales
// it to whatever size it is given. Pieces that are not in the photo (cowbell,
// clap) get pads on the floor.
class DrumKitView final : public juce::Component, private juce::Timer
{
public:
    static constexpr int imageWidth = 1920;
    static constexpr int imageHeight = 1088;

    DrumKitView();

    std::function<void (int articulation)> onPlay;
    std::function<void (int piece)> onPieceMenu;
    std::function<void (const juce::String&)> onHoverHelp;

    void notifyHit (int articulation, int velocity);
    void setLearningPiece (int piece); // -1 = none

    void paint (juce::Graphics&) override;
    void mouseDown (const juce::MouseEvent&) override;
    void mouseMove (const juce::MouseEvent&) override;
    void mouseExit (const juce::MouseEvent&) override;

private:
    struct Zone
    {
        int articulation;
        juce::Path area;      // where clicks land
        juce::Path highlight; // what lights up on hover and MIDI learn
        juce::String help;
    };

    struct Pad
    {
        int articulation;
        juce::Rectangle<float> bounds;
        juce::String label;
        float energy = 0.0f;
    };

    struct Anim
    {
        float energy = 0.0f;  // glow brightness, decays after a hit
        float ripple = 1.0f;  // 0 = just hit .. 1 = finished
        float strength = 0.0f;
        float wobble = 0.0f;  // cymbal swing amplitude
        float phase = 0.0f;
    };

    void timerCallback() override;
    void buildZones();
    juce::Point<float> toImage (juce::Point<float>) const;
    const Zone* zoneAt (juce::Point<float> imagePos) const;
    int articulationFor (const Zone&, const juce::ModifierKeys&) const;

    void drawDrumHit (juce::Graphics&, int piece);
    void drawKickHit (juce::Graphics&);
    void drawCymbal (juce::Graphics&, int piece);
    void drawPads (juce::Graphics&);
    void drawHighlights (juce::Graphics&);

    juce::Image photo;
    std::vector<Zone> zones;
    std::vector<Pad> pads;
    std::array<Anim, kit::numPieces> anims;
    float hatOpen = 0.0f, hatOpenTarget = 0.0f, hatBounce = 0.0f;
    const Zone* hovered = nullptr;
    int learningPiece = -1;
    float learnPulse = 0.0f;
};
