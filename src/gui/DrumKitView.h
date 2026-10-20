#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

#include "KitDefinition.h"

// The drum kit picture. Everything is drawn with vector graphics in a fixed
// 1000 x 530 coordinate space; the editor scales the whole UI as one piece.
// Clicking a zone plays its articulation, and every hit (from MIDI or a click)
// makes the matching drum or cymbal react.
class DrumKitView final : public juce::Component, private juce::Timer
{
public:
    static constexpr int designWidth = 1000;
    static constexpr int designHeight = 530;

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
        juce::Path area;
        juce::String help;
    };

    struct Anim
    {
        float energy = 0.0f;  // flash brightness, decays after a hit
        float ripple = 1.0f;  // 0 = just hit .. 1 = finished
        float strength = 0.0f;
        float wobble = 0.0f;  // cymbal swing amplitude
        float phase = 0.0f;
    };

    void timerCallback() override;
    void buildZones();
    const Zone* zoneAt (juce::Point<float>) const;
    int articulationFor (const Zone&, const juce::ModifierKeys&) const;

    void drawBackground (juce::Graphics&);
    void drawStands (juce::Graphics&);
    void drawCymbalPiece (juce::Graphics&, int piece);
    void drawTom (juce::Graphics&, int piece);
    void drawKick (juce::Graphics&);
    void drawSnare (juce::Graphics&);
    void drawFloorTom (juce::Graphics&);
    void drawHiHat (juce::Graphics&);
    void drawCowbell (juce::Graphics&);
    void drawHighlights (juce::Graphics&);

    std::vector<Zone> zones;
    std::array<Anim, kit::numPieces> anims;
    float hatOpen = 0.0f, hatOpenTarget = 0.0f, hatBounce = 0.0f;
    const Zone* hovered = nullptr;
    int learningPiece = -1;
    float learnPulse = 0.0f;
};
