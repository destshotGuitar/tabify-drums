#pragma once

#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_audio_formats/juce_audio_formats.h>

#include "KitDefinition.h"

// Sample playback for the built-in kits. Voices render into one mono buffer per
// piece; each piece then goes through its own compressor, fader and pan before
// being summed into the stereo output.
class DrumEngine
{
public:
    struct ChannelSettings
    {
        float gain = 1.0f;
        float pan = 0.0f;  // -1 left .. +1 right
        float comp = 0.0f; // 0 = off .. 1 = heavy
        bool audible = true;
    };

    using Peaks = std::array<float, kit::numPieces + 1>; // pieces, then master

    // The decoded samples of one kit, resampled to one sample rate.
    struct KitSamples
    {
        int kitIndex = 0;
        double sampleRate = 0.0;
        std::array<std::vector<juce::AudioBuffer<float>>, kit::numArticulations> layers;
    };

    // Decodes a kit. Slow: call it from a background thread, or from
    // prepareToPlay while audio is stopped.
    static std::unique_ptr<KitSamples> loadKit (int kitIndex, double sampleRate);

    void prepare (double sampleRate, int maxBlockSize);

    // Message thread. The audio thread switches to the new kit at the start of
    // a block; the previous one is freed by a later collectGarbage().
    void setKit (std::unique_ptr<KitSamples>);
    void collectGarbage();

    // Message thread while audio is stopped (prepareToPlay): switch at once.
    void setKitNow (std::unique_ptr<KitSamples>);

    void startBlock (int numSamples);
    void render (int startSample, int numSamples);
    void trigger (int articulation, float velocity);
    void stopAll();

    void mix (juce::AudioBuffer<float>& output,
              const std::array<ChannelSettings, kit::numPieces>& channels,
              float masterGain,
              Peaks& peaks);

private:
    struct Voice
    {
        const juce::AudioBuffer<float>* sample = nullptr;
        int position = 0;
        float gain = 0.0f;
        int piece = -1;
        int chokeGroup = kit::chokeNone;
        int fadeLeft = 0; // > 0 while fading out
        int fadeLength = 0;
        juce::uint64 age = 0;
    };

    class Compressor
    {
    public:
        void prepare (double sampleRate);
        void process (float* samples, int numSamples, float amount);

    private:
        float envelope = 0.0f;
        float attackCoef = 0.0f;
        float releaseCoef = 0.0f;
    };

    Voice& allocateVoice();
    void collectGarbageLocked();

    static constexpr int maxVoices = 64;

    // Kit hand-over. `requested` is written by the message thread and `active`
    // by the audio thread, both under kitLock. The audio thread only try-locks,
    // so it never waits, and may read `active` without the lock since nothing
    // else writes it.
    juce::SpinLock kitLock;
    KitSamples* active = nullptr;
    KitSamples* requested = nullptr;
    std::vector<std::unique_ptr<KitSamples>> kitStore; // message thread only

    std::array<Voice, maxVoices> voices;
    juce::uint64 voiceCounter = 0;

    juce::AudioBuffer<float> pieceBuffers;
    std::array<Compressor, kit::numPieces> compressors;
    std::array<juce::SmoothedValue<float>, kit::numPieces> gainLeft, gainRight;
    juce::SmoothedValue<float> master;
    bool gainsSet = false; // the first block jumps straight to the target gains

    double sampleRate = 44100.0;
    int chokeFadeSamples = 1000;
};
