#include "DrumKitView.h"
#include "DrumLookAndFeel.h"

namespace
{
struct DrumGeo
{
    float cx, top, w, headH, shellH;
};

struct CymbalGeo
{
    float cx, cy, w, h, tilt;
};

constexpr float floorY = 455.0f;

constexpr DrumGeo tom1Geo { 435.0f, 190.0f, 115.0f, 38.0f, 72.0f };
constexpr DrumGeo tom2Geo { 568.0f, 196.0f, 125.0f, 40.0f, 78.0f };
constexpr DrumGeo floorTomGeo { 782.0f, 300.0f, 160.0f, 50.0f, 118.0f };
constexpr DrumGeo snareGeo { 262.0f, 322.0f, 150.0f, 44.0f, 46.0f };

constexpr CymbalGeo crashGeo { 290.0f, 118.0f, 230.0f, 40.0f, -0.12f };
constexpr CymbalGeo rideGeo { 785.0f, 138.0f, 262.0f, 44.0f, 0.10f };
constexpr CymbalGeo splashGeo { 500.0f, 88.0f, 132.0f, 22.0f, 0.05f };
constexpr CymbalGeo hatGeo { 110.0f, 262.0f, 160.0f, 24.0f, 0.0f };

constexpr juce::Point<float> kickCentre { 500.0f, 338.0f };
constexpr float kickRadius = 110.0f;
constexpr juce::Point<float> cowbellCentre { 500.0f, 150.0f };

const juce::Colour shellWrap { 0xff8e1b22 };
const juce::Colour snareShell { 0xffaab1ba };

bool isCymbal (int piece)
{
    return piece == kit::crash || piece == kit::ride || piece == kit::splash || piece == kit::hihat;
}

juce::Path ellipsePath (float cx, float cy, float w, float h, float angle = 0.0f)
{
    juce::Path p;
    p.addEllipse (cx - w * 0.5f, cy - h * 0.5f, w, h);
    p.applyTransform (juce::AffineTransform::rotation (angle, cx, cy));
    return p;
}

juce::Path rectPath (float x, float y, float w, float h)
{
    juce::Path p;
    p.addRectangle (x, y, w, h);
    return p;
}

void chromeLine (juce::Graphics& g, juce::Point<float> a, juce::Point<float> b, float thickness)
{
    g.setColour (juce::Colour (0xff4b5058));
    g.drawLine ({ a, b }, thickness + 1.5f);
    g.setColour (juce::Colour (0xffb9bfc6));
    g.drawLine ({ a, b }, thickness);
    g.setColour (juce::Colours::white.withAlpha (0.45f));
    g.drawLine ({ a.translated (-thickness * 0.2f, 0.0f), b.translated (-thickness * 0.2f, 0.0f) }, thickness * 0.3f);
}

void tripod (juce::Graphics& g, float x, float top)
{
    chromeLine (g, { x, top }, { x, floorY - 30.0f }, 3.5f);
    const juce::Point<float> joint { x, floorY - 34.0f };
    chromeLine (g, joint, { x - 36.0f, floorY }, 2.5f);
    chromeLine (g, joint, { x + 36.0f, floorY }, 2.5f);
    chromeLine (g, joint, { x + 8.0f, floorY + 4.0f }, 2.5f);

    g.setColour (juce::Colour (0xff141414));
    g.fillEllipse (x - 40.0f, floorY - 2.0f, 8.0f, 4.0f);
    g.fillEllipse (x + 32.0f, floorY - 2.0f, 8.0f, 4.0f);
    g.fillEllipse (x + 4.0f, floorY + 2.0f, 8.0f, 4.0f);
}

void drawCymbal (juce::Graphics& g, float cx, float cy, float w, float h, float angle, float energy, bool flipped)
{
    juce::Graphics::ScopedSaveState state (g);
    g.addTransform (juce::AffineTransform::rotation (angle, cx, cy));

    const juce::Rectangle<float> r (cx - w * 0.5f, cy - h * 0.5f, w, h);

    // Edge thickness
    g.setColour (juce::Colour (0xff5a3a10));
    g.fillEllipse (r.translated (0.0f, flipped ? -2.0f : 2.0f));

    juce::ColourGradient body (juce::Colour (0xfff3d27a), cx - w * 0.18f, cy - h * 0.25f,
                               juce::Colour (0xff8f6020), cx + w * 0.5f, cy + h * 0.5f, true);
    body.addColour (0.45, juce::Colour (0xffd6a443));
    g.setGradientFill (body);
    g.fillEllipse (r);

    // Lathing grooves
    for (int i = 1; i < 8; ++i)
    {
        const auto ring = r.reduced (w * 0.5f * (float) i / 9.0f, h * 0.5f * (float) i / 9.0f);
        g.setColour (juce::Colours::black.withAlpha (0.08f));
        g.drawEllipse (ring, 0.8f);
        g.setColour (juce::Colours::white.withAlpha (0.07f));
        g.drawEllipse (ring.translated (0.0f, -0.7f), 0.6f);
    }

    const auto bell = juce::Rectangle<float> (w * 0.2f, h * 0.8f)
                          .withCentre ({ cx, cy + (flipped ? h * 0.14f : -h * 0.14f) });
    g.setGradientFill (juce::ColourGradient (juce::Colour (0xfffbe39a), bell.getX() + bell.getWidth() * 0.3f, bell.getY(),
                                             juce::Colour (0xffa07024), bell.getRight(), bell.getBottom(), false));
    g.fillEllipse (bell);

    if (! flipped)
    {
        g.setColour (juce::Colour (0xff2b2b2b));
        g.fillEllipse (juce::Rectangle<float> (6.0f, 3.5f).withCentre (bell.getCentre().translated (0.0f, -bell.getHeight() * 0.15f)));
    }

    g.setColour (juce::Colour (0xff6b4712));
    g.drawEllipse (r, 1.0f);

    if (energy > 0.005f)
    {
        juce::ColourGradient flash (juce::Colours::white.withAlpha (0.8f * energy), cx, cy,
                                    juce::Colours::white.withAlpha (0.0f), cx + w * 0.55f, cy, true);
        g.setGradientFill (flash);
        g.fillEllipse (r.expanded (w * 0.03f * energy, h * 0.12f * energy));
    }
}

// A drum seen from the front and slightly above: shell, lugs, hoops and head.
void drawDrumBody (juce::Graphics& g, const DrumGeo& d, juce::Colour wrap, int lugs,
                   float energy, float ripple, float strength)
{
    const auto left = d.cx - d.w * 0.5f;
    const auto right = d.cx + d.w * 0.5f;
    const auto bottom = d.top + d.shellH;
    const auto halfPi = juce::MathConstants<float>::halfPi;

    juce::Path shell;
    shell.startNewSubPath (left, d.top);
    shell.lineTo (left, bottom);
    shell.addCentredArc (d.cx, bottom, d.w * 0.5f, d.headH * 0.5f, 0.0f, 3.0f * halfPi, halfPi);
    shell.lineTo (right, d.top);
    shell.closeSubPath();

    juce::ColourGradient wrapGradient (wrap.darker (0.9f), left, 0.0f, wrap.darker (1.2f), right, 0.0f, false);
    wrapGradient.addColour (0.28, wrap.brighter (0.45f));
    wrapGradient.addColour (0.55, wrap);
    g.setGradientFill (wrapGradient);
    g.fillPath (shell);

    {
        juce::Graphics::ScopedSaveState state (g);
        g.reduceClipRegion (shell);
        g.setColour (juce::Colours::white.withAlpha (0.13f));
        g.fillRect (left + d.w * 0.22f, d.top, d.w * 0.05f, d.shellH + d.headH);
    }

    // Lugs, spaced around the visible half of the shell
    for (int i = 0; i < lugs; ++i)
    {
        const auto theta = juce::MathConstants<float>::pi * ((float) i + 0.5f) / (float) lugs;
        const auto x = d.cx - std::cos (theta) * d.w * 0.48f;
        const auto lugW = 2.0f + 6.0f * std::sin (theta);
        const auto lugH = juce::jmin (d.shellH * 0.45f, 26.0f);
        const auto lugY = d.top + d.shellH * 0.5f - lugH * 0.5f + std::sin (theta) * d.headH * 0.25f;

        g.setGradientFill (juce::ColourGradient (juce::Colour (0xfff0f2f5), x - lugW * 0.5f, 0.0f,
                                                 juce::Colour (0xff6c727a), x + lugW * 0.5f, 0.0f, false));
        g.fillRoundedRectangle (x - lugW * 0.5f, lugY, lugW, lugH, 2.0f);

        g.setColour (juce::Colour (0xff9aa1a9));
        g.drawLine (x, d.top + std::sin (theta) * d.headH * 0.5f, x, lugY, 1.0f);
    }

    juce::Path bottomHoop;
    bottomHoop.addCentredArc (d.cx, bottom, d.w * 0.5f, d.headH * 0.5f, 0.0f, 3.0f * halfPi, halfPi, true);
    g.setColour (juce::Colour (0xffc4cad1));
    g.strokePath (bottomHoop, juce::PathStrokeType (3.0f));

    const juce::Rectangle<float> hoop (left - 2.0f, d.top - d.headH * 0.5f - 1.5f, d.w + 4.0f, d.headH + 3.0f);
    g.setColour (juce::Colour (0xff80868e));
    g.fillEllipse (hoop);

    const auto head = hoop.reduced (3.5f, 2.5f);
    auto headLight = juce::Colour (0xfff7f4ec).interpolatedWith (juce::Colours::white, energy);
    auto headDark = juce::Colour (0xffb8b0a0).interpolatedWith (juce::Colour (0xffeae4d6), energy);
    g.setGradientFill (juce::ColourGradient (headLight, head.getX() + head.getWidth() * 0.3f, head.getY(),
                                             headDark, head.getRight(), head.getBottom(), false));
    g.fillEllipse (head);

    if (ripple < 1.0f)
    {
        const auto scale = 0.15f + 0.85f * ripple;
        g.setColour (palette::accent.withAlpha ((1.0f - ripple) * (0.3f + 0.6f * strength)));
        g.drawEllipse (head.withSizeKeepingCentre (head.getWidth() * scale, head.getHeight() * scale), 2.0f);
    }

    juce::Path frontHoop;
    frontHoop.addCentredArc (hoop.getCentreX(), hoop.getCentreY(), hoop.getWidth() * 0.5f, hoop.getHeight() * 0.5f,
                             0.0f, 3.0f * halfPi, halfPi, true);
    g.setColour (juce::Colour (0xffe3e7eb));
    g.strokePath (frontHoop, juce::PathStrokeType (2.5f));

    if (energy > 0.005f)
    {
        g.setGradientFill (juce::ColourGradient (palette::accent.withAlpha (0.35f * energy), head.getCentreX(), head.getCentreY(),
                                                 palette::accent.withAlpha (0.0f), head.getRight(), head.getCentreY(), true));
        g.fillEllipse (head.expanded (6.0f * energy));
    }
}
} // namespace

