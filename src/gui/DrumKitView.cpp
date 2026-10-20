#include "DrumKitView.h"
#include "DrumLookAndFeel.h"

#include <UiData.h>

namespace
{
// Positions in photo pixels (resources/ui/kit.jpg, 1920 x 1088).
struct Oval
{
    float cx, cy, rx, ry;

    juce::Rectangle<float> bounds() const { return { cx - rx, cy - ry, rx * 2.0f, ry * 2.0f }; }
    juce::Point<float> centre() const { return { cx, cy }; }

    juce::Path path (float scale = 1.0f) const
    {
        juce::Path p;
        p.addEllipse (bounds().withSizeKeepingCentre (rx * 2.0f * scale, ry * 2.0f * scale));
        return p;
    }
};

constexpr Oval tom1Head { 1127.0f, 357.0f, 122.0f, 110.0f };
constexpr Oval tom2Head { 790.0f, 340.0f, 123.0f, 112.0f };
constexpr Oval floorTomHead { 582.0f, 562.0f, 124.0f, 107.0f };
constexpr Oval snareHead { 1327.0f, 560.0f, 112.0f, 113.0f };
constexpr Oval kickHead { 956.0f, 787.0f, 172.0f, 62.0f };

constexpr Oval rideOval { 460.0f, 425.0f, 183.0f, 153.0f };
constexpr Oval rideBellOval { 452.0f, 400.0f, 40.0f, 34.0f };
constexpr Oval crashOval { 1390.0f, 250.0f, 188.0f, 170.0f };
constexpr Oval hatOval { 1486.0f, 527.0f, 134.0f, 121.0f };

const juce::Rectangle<float> kickShell { 800.0f, 430.0f, 330.0f, 330.0f };
const juce::Rectangle<float> kickGlowArea { 800.0f, 470.0f, 330.0f, 290.0f };

const Oval& headFor (int piece)
{
    switch (piece)
    {
        case kit::tom1:     return tom1Head;
        case kit::tom2:     return tom2Head;
        case kit::floorTom: return floorTomHead;
        case kit::snare:    return snareHead;
        case kit::crash:    return crashOval;
        case kit::ride:     return rideOval;
        case kit::hihat:    return hatOval;
        default:            return kickHead;
    }
}

bool isCymbal (int piece)
{
    return piece == kit::crash || piece == kit::ride || piece == kit::splash || piece == kit::hihat;
}

juce::Path rectPath (juce::Rectangle<float> r)
{
    juce::Path p;
    p.addRectangle (r);
    return p;
}
} // namespace

DrumKitView::DrumKitView()
{
    photo = juce::ImageCache::getFromMemory (UiData::kit_jpg, UiData::kit_jpgSize);
    buildZones();
    startTimerHz (60);
}

