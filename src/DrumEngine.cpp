#include "DrumEngine.h"

#include <KitData.h>
#include <map>

// The kit samples are normalised close to full scale; this leaves headroom
// when the whole kit plays at once.
static constexpr float kitTrim = 0.5f;

namespace
{
using ResourceMap = std::map<std::string, std::pair<const char*, int>>;

// Embedded samples by file name without extension.
const ResourceMap& embeddedSamples()
{
    static const ResourceMap resources = []
    {
        ResourceMap map;
        for (int i = 0; i < KitData::namedResourceListSize; ++i)
        {
            const auto* name = KitData::namedResourceList[i];
            int size = 0;
            if (const auto* data = KitData::getNamedResource (name, size))
            {
                const juce::String file (KitData::getNamedResourceOriginalFilename (name));
                map[file.upToLastOccurrenceOf (".", false, false).toStdString()] = { data, size };
            }
        }
        return map;
    }();
    return resources;
}

juce::AudioBuffer<float> decodeMono (juce::AudioFormatManager& formats, const std::pair<const char*, int>& data,
                                     double targetRate)
{
    auto stream = std::make_unique<juce::MemoryInputStream> (data.first, (size_t) data.second, false);
    std::unique_ptr<juce::AudioFormatReader> reader (formats.createReaderFor (std::move (stream)));
    if (reader == nullptr)
        return {};

    const auto length = (int) reader->lengthInSamples;
    juce::AudioBuffer<float> raw ((int) reader->numChannels, length);
    reader->read (&raw, 0, length, 0, true, true);

    // Mix down to mono: pan is applied per piece in the mixer.
    juce::AudioBuffer<float> mono (1, length);
    mono.clear();
    for (int ch = 0; ch < raw.getNumChannels(); ++ch)
        mono.addFrom (0, 0, raw, ch, 0, length, 1.0f / (float) raw.getNumChannels());

    const auto ratio = reader->sampleRate / targetRate;
    if (std::abs (ratio - 1.0) < 1.0e-6)
        return mono;

    const auto outLength = (int) std::ceil (length / ratio);
    juce::AudioBuffer<float> resampled (1, outLength);
    resampled.clear();
    juce::LagrangeInterpolator interpolator;
    interpolator.process (ratio, mono.getReadPointer (0), resampled.getWritePointer (0), outLength, length, 0);
    return resampled;
}
} // namespace

std::unique_ptr<DrumEngine::KitSamples> DrumEngine::loadKit (int kitIndex, double rate)
{
    kitIndex = juce::jlimit (0, (int) kit::kits.size() - 1, kitIndex);
    const std::string prefix (kit::kits[(size_t) kitIndex].filePrefix);
    const auto& resources = embeddedSamples();

    juce::AudioFormatManager formats;
    formats.registerBasicFormats();

    auto result = std::make_unique<KitSamples>();
    result->kitIndex = kitIndex;
    result->sampleRate = rate;

    for (size_t a = 0; a < kit::articulations.size(); ++a)
    {
        const auto files = kit::layerFiles (kit::articulations[a]);

        // Use the kit's own samples if it has them, else the GM Rock ones.
        const auto own = resources.count (prefix + files.front()) > 0;

        for (const auto& file : files)
        {
            const auto found = resources.find (own ? prefix + file : file);
            if (found == resources.end())
            {
                jassertfalse; // the sample is missing from the binary data
                continue;
            }

            auto buffer = decodeMono (formats, found->second, rate);
            if (buffer.getNumSamples() > 0)
                result->layers[a].push_back (std::move (buffer));
        }
    }

    return result;
}

void DrumEngine::prepare (double newSampleRate, int maxBlockSize)
{
    sampleRate = newSampleRate;
    chokeFadeSamples = juce::jmax (1, (int) (sampleRate * 0.03));

    stopAll();
    pieceBuffers.setSize (kit::numPieces, maxBlockSize);

    for (auto& c : compressors)
        c.prepare (sampleRate);

    for (int p = 0; p < kit::numPieces; ++p)
    {
        gainLeft[(size_t) p].reset (sampleRate, 0.02);
        gainRight[(size_t) p].reset (sampleRate, 0.02);
    }
    master.reset (sampleRate, 0.02);
    gainsSet = false;
}

