#include "PluginEditor.h"

namespace
{
juce::String middleDot()
{
    return juce::CharPointer_UTF8 (" \xc2\xb7 ");
}
} // namespace

TabifyDrumsEditor::TabifyDrumsEditor (TabifyDrumsProcessor& p)
    : AudioProcessorEditor (p), processor (p), mixerView (p.apvts)
{
    setLookAndFeel (&lookAndFeel);
    surface.setBounds (0, 0, designWidth, designHeight);
    addAndMakeVisible (surface);

    for (auto* tab : { &kitTab, &mixerTab })
    {
        tab->setClickingTogglesState (true);
        tab->setRadioGroupId (1);
        surface.addAndMakeVisible (*tab);
    }
    kitTab.onClick = [this] { showPage (true); };
    mixerTab.onClick = [this] { showPage (false); };

    for (const auto& k : kit::kits)
        kitSelector.addItem (k.name, kitSelector.getNumItems() + 1);
    kitSelector.setTooltip ("Drum kit");
    surface.addAndMakeVisible (kitSelector);
    kitAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment> (p.apvts, "kit", kitSelector);

    masterKnob.setSliderStyle (juce::Slider::RotaryVerticalDrag);
    masterKnob.setTextBoxStyle (juce::Slider::TextBoxRight, false, 64, 18);
    masterKnob.setTextValueSuffix (" dB");
    masterKnob.setDoubleClickReturnValue (true, 0.0);
    surface.addAndMakeVisible (masterKnob);
    masterAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (p.apvts, "master", masterKnob);
    masterLabel.setText ("MASTER", juce::dontSendNotification);
    masterLabel.setFont (juce::FontOptions (12.0f, juce::Font::bold));
    masterLabel.setColour (juce::Label::textColourId, palette::textDim);
    masterLabel.setJustificationType (juce::Justification::centredRight);
    surface.addAndMakeVisible (masterLabel);

    surface.addAndMakeVisible (kitView);
    surface.addChildComponent (mixerView);

    kitView.onPlay = [this] (int articulation)
    {
        processor.playFromEditor (articulation, (int) velocitySlider.getValue());
    };
    kitView.onPieceMenu = [this] (int piece) { showPieceMenu (piece); };
    kitView.onHoverHelp = [this] (const juce::String& help)
    {
        helpLabel.setText (help.isNotEmpty() ? help : statusText, juce::dontSendNotification);
    };

    lastHitLabel.setFont (juce::FontOptions (15.0f, juce::Font::bold));
    lastHitLabel.setColour (juce::Label::textColourId, palette::accent);
    lastHitLabel.setText ("Play MIDI or click the kit", juce::dontSendNotification);
    helpLabel.setFont (juce::FontOptions (12.5f));
    helpLabel.setColour (juce::Label::textColourId, palette::textDim);
    velocityLabel.setText ("Click velocity", juce::dontSendNotification);
    velocityLabel.setFont (juce::FontOptions (12.0f));
    velocityLabel.setColour (juce::Label::textColourId, palette::textDim);
    velocityLabel.setJustificationType (juce::Justification::centredRight);

    velocitySlider.setSliderStyle (juce::Slider::LinearHorizontal);
    velocitySlider.setRange (1.0, 127.0, 1.0);
    velocitySlider.setValue (100.0, juce::dontSendNotification);
    velocitySlider.setTextBoxStyle (juce::Slider::TextBoxRight, false, 36, 18);

    cancelLearnButton.onClick = [this] { processor.cancelMidiLearn(); };

    for (auto* c : std::initializer_list<juce::Component*> { &lastHitLabel, &helpLabel, &velocityLabel, &velocitySlider })
        surface.addAndMakeVisible (c);
    surface.addChildComponent (cancelLearnButton);

    kitTab.setToggleState (true, juce::dontSendNotification);
    showPage (true);

    // Drop hits that queued up while no editor was open.
    for (TabifyDrumsProcessor::HitEvent e; processor.popHitEvent (e);)
        ;

    setResizable (true, true);
    setResizeLimits (designWidth * 7 / 10, designHeight * 7 / 10, designWidth * 2, designHeight * 2);
    if (auto* sizeLimits = getConstrainer())
        sizeLimits->setFixedAspectRatio ((double) designWidth / (double) designHeight);
    setSize (designWidth, designHeight);

    startTimerHz (60);
}

