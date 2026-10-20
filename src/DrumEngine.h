#pragma once

#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_audio_formats/juce_audio_formats.h>

#include "KitDefinition.h"

// Sample playback for the built-in kit. Voices render into one mono buffer per
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

    DrumEngine();

    void prepare (double sampleRate, int maxBlockSize);

    void startBlock (int numSamples);
    void render (int startSample, int numSamples);
    void trigger (int articulation, float velocity);
    void stopAll();

    void mix (juce::AudioBuffer<float>& output,
              const std::array<ChannelSettings, kit::numPieces>& channels,
              float masterGain,
              Peaks& peaks);

private:
    struct Layer
    {
        juce::AudioBuffer<float> source;
        double sourceRate = 44100.0;
        juce::AudioBuffer<float> playback; // resampled to the host rate
    };

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

    void loadKit();
    Voice& allocateVoice();

    static constexpr int maxVoices = 64;

    std::array<std::vector<Layer>, kit::numArticulations> layers;
    std::array<Voice, maxVoices> voices;
    juce::uint64 voiceCounter = 0;

    juce::AudioBuffer<float> pieceBuffers;
    std::array<Compressor, kit::numPieces> compressors;
    std::array<juce::SmoothedValue<float>, kit::numPieces> gainLeft, gainRight;
    juce::SmoothedValue<float> master;

    double sampleRate = 44100.0;
    int chokeFadeSamples = 1000;
};