void DrumKitView::buildZones()
{
    const juce::String more = juce::CharPointer_UTF8 ("  \xc2\xb7  right-click: MIDI learn & more");

    const auto hatHelp = "Hi-Hat: center = closed, edge = open, Shift-click = semi-open" + more;
    zones.push_back ({ kit::hatClosed, hatOval.path (0.6f), hatOval.path (0.6f), hatHelp });
    zones.push_back ({ kit::hatOpen, hatOval.path(), hatOval.path(), hatHelp });

    zones.push_back ({ kit::crashHit, crashOval.path(), crashOval.path(), "Crash" + more });

    const auto rideHelp = "Ride: bow = ride, center = bell" + more;
    zones.push_back ({ kit::rideBell, rideBellOval.path(), rideBellOval.path(), rideHelp });
    zones.push_back ({ kit::rideHit, rideOval.path(), rideOval.path(), rideHelp });

    // Pieces that are not in the photo get pads on the floor.
    const std::array<std::pair<int, const char*>, 3> padList {{
        { kit::splashHit, "SPLASH" }, { kit::cowbellHit, "COWBELL" }, { kit::clapHit, "CLAP" } }};
    for (size_t i = 0; i < padList.size(); ++i)
    {
        Pad pad;
        pad.articulation = padList[i].first;
        pad.label = padList[i].second;
        pad.bounds = { 70.0f, 730.0f + 92.0f * (float) i, 200.0f, 76.0f };
        pads.push_back (pad);
        juce::Path padShape;
        padShape.addRoundedRectangle (pad.bounds, 12.0f);
        zones.push_back ({ pad.articulation, padShape, padShape,
                           juce::String (kit::articulations[(size_t) pad.articulation].name) + more });
    }

    const auto snareHelp = "Snare: center = snare, rim = rimshot, shell = side stick" + more;
    zones.push_back ({ kit::snareHit, snareHead.path (0.72f), snareHead.path (0.72f), snareHelp });
    zones.push_back ({ kit::snareRimshot, snareHead.path (1.05f), snareHead.path(), snareHelp });
    zones.push_back ({ kit::snareSideStick, rectPath ({ 1215.0f, 560.0f, 226.0f, 175.0f }), snareHead.path (1.08f), snareHelp });

    auto drumZone = [] (const Oval& head, float shellBottom)
    {
        auto p = head.path (1.04f);
        p.addRectangle (head.cx - head.rx, head.cy, head.rx * 2.0f, shellBottom - head.cy);
        return p;
    };

    zones.push_back ({ kit::tom1Hit, drumZone (tom1Head, 540.0f), tom1Head.path(), "Tom 1" + more });
    zones.push_back ({ kit::tom2Hit, drumZone (tom2Head, 525.0f), tom2Head.path(), "Tom 2" + more });
    zones.push_back ({ kit::floorTomHit, drumZone (floorTomHead, 735.0f), floorTomHead.path(), "Floor Tom" + more });

    auto kickArea = kickHead.path (1.05f);
    kickArea.addRectangle (kickShell);
    zones.push_back ({ kit::kickHit, kickArea, kickHead.path(), "Kick" + more });
}

juce::Point<float> DrumKitView::toImage (juce::Point<float> p) const
{
    return p * ((float) imageWidth / (float) juce::jmax (1, getWidth()));
}

const DrumKitView::Zone* DrumKitView::zoneAt (juce::Point<float> p) const
{
    for (const auto& z : zones)
        if (z.area.contains (p))
            return &z;
    return nullptr;
}

int DrumKitView::articulationFor (const Zone& zone, const juce::ModifierKeys& mods) const
{
    if (mods.isShiftDown() && (zone.articulation == kit::hatClosed || zone.articulation == kit::hatOpen))
        return kit::hatSemiOpen;
    return zone.articulation;
}

void DrumKitView::mouseDown (const juce::MouseEvent& e)
{
    const auto* zone = zoneAt (toImage (e.position));
    if (zone == nullptr)
        return;

    if (e.mods.isPopupMenu())
    {
        if (onPieceMenu)
            onPieceMenu (kit::articulations[(size_t) zone->articulation].piece);
        return;
    }

    if (onPlay)
        onPlay (articulationFor (*zone, e.mods));
}

void DrumKitView::mouseMove (const juce::MouseEvent& e)
{
    const auto* zone = zoneAt (toImage (e.position));
    if (zone == hovered)
        return;

    hovered = zone;
    setMouseCursor (zone != nullptr ? juce::MouseCursor::PointingHandCursor : juce::MouseCursor::NormalCursor);
    if (onHoverHelp)
        onHoverHelp (zone != nullptr ? zone->help : juce::String());
    repaint();
}

void DrumKitView::mouseExit (const juce::MouseEvent&)
{
    hovered = nullptr;
    if (onHoverHelp)
        onHoverHelp ({});
    repaint();
}

void DrumKitView::notifyHit (int articulation, int velocity)
{
    if (! juce::isPositiveAndBelow (articulation, (int) kit::numArticulations))
        return;

    const auto piece = kit::articulations[(size_t) articulation].piece;
    const auto v = juce::jlimit (0.0f, 1.0f, (float) velocity / 127.0f);
    auto& a = anims[(size_t) piece];
    a.energy = juce::jmax (a.energy, 0.35f + 0.65f * v);
    a.strength = v;
    a.ripple = 0.0f;
    a.wobble = juce::jmax (a.wobble * 0.5f, v);
    a.phase = 0.0f;

    for (auto& pad : pads)
        if (pad.articulation == articulation)
            pad.energy = 0.4f + 0.6f * v;

    switch (articulation)
    {
        case kit::hatClosed:   hatOpenTarget = 0.0f; a.wobble *= 0.3f; break;
        case kit::hatPedal:    hatOpenTarget = 0.0f; hatBounce = 1.0f; a.wobble = 0.0f; break;
        case kit::hatSemiOpen: hatOpenTarget = 0.45f; break;
        case kit::hatOpen:     hatOpenTarget = 1.0f; break;
        default: break;
    }

    repaint();
}

