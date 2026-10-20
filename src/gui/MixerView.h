#pragma once

#include <juce_audio_processors/juce_audio_processors.h>

class ChannelStrip final : public juce::Component
{
public:
    // piece < 0 makes this the master strip (fader only).
    ChannelStrip (juce::AudioProcessorValueTreeState&, int piece);

    void setLevel (float peak);
    void paint (juce::Graphics&) override;
    void resized() override;

private:
    using SliderAttachment = juce::AudioProcessorValueTreeState::SliderAttachment;
    using ButtonAttachment = juce::AudioProcessorValueTreeState::ButtonAttachment;

    juce::String title;
    bool isMaster;
    juce::Slider fader, pan, comp;
    juce::TextButton mute { "M" }, solo { "S" };
    std::unique_ptr<SliderAttachment> faderAttachment, panAttachment, compAttachment;
    std::unique_ptr<ButtonAttachment> muteAttachment, soloAttachment;

    float level = 0.0f, peakHold = 0.0f;
    int holdFrames = 0;
    juce::Rectangle<float> meterArea;
};

class MixerView final : public juce::Component
{
public:
    explicit MixerView (juce::AudioProcessorValueTreeState&);

    // peaks: one per piece, then master
    void setLevels (const std::vector<float>& peaks);
    void paint (juce::Graphics&) override;
    void resized() override;

private:
    std::vector<std::unique_ptr<ChannelStrip>> strips;
};
