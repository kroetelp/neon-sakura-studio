#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include <memory>
#include "../Theme/ThemeManager.h"
#include "../TrackType.h"

// Forward declarations
class TrackComponent;
class AudioRoutingGraph;

/**
 * TrackHeader - Unified Header für alle Track-Typen
 *
 * Konsistente Track-Controls für:
 *   - Sample/Drum Tracks
 *   - Synth Tracks
 *   - Audio Tracks
 *   - MIDI Tracks
 *
 * Features:
 *   - Track-Typ Switcher (Dropdown)
 *   - Plugin-Slot Indicators (visuelle Darstellung geladener Plugins)
 *   - Routing-Visualisierung (Sidechain/MIDI)
 *   - Volume/Mute/Solo Controls
 */
class TrackHeader : public juce::Component
{
public:
    TrackHeader(int trackIndex);
    ~TrackHeader() override;

    void resized() override;
    void paint(juce::Graphics& g) override;

    // Track type management
    void setTrackType(TrackType type);
    TrackType getTrackType() const { return currentTrackType; }

    // Plugin indicators
    void setPluginCount(int count);
    int getPluginCount() const { return pluginCount; }

    // Routing indicators
    void setHasSidechainSource(bool hasSource);
    void setHasSidechainTarget(bool hasTarget);
    void setHasMIDIRoute(bool hasRoute);

    // Track name
    void setTrackName(const juce::String& name);
    juce::String getTrackName() const;

    // Volume/Mute/Solo controls
    void setVolume(float volume);
    float getVolume() const { return volume; }
    void setMuted(bool muted);
    bool getMuted() const { return muted; }
    void setSolo(bool solo);
    bool getSolo() const { return solo; }

    // Expand/Collapse
    void setExpanded(bool expanded);
    bool isExpanded() const { return expanded; }

    // Callbacks
    std::function<void(TrackType)> onTrackTypeChanged;
    std::function<void(float)> onVolumeChanged;
    std::function<void(bool)> onMuteChanged;
    std::function<void(bool)> onSoloChanged;
    std::function<void()> onExpandToggled;
    std::function<void()> onPluginButtonClicked;

private:
    int trackIndex;
    TrackType currentTrackType = TrackType::Sampler;
    int pluginCount = 0;
    float volume = 0.8f;
    bool muted = false;
    bool solo = false;
    bool expanded = true;

    bool hasSidechainSource = false;
    bool hasSidechainTarget = false;
    bool hasMIDIRoute = false;

    // UI Components
    std::unique_ptr<juce::TextButton> trackTypeButton;
    std::unique_ptr<juce::ComboBox> trackTypeCombo;
    std::unique_ptr<juce::Label> trackNameLabel;
    std::unique_ptr<juce::Slider> volumeSlider;
    std::unique_ptr<juce::TextButton> muteButton;
    std::unique_ptr<juce::TextButton> soloButton;
    std::unique_ptr<juce::TextButton> expandButton;
    std::unique_ptr<juce::TextButton> pluginButton;

    // Plugin indicators (small dots)
    static constexpr int maxPluginIndicators = 8;
    std::array<bool, maxPluginIndicators> pluginSlots{false};

    void initializeComponents();
    void layoutComponents();
    void setupListeners();

    // Get icon for track type
    juce::String getTrackTypeIcon(TrackType type) const;
    juce::String getTrackTypeName(TrackType type) const;
    juce::Colour getTrackTypeColor(TrackType type) const;

    juce::Colour getNeonPink() const { return ThemeManager::getInstance().getAccentColor(); }
    juce::Colour getNeonCyan() const { return ThemeManager::getInstance().getInfoColor(); }
    juce::Colour getNeonPurple() const { return ThemeManager::getInstance().getAccentColor().withHue(0.8f); }
    juce::Colour getDarkBackground() const { return ThemeManager::getInstance().getPanelBackgroundColor(); }

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(TrackHeader)
};
