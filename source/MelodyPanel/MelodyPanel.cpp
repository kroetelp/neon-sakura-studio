#include "MelodyPanel.h"

//==============================================================================
// MiniKeyboard Implementation
//==============================================================================

void MelodyPanel::MiniKeyboard::paint(juce::Graphics& g)
{
    auto& theme = ThemeManager::getInstance();
    auto bounds = getLocalBounds().toFloat();
    float whiteKeyWidth = bounds.getWidth() / 7.0f;
    float blackKeyWidth = whiteKeyWidth * 0.6f;

    // White keys
    int whiteNotes[] = {0, 2, 4, 5, 7, 9, 11};  // C, D, E, F, G, A, B
    for (int i = 0; i < 7; ++i)
    {
        float x = i * whiteKeyWidth;
        bool isActive = std::find(activeNotes.begin(), activeNotes.end(), whiteNotes[i]) != activeNotes.end();

        g.setColour(isActive ? theme.getInfoColor() : juce::Colours::white);
        g.fillRect(x + 1, 0.0f, whiteKeyWidth - 2, bounds.getHeight());
        g.setColour(juce::Colours::darkgrey);
        g.drawRect(x + 1, 0.0f, whiteKeyWidth - 2, bounds.getHeight());
    }

    // Black keys
    int blackNotes[] = {1, 3, 6, 8, 10};  // C#, D#, F#, G#, A#
    float blackPositions[] = {0.7f, 1.7f, 3.7f, 4.7f, 5.7f};

    for (int i = 0; i < 5; ++i)
    {
        float x = blackPositions[i] * whiteKeyWidth - blackKeyWidth / 2;
        bool isActive = std::find(activeNotes.begin(), activeNotes.end(), blackNotes[i]) != activeNotes.end();

        g.setColour(isActive ? theme.getAccentColor() : juce::Colours::black);
        g.fillRect(x, 0.0f, blackKeyWidth, bounds.getHeight() * 0.6f);
    }
}

//==============================================================================
// PianoRoll Implementation
//==============================================================================

void MelodyPanel::PianoRoll::paint(juce::Graphics& g)
{
    auto& theme = ThemeManager::getInstance();
    auto bounds = getLocalBounds().toFloat();
    float stepWidth = bounds.getWidth() / 16.0f;
    float noteHeight = bounds.getHeight() / 24.0f;  // 2 octaves

    // Background
    g.fillAll(theme.getPanelBackgroundColor());

    // Grid lines
    g.setColour(juce::Colours::white.withAlpha(0.1f));
    for (int i = 0; i <= 16; ++i)
    {
        float x = i * stepWidth;
        g.drawVerticalLine(static_cast<int>(x), 0, bounds.getHeight());
    }

    // Scale highlight
    auto scales = MusicTheory::Scales::getAllScales();
    if (scaleIndex >= 0 && scaleIndex < static_cast<int>(scales.size()))
    {
        const auto& scale = scales[scaleIndex];
        g.setColour(theme.getInfoColor().withAlpha(0.1f));

        for (int interval : scale.intervals)
        {
            int noteY = static_cast<int>((24 - (interval % 12 + 12)) * noteHeight);
            g.fillRect(0.0f, static_cast<float>(noteY), bounds.getWidth(), noteHeight);
        }
    }

    // Draw notes
    for (const auto& note : melody)
    {
        if (note.step < 0 || note.step >= 16) continue;

        float x = note.step * stepWidth;
        int noteInOctave = note.midiNote % 12;
        int octave = note.midiNote / 12;
        int y = static_cast<int>((24 - (noteInOctave + (octave - 3) * 12)) * noteHeight);

        if (y >= 0 && y < bounds.getHeight())
        {
            juce::Path p;
            p.addRoundedRectangle(x + 2, static_cast<float>(y), stepWidth - 4, noteHeight - 2, 2);

            melatonin::DropShadow glow(theme.getAccentColor(), 3, {0, 0});
            glow.render(g, p);

            g.setColour(note.isGhost ? theme.getTextSecondaryColor() : theme.getAccentColor());
            g.fillPath(p);
        }
    }
}

