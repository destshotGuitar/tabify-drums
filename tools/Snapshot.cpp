// Renders the plugin editor offscreen to PNG files: the idle kit, the kit
// mid-animation right after a few hits, and the mixer page with meters moving.
// It links the plugin code directly, so no host or audio device is needed.
//
// usage: TabifyDrumsSnapshot <output-dir>

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_gui_basics/juce_gui_basics.h>

namespace
{
juce::Button* findButton (juce::Component& root, const juce::String& text)
{
    if (auto* b = dynamic_cast<juce::Button*> (&root); b != nullptr && b->getButtonText() == text)
        return b;
    for (auto* child : root.getChildren())
        if (auto* found = findButton (*child, text))
            return found;
    return nullptr;
}

class Sequence final : private juce::Timer
{
public:
    Sequence (juce::AudioProcessor& p, juce::AudioProcessorEditor& e, juce::File dir)
        : processor (p), editor (e), outDir (std::move (dir))
    {
        startTimer (20);
    }

private:
    void play (std::initializer_list<int> notes, int velocity = 115)
    {
        juce::AudioBuffer<float> buffer (2, 256);
        juce::MidiBuffer midi;
        for (const auto n : notes)
            midi.addEvent (juce::MidiMessage::noteOn (10, n, (juce::uint8) velocity), 0);
        processor.processBlock (buffer, midi);
    }

    void save (const juce::String& name, float scale = 1.0f)
    {
        const auto image = editor.createComponentSnapshot (editor.getLocalBounds(), true, scale);
        const auto file = outDir.getChildFile (name);
        file.deleteFile();
        juce::FileOutputStream out (file);
        juce::PNGImageFormat().writeImageToStream (image, out);
        std::cout << "wrote " << file.getFullPathName() << std::endl;
    }

    void timerCallback() override
    {
        ++tick; // 20 ms per tick

        if (tick == 10)
            save ("kit-idle.png");
        else if (tick == 12)
            play ({ 38, 46, 49, 36, 45 }, 120);
        else if (tick == 15)
            save ("kit-hit.png");
        else if (tick == 30)
            play ({ 51, 48, 42 }, 90);
        else if (tick == 33)
            save ("kit-hit2.png");
        else if (tick == 40)
        {
            if (auto* b = findButton (editor, "MIXER"))
                b->triggerClick();
        }
        else if (tick > 40 && tick < 70)
        {
            if (tick % 3 == 0)
                play ({ 36, 42, tick % 6 == 0 ? 38 : 51 });
        }
        else if (tick == 70)
            save ("mixer.png");
        else if (tick == 72)
        {
            stopTimer();
            juce::MessageManager::getInstance()->stopDispatchLoop();
        }
    }

    juce::AudioProcessor& processor;
    juce::AudioProcessorEditor& editor;
    juce::File outDir;
    int tick = 0;
};
} // namespace

int main (int argc, char** argv)
{
    if (argc < 2)
    {
        std::cerr << "usage: TabifyDrumsSnapshot <output-dir>\n";
        return 2;
    }

    juce::ScopedJuceInitialiser_GUI gui;
    const auto outDir = juce::File::getCurrentWorkingDirectory().getChildFile (argv[1]);
    outDir.createDirectory();

    std::unique_ptr<juce::AudioProcessor> processor (createPluginFilter());
    processor->setPlayConfigDetails (0, 2, 48000.0, 256);
    processor->prepareToPlay (48000.0, 256);

    std::unique_ptr<juce::AudioProcessorEditor> editor (processor->createEditorAndMakeActive());
    {
        Sequence sequence (*processor, *editor, outDir);
        juce::MessageManager::getInstance()->runDispatchLoop();
    }

    processor->editorBeingDeleted (editor.get());
    editor.reset();
    processor.reset();
    return 0;
}
