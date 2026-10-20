#include "DrumLookAndFeel.h"

DrumLookAndFeel::DrumLookAndFeel()
{
    setColour (juce::ResizableWindow::backgroundColourId, palette::background);
    setColour (juce::Label::textColourId, palette::text);
    setColour (juce::Slider::textBoxTextColourId, palette::text);
    setColour (juce::Slider::textBoxOutlineColourId, juce::Colours::transparentBlack);
    setColour (juce::Slider::textBoxBackgroundColourId, palette::background.withAlpha (0.6f));
    setColour (juce::Slider::rotarySliderFillColourId, palette::accent);
    setColour (juce::Slider::thumbColourId, palette::accent);
    setColour (juce::TextButton::buttonColourId, palette::panelLight);
    setColour (juce::TextButton::buttonOnColourId, palette::accent);
    setColour (juce::TextButton::textColourOffId, palette::text);
    setColour (juce::TextButton::textColourOnId, juce::Colours::black);
    setColour (juce::PopupMenu::backgroundColourId, palette::panel);
    setColour (juce::PopupMenu::textColourId, palette::text);
    setColour (juce::PopupMenu::headerTextColourId, palette::accent);
    setColour (juce::PopupMenu::highlightedBackgroundColourId, palette::accent.withAlpha (0.8f));
    setColour (juce::PopupMenu::highlightedTextColourId, juce::Colours::black);
}

void DrumLookAndFeel::drawRotarySlider (juce::Graphics& g, int x, int y, int width, int height,
                                        float sliderPos, float startAngle, float endAngle, juce::Slider& slider)
{
    const auto bounds = juce::Rectangle<int> (x, y, width, height).toFloat().reduced (3.0f);
    const auto radius = juce::jmin (bounds.getWidth(), bounds.getHeight()) * 0.5f;
    const auto centre = bounds.getCentre();
    const auto angle = startAngle + sliderPos * (endAngle - startAngle);
    const auto arcRadius = radius - 2.0f;

    // Bipolar controls such as pan light up from the centre.
    const auto bipolar = slider.getMinimum() < 0.0 && slider.getMaximum() > 0.0;
    const auto arcStart = bipolar ? startAngle + (float) slider.valueToProportionOfLength (0.0) * (endAngle - startAngle)
                                  : startAngle;

    juce::Path track;
    track.addCentredArc (centre.x, centre.y, arcRadius, arcRadius, 0.0f, startAngle, endAngle, true);
    g.setColour (palette::outline);
    g.strokePath (track, juce::PathStrokeType (3.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

    juce::Path value;
    value.addCentredArc (centre.x, centre.y, arcRadius, arcRadius, 0.0f, arcStart, angle, true);
    g.setColour (palette::accent);
    g.strokePath (value, juce::PathStrokeType (3.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

    const auto knobRadius = radius - 7.0f;
    g.setGradientFill (juce::ColourGradient (juce::Colour (0xff474d58), centre.x, centre.y - knobRadius,
                                             juce::Colour (0xff1c1f24), centre.x, centre.y + knobRadius, false));
    g.fillEllipse (juce::Rectangle<float> (knobRadius * 2.0f, knobRadius * 2.0f).withCentre (centre));
    g.setColour (juce::Colours::black.withAlpha (0.6f));
    g.drawEllipse (juce::Rectangle<float> (knobRadius * 2.0f, knobRadius * 2.0f).withCentre (centre), 1.0f);

    const auto tip = centre.getPointOnCircumference (knobRadius - 2.0f, angle);
    const auto base = centre.getPointOnCircumference (knobRadius * 0.35f, angle);
    g.setColour (palette::text);
    g.drawLine ({ base, tip }, 2.0f);
}

void DrumLookAndFeel::drawLinearSlider (juce::Graphics& g, int x, int y, int width, int height,
                                        float sliderPos, float minSliderPos, float maxSliderPos,
                                        juce::Slider::SliderStyle style, juce::Slider& slider)
{
    if (style != juce::Slider::LinearVertical && style != juce::Slider::LinearHorizontal)
    {
        LookAndFeel_V4::drawLinearSlider (g, x, y, width, height, sliderPos, minSliderPos, maxSliderPos, style, slider);
        return;
    }

    const auto bounds = juce::Rectangle<int> (x, y, width, height).toFloat();
    const auto vertical = style == juce::Slider::LinearVertical;

    auto track = vertical ? bounds.withSizeKeepingCentre (4.0f, bounds.getHeight())
                          : bounds.withSizeKeepingCentre (bounds.getWidth(), 4.0f);
    g.setColour (juce::Colours::black.withAlpha (0.7f));
    g.fillRoundedRectangle (track, 2.0f);

    auto filled = vertical ? track.withTop (sliderPos) : track.withRight (sliderPos);
    g.setColour (palette::accent.withAlpha (0.85f));
    g.fillRoundedRectangle (filled, 2.0f);

    // Fader cap
    const auto cap = vertical ? juce::Rectangle<float> (26.0f, 14.0f).withCentre ({ bounds.getCentreX(), sliderPos })
                              : juce::Rectangle<float> (12.0f, 20.0f).withCentre ({ sliderPos, bounds.getCentreY() });
    g.setGradientFill (juce::ColourGradient (juce::Colour (0xffd5d9df), cap.getX(), cap.getY(),
                                             juce::Colour (0xff70767f), cap.getX(), cap.getBottom(), false));
    g.fillRoundedRectangle (cap, 2.5f);
    g.setColour (juce::Colours::black.withAlpha (0.7f));
    g.drawRoundedRectangle (cap, 2.5f, 1.0f);
    if (vertical)
    {
        g.setColour (juce::Colours::black.withAlpha (0.8f));
        g.drawHorizontalLine ((int) cap.getCentreY(), cap.getX() + 3.0f, cap.getRight() - 3.0f);
    }
}

void DrumLookAndFeel::drawButtonBackground (juce::Graphics& g, juce::Button& button, const juce::Colour&,
                                            bool highlighted, bool down)
{
    const auto bounds = button.getLocalBounds().toFloat().reduced (0.5f);
    const auto on = button.getToggleState();

    auto base = on ? button.findColour (juce::TextButton::buttonOnColourId)
                   : button.findColour (juce::TextButton::buttonColourId);
    if (highlighted)
        base = base.brighter (0.15f);
    if (down)
        base = base.darker (0.2f);

    g.setColour (base);
    g.fillRoundedRectangle (bounds, 3.0f);
    g.setColour (on ? base.brighter (0.3f) : palette::outline);
    g.drawRoundedRectangle (bounds, 3.0f, 1.0f);
}

juce::Font DrumLookAndFeel::getTextButtonFont (juce::TextButton&, int buttonHeight)
{
    return juce::FontOptions (juce::jmin (14.0f, (float) buttonHeight * 0.55f), juce::Font::bold);
}