void DrumKitView::setLearningPiece (int piece)
{
    if (piece != learningPiece)
    {
        learningPiece = piece;
        repaint();
    }
}

void DrumKitView::timerCallback()
{
    constexpr float dt = 1.0f / 60.0f;
    bool active = learningPiece >= 0;

    for (size_t p = 0; p < anims.size(); ++p)
    {
        auto& a = anims[p];
        a.energy *= isCymbal ((int) p) ? 0.93f : 0.88f;
        a.ripple = juce::jmin (1.0f, a.ripple + dt * 2.5f);
        a.wobble *= 0.965f;
        a.phase += dt * juce::MathConstants<float>::twoPi * 2.5f;

        if (a.energy < 0.002f) a.energy = 0.0f;
        if (a.wobble < 0.002f) a.wobble = 0.0f;
        active = active || a.energy > 0.0f || a.ripple < 1.0f || a.wobble > 0.0f;
    }

    for (auto& pad : pads)
    {
        pad.energy = pad.energy < 0.002f ? 0.0f : pad.energy * 0.88f;
        active = active || pad.energy > 0.0f;
    }

    hatOpen += (hatOpenTarget - hatOpen) * 0.35f;
    hatBounce *= 0.8f;
    active = active || std::abs (hatOpenTarget - hatOpen) > 0.001f || hatBounce > 0.01f;

    if (learningPiece >= 0)
        learnPulse += dt * 5.0f;

    if (active)
        repaint();
}

void DrumKitView::paint (juce::Graphics& g)
{
    g.addTransform (juce::AffineTransform::scale ((float) getWidth() / (float) imageWidth));
    g.setImageResamplingQuality (juce::Graphics::highResamplingQuality);
    g.drawImageAt (photo, 0, 0);

    drawKickHit (g);
    for (const auto piece : { kit::snare, kit::tom1, kit::tom2, kit::floorTom })
        drawDrumHit (g, piece);

    // Cymbals are redrawn from the photo on top of the drums, so drum glows
    // stay underneath them and the cymbals can swing.
    for (const auto piece : { kit::ride, kit::crash, kit::hihat })
        drawCymbal (g, piece);

    drawPads (g);
    drawHighlights (g);
}

void DrumKitView::drawDrumHit (juce::Graphics& g, int piece)
{
    const auto& a = anims[(size_t) piece];
    if (a.energy <= 0.0f && a.ripple >= 1.0f)
        return;

    const auto& head = headFor (piece);
    const auto r = head.bounds();

    g.setGradientFill (juce::ColourGradient (juce::Colours::white.withAlpha (0.5f * a.energy), head.cx, head.cy,
                                             juce::Colours::white.withAlpha (0.0f), head.cx + head.rx, head.cy, true));
    g.fillEllipse (r);

    if (a.ripple < 1.0f)
    {
        const auto scale = 0.15f + 0.85f * a.ripple;
        g.setColour (palette::accent.withAlpha ((1.0f - a.ripple) * (0.35f + 0.6f * a.strength)));
        g.drawEllipse (r.withSizeKeepingCentre (r.getWidth() * scale, r.getHeight() * scale), 5.0f);
    }

    g.setColour (palette::accent.withAlpha (0.85f * a.energy));
    g.drawEllipse (r.expanded (6.0f), 7.0f);
}

void DrumKitView::drawKickHit (juce::Graphics& g)
{
    const auto& a = anims[kit::kick];
    if (a.energy <= 0.0f && a.ripple >= 1.0f)
        return;

    g.setGradientFill (juce::ColourGradient (palette::accent.withAlpha (0.0f), 0.0f, kickGlowArea.getY(),
                                             palette::accent.withAlpha (0.28f * a.energy), 0.0f, kickGlowArea.getBottom(), false));
    g.fillRect (kickGlowArea);

    drawDrumHit (g, kit::kick);
}

