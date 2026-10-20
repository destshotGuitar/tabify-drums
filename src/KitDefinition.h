#pragma once

#include <array>
#include <string>
#include <vector>

namespace kit
{
// A piece is a physical drum or cymbal: it has one mixer channel and one
// picture in the kit view. An articulation is a way of hitting a piece and is
// what MIDI notes map to.
enum Piece
{
    kick, snare, hihat, tom1, tom2, floorTom, crash, ride, splash, perc,
    numPieces
};

struct PieceInfo
{
    const char* id;
    const char* name;
};

inline constexpr std::array<PieceInfo, numPieces> pieces {{
    { "kick",     "Kick" },
    { "snare",    "Snare" },
    { "hihat",    "Hi-Hat" },
    { "tom1",     "Tom 1" },
    { "tom2",     "Tom 2" },
    { "floortom", "Floor Tom" },
    { "crash",    "Crash" },
    { "ride",     "Ride" },
    { "splash",   "Splash" },
    { "perc",     "Perc" },
}};

enum Articulation
{
    kickHit,
    snareHit, snareRimshot, snareSideStick,
    hatClosed, hatPedal, hatSemiOpen, hatOpen,
    tom1Hit, tom2Hit, floorTomHit,
    crashHit,
    rideHit, rideBell, ride2Hit,
    splashHit,
    cowbellHit, clapHit,
    numArticulations
};

// How the velocity layer files of an articulation are named.
enum class LayerNaming
{
    graded,   // Name-Softest.wav ... Name-Hardest.wav
    numbered, // Name-1.wav (softest) ... Name-5.wav (hardest)
    single    // Name.wav
};

inline constexpr int chokeNone = -1;
inline constexpr int chokeHiHat = 0;

struct ArticulationInfo
{
    const char* id;
    const char* name;
    Piece piece;
    const char* sample;
    LayerNaming naming;
    int chokeGroup;
    std::array<int, 3> defaultNotes; // -1 = unused slot
};

// Default notes follow the General MIDI drum map.
inline constexpr std::array<ArticulationInfo, numArticulations> articulations {{
    { "kick",      "Kick",        kick,     "Kick",         LayerNaming::graded,   chokeNone,  { 36, 35, -1 } },
    { "snare",     "Snare",       snare,    "Snare",        LayerNaming::graded,   chokeNone,  { 38, -1, -1 } },
    { "rimshot",   "Rimshot",     snare,    "SnareRimshot", LayerNaming::graded,   chokeNone,  { 40, -1, -1 } },
    { "sidestick", "Side Stick",  snare,    "SideStick",    LayerNaming::graded,   chokeNone,  { 37, -1, -1 } },
    { "hhclosed",  "Closed",      hihat,    "HatClosed",    LayerNaming::graded,   chokeHiHat, { 42, -1, -1 } },
    { "hhpedal",   "Pedal",       hihat,    "HatPedal",     LayerNaming::graded,   chokeHiHat, { 44, -1, -1 } },
    { "hhsemi",    "Semi-Open",   hihat,    "HatSemiOpen",  LayerNaming::graded,   chokeHiHat, { 23, -1, -1 } },
    { "hhopen",    "Open",        hihat,    "HatOpen",      LayerNaming::graded,   chokeHiHat, { 46, -1, -1 } },
    { "tom1",      "Tom 1",       tom1,     "Tom1",         LayerNaming::graded,   chokeNone,  { 48, 50, -1 } },
    { "tom2",      "Tom 2",       tom2,     "Tom2",         LayerNaming::graded,   chokeNone,  { 45, 47, -1 } },
    { "floortom",  "Floor Tom",   floorTom, "TomFloor",     LayerNaming::graded,   chokeNone,  { 41, 43, -1 } },
    { "crash",     "Crash",       crash,    "Crash",        LayerNaming::graded,   chokeNone,  { 49, 57, -1 } },
    { "ride",      "Ride",        ride,     "Ride",         LayerNaming::graded,   chokeNone,  { 51, -1, -1 } },
    { "ridebell",  "Ride Bell",   ride,     "Bell",         LayerNaming::graded,   chokeNone,  { 53, -1, -1 } },
    { "ride2",     "Ride 24\"",   ride,     "24Ride",       LayerNaming::numbered, chokeNone,  { 59, -1, -1 } },
    { "splash",    "Splash",      splash,   "Splash",       LayerNaming::graded,   chokeNone,  { 55, -1, -1 } },
    { "cowbell",   "Cowbell",     perc,     "Cowbell",      LayerNaming::graded,   chokeNone,  { 56, -1, -1 } },
    { "clap",      "Hand Clap",   perc,     "HandClap",     LayerNaming::single,   chokeNone,  { 39, -1, -1 } },
}};

// Lower velocity bound (0..1) of each of the five layers, taken from the
// kit's Hydrogen drumkit.xml.
inline constexpr std::array<float, 5> layerLowerBounds { 0.0f, 0.2f, 0.38f, 0.58f, 0.79f };

// Sample file names of an articulation, softest layer first.
std::vector<std::string> layerFiles (const ArticulationInfo& info);

std::string paramId (const char* prefix, int piece);
} // namespace kit