void DrumEngine::setKit (std::unique_ptr<KitSamples> samples)
{
    const juce::SpinLock::ScopedLockType lock (kitLock);
    requested = samples.get();
    kitStore.push_back (std::move (samples));
    collectGarbageLocked();
}

void DrumEngine::setKitNow (std::unique_ptr<KitSamples> samples)
{
    const juce::SpinLock::ScopedLockType lock (kitLock);
    stopAll();
    requested = active = samples.get();
    kitStore.push_back (std::move (samples));
    collectGarbageLocked();
}

void DrumEngine::collectGarbage()
{
    const juce::SpinLock::ScopedLockType lock (kitLock);
    collectGarbageLocked();
}

void DrumEngine::collectGarbageLocked()
{
    kitStore.erase (std::remove_if (kitStore.begin(), kitStore.end(),
                                    [this] (const auto& k) { return k.get() != active && k.get() != requested; }),
                    kitStore.end());
}

void DrumEngine::startBlock (int numSamples)
{
    // Some hosts exceed the block size announced in prepareToPlay.
    if (numSamples > pieceBuffers.getNumSamples())
        pieceBuffers.setSize (kit::numPieces, numSamples, false, false, true);

    pieceBuffers.clear (0, numSamples);

    {
        const juce::SpinLock::ScopedTryLockType lock (kitLock);
        if (lock.isLocked() && requested != active)
        {
            // Voices point into the old kit's buffers, so they must stop.
            stopAll();
            active = requested;
        }
    }
}

void DrumEngine::render (int startSample, int numSamples)
{
    if (numSamples <= 0)
        return;

    for (auto& v : voices)
    {
        if (v.sample == nullptr)
            continue;

        const auto* src = v.sample->getReadPointer (0);
        const auto length = v.sample->getNumSamples();
        auto* out = pieceBuffers.getWritePointer (v.piece, startSample);

        for (int i = 0; i < numSamples; ++i)
        {
            if (v.position >= length)
            {
                v.sample = nullptr;
                break;
            }

            auto g = v.gain;
            if (v.fadeLeft > 0)
            {
                g *= (float) v.fadeLeft / (float) v.fadeLength;
                if (--v.fadeLeft == 0)
                {
                    v.sample = nullptr;
                    break;
                }
            }

            out[i] += src[v.position++] * g;
        }
    }
}

DrumEngine::Voice& DrumEngine::allocateVoice()
{
    Voice* oldest = &voices[0];

    for (auto& v : voices)
    {
        if (v.sample == nullptr)
            return v;
        if (v.age < oldest->age)
            oldest = &v;
    }

    return *oldest;
}

void DrumEngine::trigger (int articulation, float velocity)
{
    if (! juce::isPositiveAndBelow (articulation, (int) kit::numArticulations))
        return;

    if (active == nullptr)
        return;

    const auto& info = kit::articulations[(size_t) articulation];
    const auto& available = active->layers[(size_t) articulation];
    if (available.empty())
        return;

    velocity = juce::jlimit (0.0f, 1.0f, velocity);

    if (info.chokeGroup != kit::chokeNone)
        for (auto& v : voices)
            if (v.sample != nullptr && v.chokeGroup == info.chokeGroup && v.fadeLeft == 0)
            {
                v.fadeLeft = chokeFadeSamples;
                v.fadeLength = chokeFadeSamples;
            }

    float gain = 1.0f;
    size_t index = 0;

    if (available.size() == kit::layerLowerBounds.size())
    {
        while (index + 1 < available.size() && velocity >= kit::layerLowerBounds[index + 1])
            ++index;

        // Within a layer, scale smoothly so neighbouring velocities still differ.
        const auto lo = kit::layerLowerBounds[index];
        const auto hi = index + 1 < available.size() ? kit::layerLowerBounds[index + 1] : 1.0f;
        gain = juce::jmap ((velocity - lo) / juce::jmax (0.01f, hi - lo), 0.7f, 1.0f);
    }
    else
    {
        index = juce::jmin (available.size() - 1, (size_t) (velocity * (float) available.size()));
        gain = juce::jmap (velocity, 0.15f, 1.0f);
    }

    auto& v = allocateVoice();
    v.sample = &available[index];
    v.position = 0;
    v.gain = gain;
    v.piece = info.piece;
    v.chokeGroup = info.chokeGroup;
    v.fadeLeft = 0;
    v.fadeLength = 0;
    v.age = ++voiceCounter;
}

