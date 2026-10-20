#include "PluginProcessor.h"
#include "PluginEditor.h"

TabifyDrumsProcessor::TabifyDrumsProcessor()
    : AudioProcessor (BusesProperties().withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "TabifyDrums", createLayout())
{
    for (int p = 0; p < kit::numPieces; ++p)
    {
        auto& pp = pieceParams[(size_t) p];
        pp.volume = apvts.getRawParameterValue (kit::paramId ("vol", p));
        pp.pan = apvts.getRawParameterValue (kit::paramId ("pan", p));
        pp.comp = apvts.getRawParameterValue (kit::paramId ("comp", p));
        pp.mute = apvts.getRawParameterValue (kit::paramId ("mute", p));
        pp.solo = apvts.getRawParameterValue (kit::paramId ("solo", p));
    }
    masterVolume = apvts.getRawParameterValue ("master");

    for (auto& p : peaks)
        p = 0.0f;

    resetAllMappings();
}

juce::AudioProcessorValueTreeState::ParameterLayout TabifyDrumsProcessor::createLayout()
{
    juce::AudioProcessorValueTreeState::ParameterLayout layout;

    const juce::NormalisableRange<float> dbRange (-60.0f, 6.0f, 0.1f, 2.0f);
    const auto dbAttributes = juce::AudioParameterFloatAttributes().withLabel ("dB");

    for (int p = 0; p < kit::numPieces; ++p)
    {
        const juce::String name (kit::pieces[(size_t) p].name);
        auto group = std::make_unique<juce::AudioProcessorParameterGroup> (kit::pieces[(size_t) p].id, name, " | ");

        group->addChild (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID { kit::paramId ("vol", p), 1 }, name + " Volume", dbRange, 0.0f, dbAttributes));
        group->addChild (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID { kit::paramId ("pan", p), 1 }, name + " Pan",
            juce::NormalisableRange<float> (-1.0f, 1.0f, 0.01f), 0.0f));
        group->addChild (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID { kit::paramId ("comp", p), 1 }, name + " Compressor",
            juce::NormalisableRange<float> (0.0f, 1.0f, 0.01f), 0.0f));
        group->addChild (std::make_unique<juce::AudioParameterBool> (
            juce::ParameterID { kit::paramId ("mute", p), 1 }, name + " Mute", false));
        group->addChild (std::make_unique<juce::AudioParameterBool> (
            juce::ParameterID { kit::paramId ("solo", p), 1 }, name + " Solo", false));

        layout.add (std::move (group));
    }

    layout.add (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "master", 1 }, "Master Volume", dbRange, 0.0f, dbAttributes));

    return layout;
}

void TabifyDrumsProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    engine.prepare (sampleRate, samplesPerBlock);
}

bool TabifyDrumsProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    return layouts.getMainOutputChannelSet() == juce::AudioChannelSet::stereo();
}

void TabifyDrumsProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midi)
{
    juce::ScopedNoDenormals noDenormals;

    const auto numSamples = buffer.getNumSamples();
    buffer.clear();
    engine.startBlock (numSamples);

    {
        const auto scope = editorFifo.read (editorFifo.getNumReady());
        for (int i = 0; i < scope.blockSize1; ++i)
            trigger (editorHits[(size_t) (scope.startIndex1 + i)].articulation,
                     editorHits[(size_t) (scope.startIndex1 + i)].velocity, -1, false);
        for (int i = 0; i < scope.blockSize2; ++i)
            trigger (editorHits[(size_t) (scope.startIndex2 + i)].articulation,
                     editorHits[(size_t) (scope.startIndex2 + i)].velocity, -1, false);
    }

    int position = 0;
    for (const auto metadata : midi)
    {
        const auto time = juce::jlimit (0, numSamples, metadata.samplePosition);
        engine.render (position, time - position);
        position = time;

        const auto message = metadata.getMessage();
        if (message.isNoteOn())
            handleNoteOn (message.getNoteNumber(), message.getVelocity());
        else if (message.isAllNotesOff() || message.isAllSoundOff())
            engine.stopAll();
    }
    engine.render (position, numSamples - position);

    std::array<DrumEngine::ChannelSettings, kit::numPieces> channels;
    bool anySolo = false;
    for (const auto& pp : pieceParams)
        anySolo = anySolo || pp.solo->load() > 0.5f;

    for (size_t p = 0; p < channels.size(); ++p)
    {
        const auto& pp = pieceParams[p];
        auto& ch = channels[p];
        ch.gain = juce::Decibels::decibelsToGain (pp.volume->load(), -59.9f);
        ch.pan = pp.pan->load();
        ch.comp = pp.comp->load();
        ch.audible = pp.mute->load() < 0.5f && (! anySolo || pp.solo->load() > 0.5f);
    }

    DrumEngine::Peaks blockPeaks;
    engine.mix (buffer, channels, juce::Decibels::decibelsToGain (masterVolume->load(), -59.9f), blockPeaks);

    for (size_t i = 0; i < peaks.size(); ++i)
        if (blockPeaks[i] > peaks[i].load())
            peaks[i] = blockPeaks[i];
}

