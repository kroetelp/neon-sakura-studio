#include "WootingSettingsPanel.h"

WootingSettingsPanel::WootingSettingsPanel(WootingManager& manager)
    : wootingManager(manager)
{
    // --- Title ---
    titleLabel.setText("Wooting Analog Keyboard", juce::dontSendNotification);
    titleLabel.setFont(juce::Font(18.0f, juce::Font::bold));
    titleLabel.setColour(juce::Label::textColourId, getNeonCyan());
    titleLabel.setJustificationType(juce::Justification::centred);
    addAndMakeVisible(titleLabel);

    // --- Status Label ---
    bool connected = wootingManager.isConnected();
    statusLabel.setText(connected ? "Status: Connected" : "Status: Not Detected",
                        juce::dontSendNotification);
    statusLabel.setFont(juce::Font(12.0f));
    statusLabel.setColour(juce::Label::textColourId,
                          connected ? getNeonGreen() : getNeonPink());
    statusLabel.setJustificationType(juce::Justification::centred);
    addAndMakeVisible(statusLabel);

    // --- Velocity Curve ---
    velocityCurveLabel.setText("Velocity Curve:", juce::dontSendNotification);
    velocityCurveLabel.setColour(juce::Label::textColourId, juce::Colours::white);
    velocityCurveLabel.setFont(juce::Font(12.0f));
    addAndMakeVisible(velocityCurveLabel);

    velocityCurveComboBox.addItem("Linear (1:1)", 1);
    velocityCurveComboBox.addItem("Soft (Sqrt) - Easy high velocity", 2);
    velocityCurveComboBox.addItem("Hard (Pow2) - Requires more force", 3);
    velocityCurveComboBox.setSelectedId(1, juce::dontSendNotification);
    velocityCurveComboBox.setColour(juce::ComboBox::backgroundColourId, getDarkBackground());
    velocityCurveComboBox.setColour(juce::ComboBox::textColourId, juce::Colours::white);
    velocityCurveComboBox.setColour(juce::ComboBox::outlineColourId, getNeonCyan());

    velocityCurveComboBox.onChange = [this]() {
        int selectedId = velocityCurveComboBox.getSelectedId();
        wootingManager.setVelocityCurve(selectedId - 1);  // 0, 1, or 2
    };
    addAndMakeVisible(velocityCurveComboBox);

    // --- Pressure (Aftertouch) Curve ---
    pressureCurveLabel.setText("Pressure Curve (Aftertouch):", juce::dontSendNotification);
    pressureCurveLabel.setColour(juce::Label::textColourId, juce::Colours::white);
    pressureCurveLabel.setFont(juce::Font(12.0f));
    addAndMakeVisible(pressureCurveLabel);

    pressureCurveComboBox.addItem("Linear - Direct response", 1);
    pressureCurveComboBox.addItem("Soft (Sqrt) - Sensitive at low pressure", 2);
    pressureCurveComboBox.addItem("Hard (Pow2) - Requires firm pressure", 3);
    pressureCurveComboBox.setSelectedId(1, juce::dontSendNotification);
    pressureCurveComboBox.setColour(juce::ComboBox::backgroundColourId, getDarkBackground());
    pressureCurveComboBox.setColour(juce::ComboBox::textColourId, juce::Colours::white);
    pressureCurveComboBox.setColour(juce::ComboBox::outlineColourId, getNeonPurple());

    pressureCurveComboBox.onChange = [this]() {
        int selectedId = pressureCurveComboBox.getSelectedId();
        wootingManager.setPressureCurve(selectedId - 1);  // 0, 1, or 2
    };
    addAndMakeVisible(pressureCurveComboBox);

    // --- Octave Offset ---
    octaveLabel.setText("Octave Offset:", juce::dontSendNotification);
    octaveLabel.setColour(juce::Label::textColourId, juce::Colours::white);
    octaveLabel.setFont(juce::Font(12.0f));
    addAndMakeVisible(octaveLabel);

    octaveSlider.setSliderStyle(juce::Slider::LinearHorizontal);
    octaveSlider.setTextBoxStyle(juce::Slider::TextBoxLeft, false, 40, 20);
    octaveSlider.setRange(-2.0, 2.0, 1.0);
    octaveSlider.setValue(0.0, juce::dontSendNotification);
    octaveSlider.setColour(juce::Slider::thumbColourId, getNeonPink());
    octaveSlider.setColour(juce::Slider::trackColourId, juce::Colours::darkgrey);
    octaveSlider.setColour(juce::Slider::textBoxTextColourId, juce::Colours::white);
    octaveSlider.setColour(juce::Slider::textBoxBackgroundColourId, getDarkBackground());

    octaveSlider.onValueChange = [this]() {
        wootingManager.setOctaveOffset(static_cast<int>(octaveSlider.getValue()));
    };
    addAndMakeVisible(octaveSlider);

    // --- Test Button ---
    testButton.setButtonText("Test Note (C4)");
    testButton.setColour(juce::TextButton::buttonColourId, getDarkBackground());
    testButton.setColour(juce::TextButton::textColourOffId, getNeonCyan());
    testButton.onClick = [this]() {
        // Trigger a test note through the MIDI queue
        auto& queue = wootingManager.getMidiQueue();
        queue.push(MidiEvent::noteOn(1, 60, 0.8f));

        // Schedule note off after 200ms
        juce::Timer::callAfterDelay(200, [this]() {
            auto& queue = wootingManager.getMidiQueue();
            queue.push(MidiEvent::noteOff(1, 60));
        });
    };
    addAndMakeVisible(testButton);

    // --- Active Keys Display ---
    activeKeysLabel.setText("Active Keys: 0", juce::dontSendNotification);
    activeKeysLabel.setColour(juce::Label::textColourId, juce::Colours::lightgrey);
    activeKeysLabel.setFont(juce::Font(11.0f));
    activeKeysLabel.setJustificationType(juce::Justification::centred);
    addAndMakeVisible(activeKeysLabel);

    // Start status update timer (10 Hz)
    startTimer(100);
}

