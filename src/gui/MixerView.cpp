#include "MixerView.h"
#include "DrumLookAndFeel.h"
#include "KitDefinition.h"

ChannelStrip::ChannelStrip (juce::AudioProcessorValueTreeState& apvts, int piece)
    : title (piece < 0 ? "Master" : kit::pieces[(size_t) piece].name),
      isMaster (piece < 0)
{
    fader.setSliderStyle (juce::Slider::LinearVertical);
    fader.setTextBoxStyle (juce::Slider::TextBoxBelow, false, 60, 18);
    fader.setTextValueSuffix (" dB");
    fader.setDoubleClickReturnValue (true, 0.0);
    addAndMakeVisible (fader);
    faderAttachment = std::make_unique<SliderAttachment> (apvts, isMaster ? std::string ("master") : kit::paramId ("vol", piece), fader);

    if (isMaster)
        return;

    for (auto* knob : { &pan, &comp })
    {
        knob->setSliderStyle (juce::Slider::RotaryVerticalDrag);
        knob->setTextBoxStyle (juce::Slider::NoTextBox, false, 0, 0);
        knob->setDoubleClickReturnValue (true, 0.0);
        addAndMakeVisible (*knob);
    }
    pan.setTooltip ("Pan");
    comp.setTooltip ("Compressor amount");
    panAttachment = std::make_unique<SliderAttachment> (apvts, kit::paramId ("pan", piece), pan);
    compAttachment = std::make_unique<SliderAttachment> (apvts, kit::paramId ("comp", piece), comp);

    mute.setClickingTogglesState (true);
    solo.setClickingTogglesState (true);
    mute.setColour (juce::TextButton::buttonOnColourId, juce::Colour (0xffe04848));
    solo.setColour (juce::TextButton::buttonOnColourId, juce::Colour (0xffe8c547));
    addAndMakeVisible (mute);
    addAndMakeVisible (solo);
    muteAttachment = std::make_unique<ButtonAttachment> (apvts, kit::paramId ("mute", piece), mute);
    soloAttachment = std::make_unique<ButtonAttachment> (apvts, kit::paramId ("solo", piece), solo);
}

void ChannelStrip::setLevel (float peak)
{
    level = juce::jmax (peak, level * 0.85f);

    if (peak >= peakHold)
    {
        peakHold = peak;
        holdFrames = 45;
    }
    else if (--holdFrames < 0)
    {
        peakHold *= 0.92f;
    }

    repaint (meterArea.getSmallestIntegerContainer().expanded (2));
}

void ChannelStrip::paint (juce::Graphics& g)
{
    auto bounds = getLocalBounds().toFloat().reduced (2.0f);
    g.setColour (isMaster ? palette::panelLight : palette::panel);
    g.fillRoundedRectangle (bounds, 6.0f);
    g.setColour (palette::outline);
    g.drawRoundedRectangle (bounds, 6.0f, 1.0f);

    g.setColour (isMaster ? palette::accent : palette::text);
    g.setFont (juce::FontOptions (13.0f, juce::Font::bold));
    g.drawText (title.toUpperCase(), bounds.removeFromTop (28.0f), juce::Justification::centred);

    if (! isMaster)
    {
        g.setColour (palette::textDim);
        g.setFont (juce::FontOptions (10.0f));
        g.drawText ("COMP", comp.getBounds().translated (0, comp.getHeight() - 2).withHeight (12), juce::Justification::centred);
        g.drawText ("PAN", pan.getBounds().translated (0, pan.getHeight() - 2).withHeight (12), juce::Justification::centred);
    }

    // Meter, scaled in dB from -60 to +6
    auto toY = [this] (float gain)
    {
        const auto db = juce::jlimit (-60.0f, 6.0f, juce::Decibels::gainToDecibels (gain, -60.0f));
        return meterArea.getBottom() - meterArea.getHeight() * (db + 60.0f) / 66.0f;
    };

    g.setColour (juce::Colours::black.withAlpha (0.7f));
    g.fillRoundedRectangle (meterArea, 2.0f);

    if (level > 0.0f)
    {
        const auto top = toY (level);
        g.setGradientFill (juce::ColourGradient (palette::meterRed, 0.0f, toY (1.0f),
                                                 palette::meterGreen, 0.0f, toY (0.25f), false));
        g.fillRect (meterArea.withTop (top));
    }

    if (peakHold > 0.001f)
    {
        g.setColour (peakHold >= 1.0f ? palette::meterRed : palette::text);
        g.fillRect (meterArea.getX(), toY (peakHold) - 1.0f, meterArea.getWidth(), 2.0f);
    }

    g.setColour (palette::textDim.withAlpha (0.6f));
    g.drawHorizontalLine ((int) toY (1.0f), meterArea.getX() - 3.0f, meterArea.getX());
}

void ChannelStrip::resized()
{
    auto area = getLocalBounds().reduced (6);
    area.removeFromTop (26);

    if (! isMaster)
    {
        comp.setBounds (area.removeFromTop (48).withSizeKeepingCentre (44, 44));
        area.removeFromTop (12);
        pan.setBounds (area.removeFromTop (48).withSizeKeepingCentre (44, 44));
        area.removeFromTop (14);

        auto buttons = area.removeFromTop (24).reduced (4, 0);
        mute.setBounds (buttons.removeFromLeft (buttons.getWidth() / 2).reduced (2, 0));
        solo.setBounds (buttons.reduced (2, 0));
        area.removeFromTop (8);
    }

    auto meter = area.removeFromRight (14);
    meterArea = meter.withTrimmedBottom (26).withTrimmedTop (7).toFloat().reduced (2.0f, 0.0f);
    fader.setBounds (area);
}

MixerView::MixerView (juce::AudioProcessorValueTreeState& apvts)
{
    for (int p = 0; p < kit::numPieces; ++p)
        strips.push_back (std::make_unique<ChannelStrip> (apvts, p));
    strips.push_back (std::make_unique<ChannelStrip> (apvts, -1));

    for (auto& s : strips)
        addAndMakeVisible (*s);
}

void MixerView::setLevels (const std::vector<float>& peaks)
{
    for (size_t i = 0; i < strips.size() && i < peaks.size(); ++i)
        strips[i]->setLevel (peaks[i]);
}

void MixerView::paint (juce::Graphics& g)
{
    g.fillAll (palette::background);
}

void MixerView::resized()
{
    auto area = getLocalBounds().reduced (10);
    const auto width = area.getWidth() / (int) strips.size();

    for (size_t i = 0; i < strips.size(); ++i)
    {
        if (i + 1 == strips.size())
            area.removeFromLeft (4);
        strips[i]->setBounds (area.removeFromLeft (width));
    }
}