void DrumKitView::drawCymbal (juce::Graphics& g, int piece)
{
    const auto& oval = headFor (piece);
    const auto& a = anims[(size_t) piece];
    const auto open = piece == kit::hihat ? hatOpen : 0.0f;

    // Seen from above a swinging cymbal mostly changes its apparent size; keep
    // the scale >= 1 so the redrawn cymbal always covers the original pixels.
    const auto sx = 1.0f + 0.03f * a.wobble * (1.0f + std::cos (a.phase)) + 0.045f * open;
    const auto sy = 1.0f + 0.03f * a.wobble * (1.0f + std::sin (a.phase)) + 0.045f * open;
    const auto transform = juce::AffineTransform::scale (sx, sy, oval.cx, oval.cy);

    auto outline = oval.path();
    outline.applyTransform (transform);

    if (open > 0.01f)
    {
        g.setColour (juce::Colours::black.withAlpha (0.35f * open));
        g.fillPath (outline, juce::AffineTransform::translation (10.0f, 14.0f));
    }

    {
        juce::Graphics::ScopedSaveState state (g);
        g.reduceClipRegion (outline);
        g.drawImageTransformed (photo, transform);

        if (a.energy > 0.0f)
        {
            // A sheen sweeping across the cymbal plus an overall brightening.
            const auto angle = a.phase * 0.5f;
            const auto dir = juce::Point<float> (std::cos (angle), std::sin (angle)) * oval.rx;
            juce::ColourGradient sheen (juce::Colours::white.withAlpha (0.0f), oval.centre() - dir,
                                        juce::Colours::white.withAlpha (0.0f), oval.centre() + dir, false);
            sheen.addColour (0.5, juce::Colours::white.withAlpha (0.45f * a.energy));
            g.setGradientFill (sheen);
            g.fillPath (outline);

            g.setGradientFill (juce::ColourGradient (juce::Colour (0xfffff2c8).withAlpha (0.35f * a.energy), oval.cx, oval.cy,
                                                     juce::Colour (0xfffff2c8).withAlpha (0.0f), oval.cx + oval.rx, oval.cy, true));
            g.fillPath (outline);
        }
    }

    if (a.energy > 0.0f)
    {
        g.setColour (palette::accent.withAlpha (0.8f * a.energy));
        g.strokePath (outline, juce::PathStrokeType (6.0f));
    }
}

void DrumKitView::drawPads (juce::Graphics& g)
{
    for (const auto& pad : pads)
    {
        g.setColour (juce::Colours::black.withAlpha (0.55f));
        g.fillRoundedRectangle (pad.bounds, 12.0f);

        if (pad.energy > 0.0f)
        {
            g.setColour (palette::accent.withAlpha (0.65f * pad.energy));
            g.fillRoundedRectangle (pad.bounds, 12.0f);
        }

        g.setColour (pad.energy > 0.0f ? palette::accent : palette::outline.brighter (0.3f));
        g.drawRoundedRectangle (pad.bounds, 12.0f, 3.0f);

        g.setColour (palette::text);
        g.setFont (juce::FontOptions (30.0f, juce::Font::bold));
        g.drawText (pad.label, pad.bounds, juce::Justification::centred);
    }
}

void DrumKitView::drawHighlights (juce::Graphics& g)
{
    if (hovered != nullptr)
    {
        g.setColour (juce::Colours::white.withAlpha (0.08f));
        g.fillPath (hovered->highlight);
        g.setColour (palette::accent.withAlpha (0.6f));
        g.strokePath (hovered->highlight, juce::PathStrokeType (3.0f));
    }

    if (learningPiece >= 0)
    {
        juce::Path area;
        for (const auto& z : zones)
            if (kit::articulations[(size_t) z.articulation].piece == learningPiece)
                area.addPath (z.highlight);

        const auto alpha = 0.45f + 0.4f * std::sin (learnPulse);
        g.setColour (palette::accent.withAlpha (alpha));
        g.strokePath (area, juce::PathStrokeType (6.0f));
    }
}
