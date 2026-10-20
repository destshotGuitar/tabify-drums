#pragma once

#include <juce_audio_processors/juce_audio_processors.h>

#include "DrumEngine.h"

class TabifyDrumsProcessor final : public juce::AudioProcessor, private juce::Timer
{
public:
    struct HitEvent
    {
        int articulation = -1;
        int velocity = 0;
        int note = -1;        // -1 when played from the editor
        bool learned = false; // the note was just assigned by MIDI learn
    };

    TabifyDrumsProcessor();
    ~TabifyDrumsProcessor() override;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override {}
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;
    using AudioProcessor::processBlock;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return JucePlugin_Name; }
    bool acceptsMidi() const override { return true; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 4.0; }

    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram (int) override {}
    const juce::String getProgramName (int) override { return {}; }
    void changeProgramName (int, const juce::String&) override {}

    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    // Editor interface. All of these are safe to call from the message thread.
    bool popHitEvent (HitEvent& event);
    void playFromEditor (int articulation, int velocity);
    float popPeak (int channel); // 0..numPieces-1 = pieces, numPieces = master

    void startMidiLearn (int articulation) { learnArticulation = articulation; }
    void cancelMidiLearn() { learnArticulation = -1; }
    int getMidiLearnArticulation() const { return learnArticulation; }
    juce::Array<int> getNotesFor (int articulation) const;
    void resetMappingFor (int articulation);
    void resetAllMappings();
    bool isKitLoading() const { return loadingKit >= 0; }

    juce::AudioProcessorValueTreeState apvts;

private:
    static juce::AudioProcessorValueTreeState::ParameterLayout createLayout();

    void timerCallback() override;
    int selectedKit() const;
    void startKitLoad (int kitIndex);

    void handleNoteOn (int note, int velocity);
    void trigger (int articulation, int velocity, int note, bool learned);
    juce::String serialiseNoteMap() const;
    void restoreNoteMap (const juce::String&);

    DrumEngine engine;

    struct PieceParams
    {
        std::atomic<float>* volume = nullptr;
        std::atomic<float>* pan = nullptr;
        std::atomic<float>* comp = nullptr;
        std::atomic<float>* mute = nullptr;
        std::atomic<float>* solo = nullptr;
    };
    std::array<PieceParams, kit::numPieces> pieceParams;
    std::atomic<float>* masterVolume = nullptr;
    std::atomic<float>* kitParam = nullptr;

    // Kits load on a background thread; the timer hands finished ones to the
    // engine. Both kit indices are only touched on the message thread.
    juce::ThreadPool loaderPool { 1 };
    juce::CriticalSection loadedLock;
    std::unique_ptr<DrumEngine::KitSamples> loadedSamples; // guarded by loadedLock
    std::atomic<double> currentSampleRate { 0.0 };
    std::atomic<int> activeKit { -1 };
    std::atomic<int> loadingKit { -1 };

    std::array<std::atomic<int>, 128> noteMap;
    std::atomic<int> learnArticulation { -1 };

    // Audio thread -> editor: hits to animate.
    juce::AbstractFifo hitFifo { 512 };
    std::array<HitEvent, 512> hitEvents;

    // Editor -> audio thread: pads clicked in the kit view.
    struct EditorHit
    {
        int articulation;
        int velocity;
    };
    juce::AbstractFifo editorFifo { 64 };
    std::array<EditorHit, 64> editorHits;

    std::array<std::atomic<float>, kit::numPieces + 1> peaks;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (TabifyDrumsProcessor)
};