TabifyDrumsEditor::~TabifyDrumsEditor()
{
    setLookAndFeel (nullptr);
}

void TabifyDrumsEditor::resized()
{
    surface.setTransform (juce::AffineTransform::scale ((float) getWidth() / (float) designWidth));
}

void TabifyDrumsEditor::Surface::paint (juce::Graphics& g)
{
    g.fillAll (palette::background);

    auto header = getLocalBounds().removeFromTop (headerHeight);
    g.setGradientFill (juce::ColourGradient (palette::panelLight, 0.0f, 0.0f, palette::panel, 0.0f, (float) headerHeight, false));
    g.fillRect (header);
    g.setColour (juce::Colours::black);
    g.drawHorizontalLine (headerHeight - 1, 0.0f, (float) getWidth());

    // Logo: resources/ui/logo.png, scaled to fit the header height.
    const auto logoArea = juce::Rectangle<float> (18.0f, 10.0f, 90.0f, 30.0f);
    g.drawImageWithin (logo, (int) logoArea.getX(), (int) logoArea.getY(), (int) logoArea.getWidth(), (int) logoArea.getHeight(),
                       juce::RectanglePlacement::xLeft | juce::RectanglePlacement::yMid | juce::RectanglePlacement::onlyReduceInSize);
    const auto logoWidth = logo.isValid() ? logoArea.getHeight() * (float) logo.getWidth() / (float) logo.getHeight() : 0.0f;

    g.setColour (palette::accent);
    g.setFont (juce::FontOptions (22.0f, juce::Font::bold));
    const auto drumsArea = header.withTrimmedLeft (18 + (int) logoWidth + 8).withWidth (90);
    g.drawText ("DRUMS", drumsArea, juce::Justification::centredLeft);
    g.setColour (palette::textDim);
    g.setFont (juce::FontOptions (11.0f));
    g.drawText ("v" JucePlugin_VersionString, drumsArea.withTrimmedLeft (82).withWidth (60), juce::Justification::centredLeft);

    auto info = getLocalBounds().removeFromBottom (infoHeight);
    g.setColour (learning ? palette::accent.withAlpha (0.18f) : palette::panel);
    g.fillRect (info);
    g.setColour (juce::Colours::black);
    g.drawHorizontalLine (info.getY(), 0.0f, (float) getWidth());
}

void TabifyDrumsEditor::showPage (bool kit)
{
    kitView.setVisible (kit);
    mixerView.setVisible (! kit);

    const auto content = juce::Rectangle<int> (0, headerHeight, designWidth, designHeight - headerHeight - infoHeight);
    kitView.setBounds (content);
    mixerView.setBounds (content);

    kitTab.setBounds (300, 10, 90, 30);
    mixerTab.setBounds (396, 10, 90, 30);
    kitSelector.setBounds (510, 11, 200, 28);
    masterLabel.setBounds (720, 10, 70, 30);
    masterKnob.setBounds (795, 4, 140, 42);

    auto info = juce::Rectangle<int> (0, designHeight - infoHeight, designWidth, infoHeight).reduced (16, 6);
    auto right = info.removeFromRight (260);
    velocityLabel.setBounds (right.removeFromLeft (90));
    velocitySlider.setBounds (right.reduced (4, 14));
    lastHitLabel.setBounds (info.removeFromTop (24));
    helpLabel.setBounds (info);
    cancelLearnButton.setBounds (helpLabel.getBounds().removeFromRight (80).reduced (0, 2));
}

juce::String TabifyDrumsEditor::noteName (int note)
{
    return juce::MidiMessage::getMidiNoteName (note, true, true, 3) + " (" + juce::String (note) + ")";
}