//==============================================================================
// MelodyPanel Implementation
//==============================================================================

MelodyPanel::MelodyPanel()
{
    // Root note selection
    addAndMakeVisible(keyLabel);
    keyLabel.setText("Key:", juce::dontSendNotification);
    keyLabel.setColour(juce::Label::textColourId, juce::Colours::white);
    keyLabel.attachToComponent(&rootNoteCombo, true);

    setupComboBox(rootNoteCombo, {"C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B"}, 0);
    rootNoteCombo.setColour(juce::ComboBox::backgroundColourId, getDarkBackground());
    rootNoteCombo.setColour(juce::ComboBox::textColourId, juce::Colours::white);

    // Scale selection
    addAndMakeVisible(scaleLabel);
    scaleLabel.setText("Scale:", juce::dontSendNotification);
    scaleLabel.setColour(juce::Label::textColourId, juce::Colours::white);
    scaleLabel.attachToComponent(&scaleCombo, true);

    setupComboBox(scaleCombo, MelodyGenerator::getScaleNames(), 0);
    scaleCombo.setColour(juce::ComboBox::backgroundColourId, getDarkBackground());
    scaleCombo.setColour(juce::ComboBox::textColourId, juce::Colours::white);

    // Octave controls
    addAndMakeVisible(octaveLabel);
    octaveLabel.setText("Oct:", juce::dontSendNotification);
    octaveLabel.setColour(juce::Label::textColourId, juce::Colours::white);

    setupSlider(octaveSlider, 1, 6, 4);
    octaveSlider.setColour(juce::Slider::thumbColourId, getNeonCyan());

    addAndMakeVisible(octaveRangeLabel);
    octaveRangeLabel.setText("Range:", juce::dontSendNotification);
    octaveRangeLabel.setColour(juce::Label::textColourId, juce::Colours::white);

    setupSlider(octaveRangeSlider, 1, 3, 1);
    octaveRangeSlider.setColour(juce::Slider::thumbColourId, getNeonCyan());

    // Melody type
    addAndMakeVisible(melodyTypeLabel);
    melodyTypeLabel.setText("Type:", juce::dontSendNotification);
    melodyTypeLabel.setColour(juce::Label::textColourId, juce::Colours::white);

    setupComboBox(melodyTypeCombo, MelodyGenerator::getMelodyTypeNames(), 0);
    melodyTypeCombo.setColour(juce::ComboBox::backgroundColourId, getDarkBackground());
    melodyTypeCombo.setColour(juce::ComboBox::textColourId, getNeonPurple());

    // Chord progression
    addAndMakeVisible(progressionLabel);
    progressionLabel.setText("Progression:", juce::dontSendNotification);
    progressionLabel.setColour(juce::Label::textColourId, juce::Colours::white);

    setupComboBox(progressionCombo, MelodyGenerator::getProgressionNames(), 0);
    progressionCombo.setColour(juce::ComboBox::backgroundColourId, getDarkBackground());
    progressionCombo.setColour(juce::ComboBox::textColourId, getNeonOrange());

    // Pattern settings
    addAndMakeVisible(densityLabel);
    densityLabel.setText("Density:", juce::dontSendNotification);
    densityLabel.setColour(juce::Label::textColourId, juce::Colours::white);

    setupSlider(densitySlider, 10, 100, 50);
    densitySlider.setColour(juce::Slider::thumbColourId, getNeonGreen());

    addAndMakeVisible(variationLabel);
    variationLabel.setText("Variation:", juce::dontSendNotification);
    variationLabel.setColour(juce::Label::textColourId, juce::Colours::white);

    setupSlider(variationSlider, 0, 100, 30);
    variationSlider.setColour(juce::Slider::thumbColourId, getNeonGreen());

    // Rhythm settings
    addAndMakeVisible(syncopateToggle);
    syncopateToggle.setButtonText("Syncopate");
    syncopateToggle.setColour(juce::ToggleButton::textColourId, juce::Colours::white);
    syncopateToggle.setColour(juce::ToggleButton::tickColourId, getNeonCyan());

    addAndMakeVisible(maxLeapLabel);
    maxLeapLabel.setText("Max Leap:", juce::dontSendNotification);
    maxLeapLabel.setColour(juce::Label::textColourId, juce::Colours::white);

    setupSlider(maxLeapSlider, 1, 12, 5);
    maxLeapSlider.setColour(juce::Slider::thumbColourId, getNeonPink());

    // Chord type for arpeggios
    addAndMakeVisible(chordTypeCombo);
    setupComboBox(chordTypeCombo, MelodyGenerator::getChordNames(), 0);
    chordTypeCombo.setColour(juce::ComboBox::backgroundColourId, getDarkBackground());
    chordTypeCombo.setColour(juce::ComboBox::textColourId, getNeonPurple());

    addAndMakeVisible(arpUpToggle);
    arpUpToggle.setButtonText("Up");
    arpUpToggle.setToggleState(true, juce::dontSendNotification);
    arpUpToggle.setColour(juce::ToggleButton::textColourId, juce::Colours::white);

    addAndMakeVisible(arpDownToggle);
    arpDownToggle.setButtonText("Down");
    arpDownToggle.setColour(juce::ToggleButton::textColourId, juce::Colours::white);

    // Step count
    addAndMakeVisible(stepCountLabel);
    stepCountLabel.setText("Steps:", juce::dontSendNotification);
    stepCountLabel.setColour(juce::Label::textColourId, juce::Colours::white);

    setupSlider(stepCountSlider, 4, 64, 16);
    stepCountSlider.setColour(juce::Slider::thumbColourId, getNeonOrange());

    // Target track selection
    addAndMakeVisible(targetTrackLabel);
    targetTrackLabel.setText("Target:", juce::dontSendNotification);
    targetTrackLabel.setColour(juce::Label::textColourId, juce::Colours::white);

    addAndMakeVisible(targetTrackCombo);
    for (int i = 0; i < 8; ++i)
        targetTrackCombo.addItem("Track " + juce::String(i + 1), i + 1);
    targetTrackCombo.setSelectedId(1);
    targetTrackCombo.setColour(juce::ComboBox::backgroundColourId, getDarkBackground());
    targetTrackCombo.setColour(juce::ComboBox::textColourId, getNeonPink());
    targetTrackCombo.setColour(juce::ComboBox::outlineColourId, getNeonPink());
    targetTrackCombo.onChange = [this] {
        targetTrack = targetTrackCombo.getSelectedId() - 1;
    };

    // Action buttons
    setupButton(generateMelodyBtn, "Generate Melody", getNeonPink());
    generateMelodyBtn.onClick = [this] { generateMelody(); };

    setupButton(generateArpeggioBtn, "Arpeggio", getNeonCyan());
    generateArpeggioBtn.onClick = [this] { generateArpeggio(); };

    setupButton(generateBassBtn, "Bass Line", getNeonPurple());
    generateBassBtn.onClick = [this] { generateBassLine(); };

    setupButton(generateLeadBtn, "Lead Line", getNeonGreen());
    generateLeadBtn.onClick = [this] { generateLeadLine(); };

    setupButton(generateProgressionBtn, "Progression", getNeonOrange());
    generateProgressionBtn.onClick = [this] { generateProgression(); };

    setupButton(clearBtn, "Clear", juce::Colours::darkred);
    clearBtn.onClick = [this] { clearMelody(); };

    // Piano roll preview
    pianoRoll = std::make_unique<PianoRoll>();
    addAndMakeVisible(pianoRoll.get());

    // Keyboard preview
    keyboard = std::make_unique<MiniKeyboard>();
    addAndMakeVisible(keyboard.get());
}