void TabifyDrumsProcessor::handleNoteOn (int note, int velocity)
{
    if (! juce::isPositiveAndBelow (note, 128))
        return;

    const auto learning = learnArticulation.exchange (-1);
    if (learning >= 0)
    {
        noteMap[(size_t) note] = learning;
        trigger (learning, velocity, note, true);
        return;
    }

    const auto articulation = noteMap[(size_t) note].load();
    if (articulation >= 0)
        trigger (articulation, velocity, note, false);
}

void TabifyDrumsProcessor::trigger (int articulation, int velocity, int note, bool learned)
{
    engine.trigger (articulation, (float) velocity / 127.0f);

    const auto scope = hitFifo.write (1);
    if (scope.blockSize1 > 0)
        hitEvents[(size_t) scope.startIndex1] = { articulation, velocity, note, learned };
}

bool TabifyDrumsProcessor::popHitEvent (HitEvent& event)
{
    const auto scope = hitFifo.read (1);
    if (scope.blockSize1 == 0)
        return false;

    event = hitEvents[(size_t) scope.startIndex1];
    return true;
}

void TabifyDrumsProcessor::playFromEditor (int articulation, int velocity)
{
    const auto scope = editorFifo.write (1);
    if (scope.blockSize1 > 0)
        editorHits[(size_t) scope.startIndex1] = { articulation, velocity };
}

float TabifyDrumsProcessor::popPeak (int channel)
{
    return peaks[(size_t) channel].exchange (0.0f);
}

juce::Array<int> TabifyDrumsProcessor::getNotesFor (int articulation) const
{
    juce::Array<int> notes;
    for (int n = 0; n < 128; ++n)
        if (noteMap[(size_t) n].load() == articulation)
            notes.add (n);
    return notes;
}

void TabifyDrumsProcessor::resetMappingFor (int articulation)
{
    for (auto& entry : noteMap)
        if (entry.load() == articulation)
            entry = -1;

    for (const auto note : kit::articulations[(size_t) articulation].defaultNotes)
        if (note >= 0)
            noteMap[(size_t) note] = articulation;
}

void TabifyDrumsProcessor::resetAllMappings()
{
    for (auto& entry : noteMap)
        entry = -1;

    for (int a = 0; a < kit::numArticulations; ++a)
        for (const auto note : kit::articulations[(size_t) a].defaultNotes)
            if (note >= 0)
                noteMap[(size_t) note] = a;
}

juce::String TabifyDrumsProcessor::serialiseNoteMap() const
{
    // Stored by articulation id so the mapping survives reordering the kit table.
    juce::StringArray entries;
    for (int n = 0; n < 128; ++n)
        if (const auto a = noteMap[(size_t) n].load(); a >= 0)
            entries.add (juce::String (n) + ":" + kit::articulations[(size_t) a].id);
    return entries.joinIntoString (",");
}

void TabifyDrumsProcessor::restoreNoteMap (const juce::String& text)
{
    for (auto& entry : noteMap)
        entry = -1;

    for (const auto& item : juce::StringArray::fromTokens (text, ",", {}))
    {
        const auto note = item.upToFirstOccurrenceOf (":", false, false).getIntValue();
        const auto id = item.fromFirstOccurrenceOf (":", false, false);
        if (! juce::isPositiveAndBelow (note, 128))
            continue;

        for (int a = 0; a < kit::numArticulations; ++a)
            if (id == kit::articulations[(size_t) a].id)
                noteMap[(size_t) note] = a;
    }
}

void TabifyDrumsProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    auto state = apvts.copyState();
    state.setProperty ("noteMap", serialiseNoteMap(), nullptr);
    if (const auto xml = state.createXml())
        copyXmlToBinary (*xml, destData);
}

void TabifyDrumsProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    const auto xml = getXmlFromBinary (data, sizeInBytes);
    if (xml == nullptr || ! xml->hasTagName (apvts.state.getType()))
        return;

    const auto state = juce::ValueTree::fromXml (*xml);
    if (state.hasProperty ("noteMap"))
        restoreNoteMap (state["noteMap"].toString());
    apvts.replaceState (state);
}

juce::AudioProcessorEditor* TabifyDrumsProcessor::createEditor()
{
    return new TabifyDrumsEditor (*this);
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new TabifyDrumsProcessor();
}
