#include "TrackHeader.h"
#include <melatonin_blur.h>

//==============================================================================
// TrackHeader Implementation
//==============================================================================

TrackHeader::TrackHeader(int trackIndex)
    : trackIndex(trackIndex), currentTrackType(TrackType::Sampler)
{
    initializeComponents();
    setupListeners();
}

TrackHeader::~TrackHeader() = default;

void TrackHeader::initializeComponents()
{
    // Track type combo
    trackTypeCombo = std::make_unique<juce::ComboBox>();
    trackTypeCombo->addItem("Sample", 1);
    trackTypeCombo->addItem("Synth", 2);
    trackTypeCombo->addItem("Audio", 3);
    trackTypeCombo->addItem("Plugin", 4);
    trackTypeCombo->setSelectedId(1, juce::dontSendNotification);

    // Track name label
    trackNameLabel = std::make_unique<juce::Label>();
    trackNameLabel->setText("Track " + juce::String(trackIndex + 1), juce::dontSendNotification);
    trackNameLabel->setFont(juce::Font(12, juce::Font::bold));
    trackNameLabel->setJustificationType(juce::Justification::centredLeft);

    // Volume slider
    volumeSlider = std::make_unique<juce::Slider>(juce::Slider::LinearBar, juce::Slider::NoTextBox);
    volumeSlider->setRange(0.0, 1.5, 0.01);
    volumeSlider->setValue(0.8f, juce::dontSendNotification);
    volumeSlider->setPopupMenuEnabled(true);

    // Mute button
    muteButton = std::make_unique<juce::TextButton>();
    muteButton->setButtonText("M");
    muteButton->setClickingTogglesState(true);
    muteButton->setTooltip("Mute Track");

    // Solo button
    soloButton = std::make_unique<juce::TextButton>();
    soloButton->setButtonText("S");
    soloButton->setClickingTogglesState(true);
    soloButton->setTooltip("Solo Track");

    // Expand/Collapse button
    expandButton = std::make_unique<juce::TextButton>();
    expandButton->setButtonText("▼");
    expandButton->setTooltip("Expand/Collapse Track");

    // Plugin button
    pluginButton = std::make_unique<juce::TextButton>();
    pluginButton->setButtonText("FX");
    pluginButton->setTooltip("Plugin Chain");

    // Add child components
    addAndMakeVisible(*trackTypeCombo);
    addAndMakeVisible(*trackNameLabel);
    addAndMakeVisible(*volumeSlider);
    addAndMakeVisible(*muteButton);
    addAndMakeVisible(*soloButton);
    addAndMakeVisible(*expandButton);
    addAndMakeVisible(*pluginButton);
}

void TrackHeader::setupListeners()
{
    trackTypeCombo->onChange = [this]
    {
        int selectedId = trackTypeCombo->getSelectedId();
        TrackType newType = TrackType::Sampler;

        switch (selectedId)
        {
            case 1: newType = TrackType::Sampler; break;
            case 2: newType = TrackType::Wavetable; break;
            case 3: newType = TrackType::Audio; break;
            case 4: newType = TrackType::Plugin; break;
        }

        if (newType != currentTrackType)
        {
            currentTrackType = newType;
            if (onTrackTypeChanged)
                onTrackTypeChanged(newType);
        }
    };

    volumeSlider->onValueChange = [this]
    {
        volume = static_cast<float>(volumeSlider->getValue());
        if (onVolumeChanged)
            onVolumeChanged(volume);
    };

    muteButton->onClick = [this]
    {
        muted = muteButton->getToggleState();
        muteButton->setButtonText(muted ? "M" : "M");
        if (onMuteChanged)
            onMuteChanged(muted);
    };

    soloButton->onClick = [this]
    {
        solo = soloButton->getToggleState();
        soloButton->setButtonText(solo ? "S" : "S");
        if (onSoloChanged)
            onSoloChanged(solo);
    };

    expandButton->onClick = [this]
    {
        expanded = !expanded;
        expandButton->setButtonText(expanded ? "▼" : "▶");
        if (onExpandToggled)
            onExpandToggled();
    };

    pluginButton->onClick = [this]
    {
        if (onPluginButtonClicked)
            onPluginButtonClicked();
    };
}

void TrackHeader::resized()
{
    auto bounds = getLocalBounds();

    const int typeWidth = 70;
    const int nameWidth = 100;
    const int volumeWidth = 80;
    const int buttonSize = 24;
    const int gap = 4;

    int x = bounds.getX();
    int y = bounds.getY();
    int height = bounds.getHeight();

    // Track type combo (left)
    trackTypeCombo->setBounds(x, y + (height - 20) / 2, typeWidth, 20);
    x += typeWidth + gap;

    // Track name
    trackNameLabel->setBounds(x, y, nameWidth, height);
    x += nameWidth + gap * 2;

    // Volume slider
    volumeSlider->setBounds(x, y + (height - 16) / 2, volumeWidth, 16);
    x += volumeWidth + gap;

    // Mute button
    muteButton->setBounds(x, y + (height - buttonSize) / 2, buttonSize, buttonSize);
    x += buttonSize + gap;

    // Solo button
    soloButton->setBounds(x, y + (height - buttonSize) / 2, buttonSize, buttonSize);
    x += buttonSize + gap * 2;

    // Spacer for routing indicators
    int routingIndicatorWidth = 60;
    x += routingIndicatorWidth;

    // Plugin button (right)
    pluginButton->setBounds(x, y + (height - buttonSize) / 2, 40, buttonSize);

    // Expand button (far right)
    expandButton->setBounds(bounds.getRight() - buttonSize - gap,
                          y + (height - buttonSize) / 2,
                          buttonSize, buttonSize);
}