void DrumEngine::stopAll()
{
    for (auto& v : voices)
        v.sample = nullptr;
}

void DrumEngine::mix (juce::AudioBuffer<float>& output,
                      const std::array<ChannelSettings, kit::numPieces>& channels,
                      float masterGain,
                      Peaks& peaks)
{
    const auto numSamples = output.getNumSamples();
    auto* left = output.getWritePointer (0);
    auto* right = output.getNumChannels() > 1 ? output.getWritePointer (1) : nullptr;

    peaks.fill (0.0f);

    for (int p = 0; p < kit::numPieces; ++p)
    {
        const auto& ch = channels[(size_t) p];
        auto* data = pieceBuffers.getWritePointer (p);

        compressors[(size_t) p].process (data, numSamples, ch.comp);

        // Constant-power pan, normalised so the centre position is unity gain.
        const auto angle = (ch.pan + 1.0f) * juce::MathConstants<float>::pi * 0.25f;
        const auto level = ch.audible ? ch.gain * kitTrim * juce::MathConstants<float>::sqrt2 : 0.0f;
        auto& gl = gainLeft[(size_t) p];
        auto& gr = gainRight[(size_t) p];
        if (gainsSet)
        {
            gl.setTargetValue (level * std::cos (angle));
            gr.setTargetValue (level * std::sin (angle));
        }
        else
        {
            gl.setCurrentAndTargetValue (level * std::cos (angle));
            gr.setCurrentAndTargetValue (level * std::sin (angle));
        }

        float peak = 0.0f;
        for (int i = 0; i < numSamples; ++i)
        {
            const auto l = data[i] * gl.getNextValue();
            const auto r = data[i] * gr.getNextValue();
            left[i] += l;
            if (right != nullptr)
                right[i] += r;
            peak = juce::jmax (peak, std::abs (l), std::abs (r));
        }
        peaks[(size_t) p] = peak;
    }

    if (gainsSet)
        master.setTargetValue (masterGain);
    else
        master.setCurrentAndTargetValue (masterGain);
    gainsSet = true;
    float masterPeak = 0.0f;
    for (int i = 0; i < numSamples; ++i)
    {
        const auto g = master.getNextValue();
        left[i] *= g;
        masterPeak = juce::jmax (masterPeak, std::abs (left[i]));
        if (right != nullptr)
        {
            right[i] *= g;
            masterPeak = juce::jmax (masterPeak, std::abs (right[i]));
        }
    }
    peaks[kit::numPieces] = masterPeak;
}

void DrumEngine::Compressor::prepare (double sr)
{
    envelope = 0.0f;
    attackCoef = (float) std::exp (-1.0 / (0.003 * sr));
    releaseCoef = (float) std::exp (-1.0 / (0.12 * sr));
}

void DrumEngine::Compressor::process (float* samples, int numSamples, float amount)
{
    if (amount <= 0.001f)
    {
        envelope = 0.0f;
        return;
    }

    // One knob: deeper threshold and higher ratio together, with automatic
    // makeup gain so turning it up does not just make the channel quieter.
    const auto threshold = -30.0f * amount;
    const auto slope = 1.0f - 1.0f / (1.0f + 5.0f * amount);
    const auto makeup = juce::Decibels::decibelsToGain (-threshold * slope * 0.5f);

    for (int i = 0; i < numSamples; ++i)
    {
        const auto x = std::abs (samples[i]);
        const auto coef = x > envelope ? attackCoef : releaseCoef;
        envelope = coef * envelope + (1.0f - coef) * x;

        const auto over = juce::Decibels::gainToDecibels (envelope, -120.0f) - threshold;
        const auto reduction = over > 0.0f ? over * slope : 0.0f;
        samples[i] *= juce::Decibels::decibelsToGain (-reduction) * makeup;
    }
}