WootingSettingsPanel::~WootingSettingsPanel()
{
    stopTimer();
}

void WootingSettingsPanel::timerCallback()
{
    int numKeys = wootingManager.getNumActiveKeys();
    activeKeysLabel.setText("Active Keys: " + juce::String(numKeys),
                            juce::dontSendNotification);
}

void WootingSettingsPanel::paint(juce::Graphics& g)
{
    g.fillAll(getDarkBackground());

    // Draw border
    g.setColour(juce::Colour(40, 40, 60));
    g.drawRect(getLocalBounds(), 1);

    // Draw decorative line under title
    auto titleBounds = titleLabel.getBounds();
    g.setColour(getNeonCyan().withAlpha(0.3f));
    g.drawHorizontalLine(titleBounds.getBottom() + 5,
                         titleBounds.getX(),
                         titleBounds.getRight());
}

void WootingSettingsPanel::resized()
{
    auto area = getLocalBounds().reduced(15);

    // Title
    titleLabel.setBounds(area.removeFromTop(30));
    area.removeFromTop(5);

    // Status
    statusLabel.setBounds(area.removeFromTop(20));
    area.removeFromTop(15);

    // FlexBox for controls
    juce::FlexBox flexBox;
    flexBox.flexDirection = juce::FlexBox::Direction::column;
    flexBox.alignItems = juce::FlexBox::AlignItems::stretch;
    flexBox.justifyContent = juce::FlexBox::JustifyContent::flexStart;

    juce::FlexItem::Margin labelMargin(5, 0, 3, 0);
    juce::FlexItem::Margin controlMargin(0, 0, 12, 0);

    // Velocity Curve Controls
    flexBox.items.add(juce::FlexItem(velocityCurveLabel).withHeight(18).withMargin(labelMargin));
    flexBox.items.add(juce::FlexItem(velocityCurveComboBox).withHeight(28).withMargin(controlMargin));

    // Pressure Curve Controls
    flexBox.items.add(juce::FlexItem(pressureCurveLabel).withHeight(18).withMargin(labelMargin));
    flexBox.items.add(juce::FlexItem(pressureCurveComboBox).withHeight(28).withMargin(controlMargin));

    // Octave Controls
    flexBox.items.add(juce::FlexItem(octaveLabel).withHeight(18).withMargin(labelMargin));
    flexBox.items.add(juce::FlexItem(octaveSlider).withHeight(28).withMargin(controlMargin));

    // Test Button
    flexBox.items.add(juce::FlexItem(testButton).withHeight(32).withMargin(controlMargin));

    // Active Keys
    flexBox.items.add(juce::FlexItem(activeKeysLabel).withHeight(18));

    flexBox.performLayout(area);
}