void TrackHeader::paint(juce::Graphics& g)
{
    auto bounds = getLocalBounds();

    // Background
    g.fillAll(getDarkBackground());

    // Bottom border
    g.setColour(juce::Colours::white.withAlpha(0.1f));
    g.drawLine(bounds.getX(), bounds.getBottom() - 1,
              bounds.getRight(), bounds.getBottom() - 1);

    // Draw plugin indicators as small dots
    int indicatorX = bounds.getX() + 70 + 100 + 80 + 4 * 4 + 24 * 2 + 8;
    int indicatorY = bounds.getCentre().y - 3;
    int indicatorGap = 6;

    for (int i = 0; i < pluginCount && i < maxPluginIndicators; ++i)
    {
        float x = indicatorX + i * indicatorGap;

        // Plugin indicator dot
        juce::Colour dotColor = getNeonPink();
        if (i == 0)
            dotColor = getNeonCyan();
        else if (i == 1)
            dotColor = getNeonPurple();

        g.setColour(dotColor.withAlpha(0.8f));
        g.fillEllipse(x, indicatorY, 5, 5);
    }

    // Draw routing indicators
    int routingX = indicatorX + maxPluginIndicators * indicatorGap + 10;

    if (hasSidechainSource || hasSidechainTarget)
    {
        // Sidechain icon (small S)
        g.setColour(juce::Colours::orange.withAlpha(0.8f));
        g.setFont(juce::Font(10, juce::Font::bold));
        g.drawText("SC", routingX, indicatorY - 2, 20, 10,
                   juce::Justification::centred);
    }

    if (hasMIDIRoute)
    {
        // MIDI routing icon (small M)
        g.setColour(juce::Colours::green.withAlpha(0.8f));
        g.setFont(juce::Font(10, juce::Font::bold));
        g.drawText("MIDI", routingX + 25, indicatorY - 2, 30, 10,
                   juce::Justification::centred);
    }
}

void TrackHeader::setTrackType(TrackType type)
{
    currentTrackType = type;
    int comboId = 1;

    switch (type)
    {
        case TrackType::Sampler: comboId = 1; break;
        case TrackType::Wavetable: comboId = 2; break;
        case TrackType::Audio: comboId = 3; break;
        case TrackType::Plugin: comboId = 4; break;
    }

    trackTypeCombo->setSelectedId(comboId, juce::dontSendNotification);
}

void TrackHeader::setPluginCount(int count)
{
    pluginCount = juce::jmin(count, maxPluginIndicators);
    repaint();
}

void TrackHeader::setHasSidechainSource(bool hasSource)
{
    hasSidechainSource = hasSource;
    repaint();
}

void TrackHeader::setHasSidechainTarget(bool hasTarget)
{
    hasSidechainTarget = hasTarget;
    repaint();
}

void TrackHeader::setHasMIDIRoute(bool hasRoute)
{
    hasMIDIRoute = hasRoute;
    repaint();
}

void TrackHeader::setTrackName(const juce::String& name)
{
    trackNameLabel->setText(name, juce::dontSendNotification);
}

juce::String TrackHeader::getTrackName() const
{
    return trackNameLabel->getText();
}

void TrackHeader::setVolume(float vol)
{
    volume = vol;
    volumeSlider->setValue(vol, juce::dontSendNotification);
}

void TrackHeader::setMuted(bool mute)
{
    muted = mute;
    muteButton->setToggleState(mute, juce::dontSendNotification);
    muteButton->setButtonText(mute ? "M" : "M");
}

void TrackHeader::setSolo(bool soloState)
{
    solo = soloState;
    soloButton->setToggleState(soloState, juce::dontSendNotification);
    soloButton->setButtonText(solo ? "S" : "S");
}

void TrackHeader::setExpanded(bool expand)
{
    expanded = expand;
    expandButton->setButtonText(expanded ? "▼" : "▶");
}

juce::String TrackHeader::getTrackTypeIcon(TrackType type) const
{
    switch (type)
    {
        case TrackType::Sampler: return "🥁";
        case TrackType::Wavetable: return "🎹";
        case TrackType::Audio: return "🎤";
        case TrackType::Plugin: return "🎛";
        default: return "🎵";
    }
}

juce::String TrackHeader::getTrackTypeName(TrackType type) const
{
    switch (type)
    {
        case TrackType::Sampler: return "Sample";
        case TrackType::Wavetable: return "Synth";
        case TrackType::Audio: return "Audio";
        case TrackType::Plugin: return "Plugin";
        default: return "Unknown";
    }
}

juce::Colour TrackHeader::getTrackTypeColor(TrackType type) const
{
    switch (type)
    {
        case TrackType::Sampler: return getNeonPink();
        case TrackType::Wavetable: return getNeonCyan();
        case TrackType::Audio: return getNeonPurple();
        case TrackType::Plugin: return juce::Colours::green;
        default: return juce::Colours::white;
    }
}
