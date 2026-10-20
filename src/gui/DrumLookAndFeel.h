#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

namespace palette
{
inline const juce::Colour background { 0xff15171b };
inline const juce::Colour panel { 0xff1f2228 };
inline const juce::Colour panelLight { 0xff2a2e36 };
inline const juce::Colour outline { 0xff3a3f49 };
inline const juce::Colour text { 0xffd9dde4 };
inline const juce::Colour textDim { 0xff8b919c };
inline const juce::Colour accent { 0xffff8a2a };
inline const juce::Colour meterGreen { 0xff4cd07d };
inline const juce::Colour meterRed { 0xffff4a4a };
} // namespace palette

class DrumLookAndFeel final : public juce::LookAndFeel_V4
{
public:
    DrumLookAndFeel();

    void drawRotarySlider (juce::Graphics&, int x, int y, int width, int height,
                           float sliderPos, float startAngle, float endAngle, juce::Slider&) override;

    void drawLinearSlider (juce::Graphics&, int x, int y, int width, int height,
                           float sliderPos, float minSliderPos, float maxSliderPos,
                           juce::Slider::SliderStyle, juce::Slider&) override;

    void drawButtonBackground (juce::Graphics&, juce::Button&, const juce::Colour& backgroundColour,
                               bool highlighted, bool down) override;

    juce::Font getTextButtonFont (juce::TextButton&, int buttonHeight) override;
};
