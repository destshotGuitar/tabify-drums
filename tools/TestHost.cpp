// Loads the built plugin the way a DAW would, renders a beat to a WAV file,
// checks every articulation makes sound and round-trips the state.
//
// usage: TabifyDrumsTestHost <plugin.vst3> <output-dir>

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_audio_formats/juce_audio_formats.h>
#include <juce_gui_basics/juce_gui_basics.h>

namespace
{
constexpr double sampleRate = 48000.0;
constexpr int blockSize = 256;

int failures = 0;

void check (bool ok, const juce::String& what)
{
    std::cout << (ok ? "  ok    " : "  FAIL  ") << what << std::endl;
    if (! ok)
        ++failures;
}

// Render `numSamples` with the given note-ons, returning the peak level.
float render (juce::AudioPluginInstance& plugin, juce::AudioBuffer<float>& out, int start, int numSamples,
              const std::vector<std::pair<int, int>>& notes /* sample offset, note */, int velocity = 110)
{
    float peak = 0.0f;
    juce::AudioBuffer<float> block (2, blockSize);

    for (int pos = 0; pos < numSamples; pos += blockSize)
    {
        const auto n = juce::jmin (blockSize, numSamples - pos);
        juce::MidiBuffer midi;
        for (const auto& [offset, note] : notes)
            if (offset >= pos && offset < pos + n)
                midi.addEvent (juce::MidiMessage::noteOn (10, note, (juce::uint8) velocity), offset - pos);

        block.setSize (2, n, false, false, true);
        block.clear();
        plugin.processBlock (block, midi);

        for (int ch = 0; ch < 2; ++ch)
        {
            peak = juce::jmax (peak, block.getMagnitude (ch, 0, n));
            if (start + pos + n <= out.getNumSamples())
                out.copyFrom (ch, start + pos, block, ch, 0, n);
        }
    }
    return peak;
}
} // namespace