void MelodyPanel::resized()
{
    auto area = getLocalBounds().reduced(10);

    // Title
    auto titleArea = area.removeFromTop(25);

    // Row 1: Key, Scale, Octave
    auto row1 = area.removeFromTop(35);
    rootNoteCombo.setBounds(row1.removeFromLeft(70));
    row1.removeFromLeft(5);
    scaleCombo.setBounds(row1.removeFromLeft(120));
    row1.removeFromLeft(5);
    octaveLabel.setBounds(row1.removeFromLeft(30));
    octaveSlider.setBounds(row1.removeFromLeft(60));
    row1.removeFromLeft(5);
    octaveRangeLabel.setBounds(row1.removeFromLeft(45));
    octaveRangeSlider.setBounds(row1.removeFromLeft(60));

    // Row 2: Type, Density, Variation
    auto row2 = area.removeFromTop(35);
    melodyTypeLabel.setBounds(row2.removeFromLeft(35));
    melodyTypeCombo.setBounds(row2.removeFromLeft(120));
    row2.removeFromLeft(5);
    densityLabel.setBounds(row2.removeFromLeft(50));
    densitySlider.setBounds(row2.removeFromLeft(100));
    row2.removeFromLeft(5);
    variationLabel.setBounds(row2.removeFromLeft(55));
    variationSlider.setBounds(row2.removeFromLeft(100));

    // Row 3: Rhythm, Leap, Syncopate
    auto row3 = area.removeFromTop(35);
    maxLeapLabel.setBounds(row3.removeFromLeft(55));
    maxLeapSlider.setBounds(row3.removeFromLeft(80));
    row3.removeFromLeft(10);
    syncopateToggle.setBounds(row3.removeFromLeft(90));
    row3.removeFromLeft(10);
    targetTrackLabel.setBounds(row3.removeFromLeft(45));
    targetTrackCombo.setBounds(row3.removeFromLeft(90));

    // Row 4: Chord type, Arp direction, Steps
    auto row4 = area.removeFromTop(35);
    chordTypeCombo.setBounds(row4.removeFromLeft(100));
    row4.removeFromLeft(10);
    arpUpToggle.setBounds(row4.removeFromLeft(50));
    arpDownToggle.setBounds(row4.removeFromLeft(50));
    row4.removeFromLeft(10);
    stepCountLabel.setBounds(row4.removeFromLeft(40));
    stepCountSlider.setBounds(row4.removeFromLeft(100));
    row4.removeFromLeft(10);
    progressionCombo.setBounds(row4.removeFromLeft(140));

    // Piano roll preview
    pianoRoll->setBounds(area.removeFromTop(80));

    // Keyboard preview
    keyboard->setBounds(area.removeFromTop(40));

    // Action buttons
    auto btnRow1 = area.removeFromTop(35);
    generateMelodyBtn.setBounds(btnRow1.removeFromLeft(120));
    btnRow1.removeFromLeft(5);
    generateArpeggioBtn.setBounds(btnRow1.removeFromLeft(80));
    btnRow1.removeFromLeft(5);
    generateBassBtn.setBounds(btnRow1.removeFromLeft(80));
    btnRow1.removeFromLeft(5);
    generateLeadBtn.setBounds(btnRow1.removeFromLeft(80));
    btnRow1.removeFromLeft(5);
    clearBtn.setBounds(btnRow1.removeFromLeft(60));

    auto btnRow2 = area.removeFromTop(35);
    generateProgressionBtn.setBounds(btnRow2.removeFromLeft(100));
}