juce::String TabifyDrumsEditor::describeNotes (int articulation) const
{
    juce::StringArray names;
    for (const auto note : processor.getNotesFor (articulation))
        names.add (noteName (note));
    return names.isEmpty() ? juce::String ("no note") : names.joinIntoString (", ");
}

void TabifyDrumsEditor::showPieceMenu (int piece)
{
    enum { playBase = 1000, learnBase = 2000, resetBase = 3000, cancelLearn = 4000, resetAll = 4001 };

    juce::PopupMenu menu;
    menu.setLookAndFeel (&lookAndFeel);
    menu.addSectionHeader (kit::pieces[(size_t) piece].name);

    for (int a = 0; a < kit::numArticulations; ++a)
    {
        if (kit::articulations[(size_t) a].piece != piece)
            continue;

        juce::PopupMenu sub;
        sub.addItem (playBase + a, "Play");
        sub.addItem (learnBase + a, "MIDI Learn...");
        sub.addItem (resetBase + a, "Reset to default notes");
        menu.addSubMenu (juce::String (kit::articulations[(size_t) a].name) + "   [" + describeNotes (a) + "]", sub);
    }

    menu.addSeparator();
    if (processor.getMidiLearnArticulation() >= 0)
        menu.addItem (cancelLearn, "Cancel MIDI learn");
    menu.addItem (resetAll, "Reset all MIDI mappings");

    menu.showMenuAsync (juce::PopupMenu::Options().withMousePosition(),
                        [safe = juce::Component::SafePointer<TabifyDrumsEditor> (this)] (int result)
                        {
                            if (safe == nullptr || result == 0)
                                return;

                            auto& proc = safe->processor;
                            if (result >= playBase && result < learnBase)
                                proc.playFromEditor (result - playBase, (int) safe->velocitySlider.getValue());
                            else if (result >= learnBase && result < resetBase)
                                proc.startMidiLearn (result - learnBase);
                            else if (result >= resetBase && result < cancelLearn)
                                proc.resetMappingFor (result - resetBase);
                            else if (result == cancelLearn)
                                proc.cancelMidiLearn();
                            else if (result == resetAll)
                                proc.resetAllMappings();
                        });
}

void TabifyDrumsEditor::updateLearnState()
{
    const auto learning = processor.getMidiLearnArticulation();
    if (learning == lastLearnArticulation)
        return;

    lastLearnArticulation = learning;
    surface.learning = learning >= 0;
    cancelLearnButton.setVisible (learning >= 0);
    kitView.setLearningPiece (learning >= 0 ? (int) kit::articulations[(size_t) learning].piece : -1);

    if (learning >= 0)
        statusText = "MIDI Learn: hit a pad on your controller for "
                     + juce::String (kit::pieces[(size_t) kit::articulations[(size_t) learning].piece].name)
                     + middleDot() + kit::articulations[(size_t) learning].name;
    else if (! statusText.startsWith ("Assigned"))
        statusText = {};

    helpLabel.setText (statusText, juce::dontSendNotification);
    surface.repaint();
}

void TabifyDrumsEditor::timerCallback()
{
    TabifyDrumsProcessor::HitEvent e;
    while (processor.popHitEvent (e))
    {
        kitView.notifyHit (e.articulation, e.velocity);

        const auto& info = kit::articulations[(size_t) e.articulation];
        auto text = juce::String (kit::pieces[(size_t) info.piece].name);
        if (juce::String (info.name) != text)
            text << middleDot() << info.name;
        text << middleDot() << (e.note >= 0 ? noteName (e.note) : juce::String ("click"))
             << middleDot() << "vel " << e.velocity;
        lastHitLabel.setText (text, juce::dontSendNotification);

        if (e.learned)
        {
            statusText = "Assigned " + noteName (e.note) + " to " + kit::pieces[(size_t) info.piece].name
                         + middleDot() + info.name;
            lastLearnArticulation = -2; // force a refresh of the learn display
        }
    }

    updateLearnState();

    std::vector<float> peaks;
    for (int i = 0; i <= kit::numPieces; ++i)
        peaks.push_back (processor.popPeak (i));
    if (mixerView.isVisible())
        mixerView.setLevels (peaks);
}