int main (int argc, char** argv)
{
    if (argc < 3)
    {
        std::cerr << "usage: TabifyDrumsTestHost <plugin.vst3> <output-dir>\n";
        return 2;
    }

    juce::ScopedJuceInitialiser_GUI gui;
    const juce::File pluginFile (juce::File::getCurrentWorkingDirectory().getChildFile (argv[1]));
    const juce::File outDir (juce::File::getCurrentWorkingDirectory().getChildFile (argv[2]));
    outDir.createDirectory();

    juce::AudioPluginFormatManager formats;
    formats.addFormat (std::make_unique<juce::VST3PluginFormat>());

    juce::OwnedArray<juce::PluginDescription> found;
    juce::VST3PluginFormat vst3;
    vst3.findAllTypesForFile (found, pluginFile.getFullPathName());
    if (found.isEmpty())
    {
        std::cerr << "no plugin found in " << pluginFile.getFullPathName() << "\n";
        return 1;
    }

    std::cout << "Loaded description: " << found[0]->name << " (" << found[0]->pluginFormatName
              << ", instrument=" << (found[0]->isInstrument ? "yes" : "no") << ")\n";

    juce::String error;
    auto plugin = formats.createPluginInstance (*found[0], sampleRate, blockSize, error);
    if (plugin == nullptr)
    {
        std::cerr << "failed to instantiate: " << error << "\n";
        return 1;
    }

    plugin->setPlayConfigDetails (0, 2, sampleRate, blockSize);
    plugin->prepareToPlay (sampleRate, blockSize);

    std::cout << "Articulations (GM notes):\n";
    const std::vector<std::pair<juce::String, int>> articulations {
        { "kick", 36 }, { "snare", 38 }, { "rimshot", 40 }, { "side stick", 37 },
        { "hh closed", 42 }, { "hh pedal", 44 }, { "hh semi", 23 }, { "hh open", 46 },
        { "tom 1", 48 }, { "tom 2", 45 }, { "floor tom", 41 }, { "crash", 49 },
        { "ride", 51 }, { "ride bell", 53 }, { "ride 24", 59 }, { "splash", 55 },
        { "cowbell", 56 }, { "clap", 39 },
    };

    juce::AudioBuffer<float> scratch (2, 1);
    for (const auto& [name, note] : articulations)
    {
        const auto peak = render (*plugin, scratch, 1 << 30, (int) sampleRate / 2, { { 0, note } });
        check (peak > 0.01f, name.paddedRight (' ', 12) + "peak " + juce::String (peak, 3));
    }

    const auto unmapped = render (*plugin, scratch, 1 << 30, (int) sampleRate / 4, { { 0, 100 } });
    // let earlier tails die out first
    render (*plugin, scratch, 1 << 30, (int) sampleRate * 12, {});
    const auto silence = render (*plugin, scratch, 1 << 30, (int) sampleRate / 4, { { 0, 100 } });
    check (silence < 1.0e-4f, "unmapped note is silent (" + juce::String (silence, 6) + ", before tails: " + juce::String (unmapped, 3) + ")");

    const auto softPeak = render (*plugin, scratch, 1 << 30, (int) sampleRate / 2, { { 0, 38 } }, 20);
    const auto hardPeak = render (*plugin, scratch, 1 << 30, (int) sampleRate / 2, { { 0, 38 } }, 127);
    check (hardPeak > softPeak * 1.5f, "velocity layers: soft " + juce::String (softPeak, 3) + " < hard " + juce::String (hardPeak, 3));

    // Two bars of rock beat at 120 bpm, then a tom fill and a crash.
    const auto eighth = (int) (sampleRate * 0.25);
    std::vector<std::pair<int, int>> beat;
    for (int bar = 0; bar < 2; ++bar)
        for (int i = 0; i < 8; ++i)
        {
            const auto t = (bar * 8 + i) * eighth;
            beat.push_back ({ t, (bar == 1 && i == 7) ? 46 : 42 });
            if (i == 0 || i == 3 || i == 4)
                beat.push_back ({ t, 36 });
            if (i == 2 || i == 6)
                beat.push_back ({ t, 38 });
        }
    for (int i = 0; i < 8; ++i)
        beat.push_back ({ 16 * eighth + i * eighth / 2, i < 3 ? 48 : i < 6 ? 45 : 41 });
    beat.push_back ({ 20 * eighth, 49 });
    beat.push_back ({ 20 * eighth, 36 });

    const auto beatLength = 24 * eighth;
    juce::AudioBuffer<float> beatAudio (2, beatLength);
    beatAudio.clear();
    render (*plugin, beatAudio, 0, beatLength, beat);

    {
        const auto wav = outDir.getChildFile ("beat.wav");
        wav.deleteFile();
        juce::WavAudioFormat format;
        std::unique_ptr<juce::AudioFormatWriter> writer (format.createWriterFor (
            new juce::FileOutputStream (wav), sampleRate, 2, 24, {}, 0));
        writer->writeFromAudioSampleBuffer (beatAudio, 0, beatLength);
        std::cout << "  wrote " << wav.getFullPathName() << " (peak " << beatAudio.getMagnitude (0, beatLength) << ")\n";
    }

    // State round trip: change a parameter, save, reset, restore.
    auto* param = plugin->getParameters()[0];
    param->setValueNotifyingHost (0.25f);
    juce::MemoryBlock state;
    plugin->getStateInformation (state);
    param->setValueNotifyingHost (0.9f);
    plugin->setStateInformation (state.getData(), (int) state.getSize());
    check (std::abs (param->getValue() - 0.25f) < 0.01f,
           "state round trip (" + param->getName (40) + " = " + juce::String (param->getValue(), 3) + ")");
    std::cout << "  parameters: " << plugin->getParameters().size() << ", state " << (int) state.getSize() << " bytes\n";
    param->setValueNotifyingHost (param->getDefaultValue());

    // The hosted editor is only a wrapper around the plugin's own window, so
    // screenshots are taken by TabifyDrumsSnapshot instead.
    std::unique_ptr<juce::AudioProcessorEditor> editor (plugin->createEditorIfNeeded());
    check (editor != nullptr, "editor created " + juce::String (editor ? editor->getWidth() : 0) + "x" + juce::String (editor ? editor->getHeight() : 0));
    if (editor != nullptr)
    {
        editor->setSize (1500, 960);
        juce::MessageManager::getInstance()->runDispatchLoopUntil (50);
        check (editor->getWidth() == 1500, "editor resizes to " + juce::String (editor->getWidth()) + "x" + juce::String (editor->getHeight()));

        plugin->editorBeingDeleted (editor.get());
        editor.reset();
    }

    plugin->releaseResources();
    plugin.reset();

    std::cout << (failures == 0 ? "ALL PASSED" : juce::String (failures) + " FAILURE(S)") << std::endl;
    return failures == 0 ? 0 : 1;
}