void MelodyPanel::paint(juce::Graphics& g)
{
    auto gradient = juce::ColourGradient::vertical(
        getDarkBackground().brighter(0.05f), 0,
        getDarkBackground(), getHeight());
    g.setGradientFill(gradient);
    g.fillAll();

    g.setColour(getNeonPurple().withAlpha(0.3f));
    g.drawRoundedRectangle(getLocalBounds().toFloat().reduced(2), 5, 1);

    g.setColour(getNeonPurple());
    g.setFont(juce::Font(13.0f, juce::Font::bold));
    g.drawText("MELODY WORKSTATION", getLocalBounds().removeFromTop(22),
               juce::Justification::centred, false);
}

MelodyParams MelodyPanel::getCurrentParams() const
{
    MelodyParams params;
    params.rootNote = rootNoteCombo.getSelectedItemIndex();
    params.scaleIndex = scaleCombo.getSelectedItemIndex();
    params.octave = static_cast<int>(octaveSlider.getValue());
    params.octaveRange = static_cast<int>(octaveRangeSlider.getValue());
    params.density = static_cast<int>(densitySlider.getValue());
    params.variation = static_cast<int>(variationSlider.getValue());
    params.maxLeap = static_cast<int>(maxLeapSlider.getValue());
    params.syncopate = syncopateToggle.getToggleState();
    params.chordIndex = chordTypeCombo.getSelectedItemIndex();
    params.arpeggiateUp = arpUpToggle.getToggleState();
    params.arpeggiateDown = arpDownToggle.getToggleState();
    params.type = static_cast<MelodyType>(melodyTypeCombo.getSelectedItemIndex());
    return params;
}