DrumKitView::DrumKitView()
{
    setSize (designWidth, designHeight);
    buildZones();
    startTimerHz (60);
}

void DrumKitView::buildZones()
{
    const juce::String more = juce::CharPointer_UTF8 ("  \xc2\xb7  right-click: MIDI learn & more");

    const auto hatHelp = "Hi-Hat: center = closed, edge = open, Shift-click = semi-open, pedal = foot chick" + more;
    zones.push_back ({ kit::hatPedal, rectPath (82.0f, 416.0f, 56.0f, 44.0f), "Hi-Hat pedal" + more });
    zones.push_back ({ kit::hatClosed, ellipsePath (hatGeo.cx, hatGeo.cy - 6.0f, 96.0f, 30.0f), hatHelp });
    zones.push_back ({ kit::hatOpen, ellipsePath (hatGeo.cx, hatGeo.cy - 6.0f, hatGeo.w + 14.0f, 52.0f), hatHelp });

    const auto snareHelp = "Snare: center = snare, rim = rimshot, shell = side stick" + more;
    zones.push_back ({ kit::snareHit, ellipsePath (snareGeo.cx, snareGeo.top, snareGeo.w * 0.72f, snareGeo.headH * 0.72f), snareHelp });
    zones.push_back ({ kit::snareRimshot, ellipsePath (snareGeo.cx, snareGeo.top, snareGeo.w + 8.0f, snareGeo.headH + 8.0f), snareHelp });
    zones.push_back ({ kit::snareSideStick, rectPath (snareGeo.cx - snareGeo.w * 0.5f, snareGeo.top,
                                                      snareGeo.w, snareGeo.shellH + snareGeo.headH * 0.5f), snareHelp });

    auto drumZone = [] (const DrumGeo& d)
    {
        auto p = ellipsePath (d.cx, d.top, d.w + 6.0f, d.headH + 6.0f);
        p.addRectangle (d.cx - d.w * 0.5f, d.top, d.w, d.shellH);
        return p;
    };

    zones.push_back ({ kit::floorTomHit, drumZone (floorTomGeo), "Floor Tom" + more });
    zones.push_back ({ kit::kickHit, ellipsePath (kickCentre.x, kickCentre.y, kickRadius * 2.0f + 24.0f, kickRadius * 2.0f + 24.0f), "Kick" + more });
    zones.push_back ({ kit::cowbellHit, rectPath (cowbellCentre.x - 20.0f, cowbellCentre.y - 16.0f, 40.0f, 34.0f), "Cowbell (Hand Clap via right-click)" + more });
    zones.push_back ({ kit::tom1Hit, drumZone (tom1Geo), "Tom 1" + more });
    zones.push_back ({ kit::tom2Hit, drumZone (tom2Geo), "Tom 2" + more });

    auto cymbalZone = [] (const CymbalGeo& c, float widthScale = 1.0f, float heightScale = 2.0f)
    {
        return ellipsePath (c.cx, c.cy, c.w * widthScale, c.h * heightScale, c.tilt);
    };

    zones.push_back ({ kit::splashHit, cymbalZone (splashGeo), "Splash" + more });
    zones.push_back ({ kit::rideBell, ellipsePath (rideGeo.cx, rideGeo.cy - rideGeo.h * 0.12f, rideGeo.w * 0.24f, rideGeo.h * 1.1f, rideGeo.tilt),
                       "Ride: bow = ride, center = bell" + more });
    zones.push_back ({ kit::rideHit, cymbalZone (rideGeo), "Ride: bow = ride, center = bell" + more });
    zones.push_back ({ kit::crashHit, cymbalZone (crashGeo), "Crash" + more });
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
    const auto* zone = zoneAt (e.position);
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
    const auto* zone = zoneAt (e.position);
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

    switch (articulation)
    {
        case kit::hatClosed: hatOpenTarget = 0.0f; a.wobble *= 0.3f; break;
        case kit::hatPedal:  hatOpenTarget = 0.0f; hatBounce = 1.0f; a.wobble = 0.0f; break;
        case kit::hatSemiOpen: hatOpenTarget = 0.45f; break;
        case kit::hatOpen:   hatOpenTarget = 1.0f; break;
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
        a.energy *= isCymbal ((int) p) ? 0.93f : 0.86f;
        a.ripple = juce::jmin (1.0f, a.ripple + dt * 2.5f);
        a.wobble *= 0.965f;
        a.phase += dt * juce::MathConstants<float>::twoPi * 3.0f;

        if (a.energy < 0.002f) a.energy = 0.0f;
        if (a.wobble < 0.002f) a.wobble = 0.0f;
        active = active || a.energy > 0.0f || a.ripple < 1.0f || a.wobble > 0.0f;
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
    drawBackground (g);
    drawStands (g);
    drawCymbalPiece (g, kit::ride);
    drawCymbalPiece (g, kit::crash);
    drawCymbalPiece (g, kit::splash);
    drawCowbell (g);
    drawTom (g, kit::tom1);
    drawTom (g, kit::tom2);
    drawKick (g);
    drawFloorTom (g);
    drawSnare (g);
    drawHiHat (g);
    drawHighlights (g);
}

void DrumKitView::drawBackground (juce::Graphics& g)
{
    const auto bounds = getLocalBounds().toFloat();

    g.setGradientFill (juce::ColourGradient (juce::Colour (0xff3c4352), 500.0f, 40.0f,
                                             juce::Colour (0xff101216), 500.0f, 600.0f, true));
    g.fillRect (bounds);

    // Floor rug
    g.setGradientFill (juce::ColourGradient (juce::Colour (0xff4d2c25), 500.0f, 440.0f,
                                             juce::Colour (0xff1a100e), 500.0f, 540.0f, true));
    g.fillEllipse (20.0f, 398.0f, 960.0f, 118.0f);

    // Spotlight cone
    juce::Path cone;
    cone.startNewSubPath (430.0f, 0.0f);
    cone.lineTo (570.0f, 0.0f);
    cone.lineTo (900.0f, 470.0f);
    cone.lineTo (100.0f, 470.0f);
    cone.closeSubPath();
    g.setGradientFill (juce::ColourGradient (juce::Colours::white.withAlpha (0.06f), 500.0f, 0.0f,
                                             juce::Colours::white.withAlpha (0.0f), 500.0f, 470.0f, false));
    g.fillPath (cone);
}

void DrumKitView::drawStands (juce::Graphics& g)
{
    // Crash and ride on boom stands
    chromeLine (g, { crashGeo.cx, crashGeo.cy }, { 325.0f, 212.0f }, 3.0f);
    tripod (g, 325.0f, 212.0f);
    chromeLine (g, { rideGeo.cx, rideGeo.cy }, { 735.0f, 218.0f }, 3.0f);
    tripod (g, 735.0f, 218.0f);

    // Splash and cowbell sit on a post rising from the kick
    chromeLine (g, { kickCentre.x, kickCentre.y - kickRadius }, { splashGeo.cx, splashGeo.cy }, 3.0f);

    // Tom arms
    chromeLine (g, { kickCentre.x - 6.0f, kickCentre.y - kickRadius + 4.0f },
                { tom1Geo.cx, tom1Geo.top + tom1Geo.shellH * 0.6f }, 4.0f);
    chromeLine (g, { kickCentre.x + 6.0f, kickCentre.y - kickRadius + 4.0f },
                { tom2Geo.cx, tom2Geo.top + tom2Geo.shellH * 0.6f }, 4.0f);
}

void DrumKitView::drawCymbalPiece (juce::Graphics& g, int piece)
{
    const auto& geo = piece == kit::crash ? crashGeo : piece == kit::ride ? rideGeo : splashGeo;
    const auto& a = anims[(size_t) piece];
    const auto swing = std::sin (a.phase) * a.wobble;
    const auto bob = std::sin (a.phase + 1.2f) * a.wobble;

    drawCymbal (g, geo.cx, geo.cy, geo.w, geo.h * (1.0f + 0.25f * bob),
                geo.tilt + 0.07f * swing, a.energy, false);
}

void DrumKitView::drawTom (juce::Graphics& g, int piece)
{
    const auto& a = anims[(size_t) piece];
    drawDrumBody (g, piece == kit::tom1 ? tom1Geo : tom2Geo, shellWrap, 6, a.energy, a.ripple, a.strength);
}

void DrumKitView::drawFloorTom (juce::Graphics& g)
{
    const auto& d = floorTomGeo;
    const auto left = d.cx - d.w * 0.5f;
    const auto right = d.cx + d.w * 0.5f;
    chromeLine (g, { left + 8.0f, d.top + d.shellH * 0.3f }, { left - 12.0f, floorY }, 3.0f);
    chromeLine (g, { right - 8.0f, d.top + d.shellH * 0.3f }, { right + 12.0f, floorY }, 3.0f);

    const auto& a = anims[kit::floorTom];
    drawDrumBody (g, d, shellWrap, 8, a.energy, a.ripple, a.strength);
}

void DrumKitView::drawSnare (juce::Graphics& g)
{
    const auto& d = snareGeo;
    const auto bottom = d.top + d.shellH + d.headH * 0.5f;
    tripod (g, d.cx, bottom);
    chromeLine (g, { d.cx, bottom }, { d.cx - d.w * 0.32f, bottom - 6.0f }, 2.5f);
    chromeLine (g, { d.cx, bottom }, { d.cx + d.w * 0.32f, bottom - 6.0f }, 2.5f);

    const auto& a = anims[kit::snare];
    drawDrumBody (g, d, snareShell, 8, a.energy, a.ripple, a.strength);
}

void DrumKitView::drawKick (juce::Graphics& g)
{
    const auto c = kickCentre;
    const auto r = kickRadius;
    const auto& a = anims[kit::kick];

    chromeLine (g, { c.x - r * 0.8f, c.y + r * 0.45f }, { c.x - r * 1.3f, floorY }, 3.0f);
    chromeLine (g, { c.x + r * 0.8f, c.y + r * 0.45f }, { c.x + r * 1.3f, floorY }, 3.0f);

    // Hoop with wrap finish
    const auto outer = juce::Rectangle<float> ((r + 12.0f) * 2.0f, (r + 12.0f) * 2.0f).withCentre (c);
    g.setGradientFill (juce::ColourGradient (shellWrap.brighter (0.5f), c.x, c.y - r,
                                             shellWrap.darker (1.0f), c.x, c.y + r, false));
    g.fillEllipse (outer);
    g.setColour (juce::Colours::black.withAlpha (0.5f));
    g.drawEllipse (outer, 1.0f);

    // Claws
    for (int i = 0; i < 10; ++i)
    {
        const auto angle = juce::MathConstants<float>::twoPi * (float) i / 10.0f + juce::MathConstants<float>::pi / 10.0f;
        const auto p = c.getPointOnCircumference (r + 6.0f, angle);
        juce::Path claw;
        claw.addRoundedRectangle (p.x - 3.5f, p.y - 8.0f, 7.0f, 16.0f, 2.0f);
        claw.applyTransform (juce::AffineTransform::rotation (angle, p.x, p.y));
        g.setColour (juce::Colour (0xffd0d5db));
        g.fillPath (claw);
        g.setColour (juce::Colour (0xff5c6168));
        g.strokePath (claw, juce::PathStrokeType (0.8f));
    }

    juce::Graphics::ScopedSaveState state (g);
    const auto pulse = 1.0f + 0.025f * a.energy;
    g.addTransform (juce::AffineTransform::scale (pulse, pulse, c.x, c.y));

    const auto head = juce::Rectangle<float> (r * 2.0f, r * 2.0f).withCentre (c);
    g.setGradientFill (juce::ColourGradient (juce::Colour (0xff30333a).interpolatedWith (juce::Colour (0xff4a4f58), a.energy),
                                             c.x - r * 0.3f, c.y - r * 0.4f,
                                             juce::Colour (0xff0b0c0f), c.x + r * 0.7f, c.y + r * 0.8f, true));
    g.fillEllipse (head);
    g.setColour (juce::Colours::white.withAlpha (0.08f));
    g.drawEllipse (head.reduced (8.0f), 1.0f);

    g.setColour (palette::accent.interpolatedWith (juce::Colours::white, a.energy * 0.5f));
    g.setFont (juce::FontOptions (30.0f, juce::Font::bold));
    g.drawText ("OPEN DRUMS", juce::Rectangle<float> (c.x - r, c.y - 26.0f, r * 2.0f, 34.0f), juce::Justification::centred);
    g.setColour (palette::textDim);
    g.setFont (juce::FontOptions (11.0f, juce::Font::bold));
    g.drawText ("GM ROCK KIT", juce::Rectangle<float> (c.x - r, c.y + 8.0f, r * 2.0f, 16.0f), juce::Justification::centred);

    // Port hole
    const auto port = juce::Rectangle<float> (30.0f, 30.0f).withCentre ({ c.x + r * 0.45f, c.y + r * 0.5f });
    g.setColour (juce::Colours::black);
    g.fillEllipse (port);
    g.setColour (juce::Colour (0xff8a9099));
    g.drawEllipse (port, 2.0f);

    if (a.ripple < 1.0f)
    {
        const auto scale = 0.2f + 0.8f * a.ripple;
        g.setColour (palette::accent.withAlpha ((1.0f - a.ripple) * (0.3f + 0.6f * a.strength)));
        g.drawEllipse (head.withSizeKeepingCentre (head.getWidth() * scale, head.getHeight() * scale), 3.0f);
    }

    if (a.energy > 0.005f)
    {
        g.setGradientFill (juce::ColourGradient (palette::accent.withAlpha (0.25f * a.energy), c.x, c.y,
                                                 palette::accent.withAlpha (0.0f), c.x + r, c.y, true));
        g.fillEllipse (head);
    }
}

void DrumKitView::drawHiHat (juce::Graphics& g)
{
    const auto& h = hatGeo;
    const auto& a = anims[kit::hihat];
    const auto bottomY = h.cy;
    const auto topY = h.cy - 4.0f - hatOpen * 16.0f + hatBounce * 2.0f;

    // Stand and pedal
    tripod (g, h.cx, topY - 22.0f);
    const auto press = hatBounce * 4.0f;
    juce::Path pedal;
    pedal.startNewSubPath (h.cx - 20.0f, floorY + 2.0f);
    pedal.lineTo (h.cx + 20.0f, floorY + 2.0f);
    pedal.lineTo (h.cx + 15.0f, floorY - 28.0f + press);
    pedal.lineTo (h.cx - 15.0f, floorY - 28.0f + press);
    pedal.closeSubPath();
    g.setGradientFill (juce::ColourGradient (juce::Colour (0xff3b3f46), h.cx, floorY - 28.0f,
                                             juce::Colour (0xff15171a), h.cx, floorY, false));
    g.fillPath (pedal);
    g.setColour (juce::Colour (0xff9aa1a9));
    g.strokePath (pedal, juce::PathStrokeType (1.2f));

    drawCymbal (g, h.cx, bottomY, h.w, h.h * 0.9f, 0.0f, a.energy * 0.4f, true);

    const auto swing = std::sin (a.phase) * a.wobble * hatOpen;
    drawCymbal (g, h.cx, topY, h.w, h.h, 0.05f * swing, a.energy, false);

    // Clutch
    g.setColour (juce::Colour (0xffc9ced4));
    g.fillRoundedRectangle (h.cx - 5.0f, topY - 14.0f, 10.0f, 8.0f, 2.0f);
}

void DrumKitView::drawCowbell (juce::Graphics& g)
{
    const auto c = cowbellCentre;
    const auto& a = anims[kit::perc];

    juce::Path bell;
    bell.startNewSubPath (c.x - 9.0f, c.y - 14.0f);
    bell.lineTo (c.x + 9.0f, c.y - 14.0f);
    bell.lineTo (c.x + 18.0f, c.y + 12.0f);
    bell.lineTo (c.x - 18.0f, c.y + 12.0f);
    bell.closeSubPath();

    g.setGradientFill (juce::ColourGradient (juce::Colour (0xff6b7079).interpolatedWith (juce::Colours::white, a.energy * 0.6f),
                                             c.x - 18.0f, c.y, juce::Colour (0xff1e2024), c.x + 18.0f, c.y, false));
    g.fillPath (bell);
    g.setColour (juce::Colour (0xffb5bbc2));
    g.strokePath (bell, juce::PathStrokeType (1.0f));
    g.setColour (juce::Colours::black);
    g.fillEllipse (c.x - 17.0f, c.y + 9.0f, 34.0f, 7.0f);

    if (a.energy > 0.005f)
    {
        g.setGradientFill (juce::ColourGradient (palette::accent.withAlpha (0.5f * a.energy), c.x, c.y,
                                                 palette::accent.withAlpha (0.0f), c.x + 30.0f, c.y, true));
        g.fillEllipse (c.x - 30.0f, c.y - 30.0f, 60.0f, 60.0f);
    }
}

void DrumKitView::drawHighlights (juce::Graphics& g)
{
    if (hovered != nullptr)
    {
        g.setColour (juce::Colours::white.withAlpha (0.06f));
        g.fillPath (hovered->area);
        g.setColour (palette::accent.withAlpha (0.55f));
        g.strokePath (hovered->area, juce::PathStrokeType (1.5f));
    }

    if (learningPiece >= 0)
    {
        juce::Path area;
        for (const auto& z : zones)
            if (kit::articulations[(size_t) z.articulation].piece == learningPiece)
                area.addPath (z.area);

        const auto alpha = 0.45f + 0.4f * std::sin (learnPulse);
        g.setColour (palette::accent.withAlpha (alpha));
        g.strokePath (area, juce::PathStrokeType (3.0f));
    }
}