void MelodyPanel::generateMelody()
{
    auto params = getCurrentParams();
    int steps = static_cast<int>(stepCountSlider.getValue());
    currentMelody = generator.generateMelody(params, steps);

    pianoRoll->setMelody(currentMelody);
    pianoRoll->setScale(params.rootNote, params.scaleIndex);

    applyMelody();
}

void MelodyPanel::generateArpeggio()
{
    auto params = getCurrentParams();
    int steps = static_cast<int>(stepCountSlider.getValue());
    currentMelody = generator.generateArpeggio(params, steps);

    pianoRoll->setMelody(currentMelody);
    pianoRoll->setScale(params.rootNote, params.scaleIndex);

    applyMelody();
}

void MelodyPanel::generateBassLine()
{
    auto params = getCurrentParams();
    int steps = static_cast<int>(stepCountSlider.getValue());
    currentMelody = generator.generateBassLine(params, steps);

    pianoRoll->setMelody(currentMelody);
    pianoRoll->setScale(params.rootNote, params.scaleIndex);

    applyMelody();
}

void MelodyPanel::generateLeadLine()
{
    auto params = getCurrentParams();
    int steps = static_cast<int>(stepCountSlider.getValue());
    currentMelody = generator.generateLeadLine(params, steps);

    pianoRoll->setMelody(currentMelody);
    pianoRoll->setScale(params.rootNote, params.scaleIndex);

    applyMelody();
}

void MelodyPanel::generateProgression()
{
    auto params = getCurrentParams();
    auto progressions = MusicTheory::Progressions::getAllProgressions();
    int progIndex = progressionCombo.getSelectedItemIndex();

    if (progIndex >= 0 && progIndex < static_cast<int>(progressions.size()))
    {
        currentMelody = generator.generateChordProgression(progressions[progIndex], params);

        pianoRoll->setMelody(currentMelody);
        pianoRoll->setScale(params.rootNote, params.scaleIndex);

        applyMelody();
    }
}

void MelodyPanel::clearMelody()
{
    currentMelody.clear();
    pianoRoll->setMelody(currentMelody);

    if (onApplyMelody)
        onApplyMelody(targetTrack, {});
}

void MelodyPanel::applyMelody()
{
    if (onApplyMelody && !currentMelody.empty())
    {
        auto steps = MelodyConverter::melodyToSteps(currentMelody, 60);
        onApplyMelody(targetTrack, steps);
    }
}

void MelodyPanel::setupComboBox(juce::ComboBox& combo, const juce::StringArray& items, int defaultIndex)
{
    addAndMakeVisible(combo);
    combo.clear();
    combo.addItemList(items, 1);
    combo.setSelectedItemIndex(defaultIndex);
}

void MelodyPanel::setupSlider(juce::Slider& slider, double min, double max, double initial, double step)
{
    addAndMakeVisible(slider);
    slider.setRange(min, max, step);
    slider.setValue(initial);
    slider.setSliderStyle(juce::Slider::LinearHorizontal);
    slider.setTextBoxStyle(juce::Slider::TextBoxRight, false, 35, 20);
    slider.setColour(juce::Slider::trackColourId, juce::Colours::darkgrey);
    slider.setColour(juce::Slider::textBoxTextColourId, juce::Colours::white);
    slider.setColour(juce::Slider::textBoxBackgroundColourId, getDarkBackground());
}

void MelodyPanel::setupButton(juce::TextButton& btn, const juce::String& text, juce::Colour color)
{
    addAndMakeVisible(btn);
    btn.setButtonText(text);
    btn.setColour(juce::TextButton::buttonColourId, getDarkBackground());
    btn.setColour(juce::TextButton::textColourOffId, color);
}
