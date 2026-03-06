#pragma once

/**
 * MasterBusPanel - Master-Ausgangs-Steuerung und Metering
 *
 * Features:
 * - Stereo Level Meter (L/R-Kanäle)
 * - Loudness Meter (LUFS, EBU R128)
 * - Master Controls: Volume, Pan, Mute
 * - Export/Render Buttons
 * - Master Effects Section
 */

#include <juce_gui_basics/juce_gui_basics.h>
#include <memory>
#include <functional>
#include <vector>
#include <deque>
#include "../DockablePanel.h"

// Forward declarations
class AudioRoutingGraph;

// ============================================================================
// Loudness Meter Types
// ============================================================================

enum class LoudnessMode
{
    LUFS,       // LUFS (EBU R128)
    EBU_R128,   // EBU R128 loudness units
    TruePeak,   // True Peak dB
    FalsePeak   // False Peak dB
};

// ============================================================================
/**
 * MasterBusPanel - Master-Ausgangs
 */
class MasterBusPanel : public DockablePanel, private juce::Timer
{
public:
    MasterBusPanel(AudioRoutingGraph& graph);
    ~MasterBusPanel() override;

    // ============================================================
    // Metering
    // ============================================================

    void setLoudnessMode(LoudnessMode mode);
    LoudnessMode getLoudnessMode() const { return currentLoudnessMode; }

    void setMeterIntegrationTime(float seconds);  // Standard: 0.4s

    // ============================================================
    // Master Controls
    // ============================================================

    void setMasterVolume(float volume);  // 0.0 - 1.0
    float getMasterVolume() const { return masterVolume; }

    void setMasterPan(float pan);  // -1.0 (links) bis 1.0 (rechts)
    float getMasterPan() const { return masterPan; }

    void setMasterMute(bool muted);
    bool isMasterMuted() const { return masterMuted; }

    void toggleMasterMute();

    // ============================================================
    // Export/Render
    // ============================================================

    std::function<void()> onExportClicked;
    std::function<void()> onRenderClicked;

    void setExportFormat(const juce::String& format);  // WAV, MP3, FLAC, etc.
    void setSampleRate(int sampleRate);  // 44100, 48000, 96000, etc.
    void setBitDepth(int bits);  // 16, 24, 32

    // ============================================================
    // Master Effects
    // ============================================================

    void setMasterEffectsEnabled(bool enabled);
    bool isMasterEffectsEnabled() const { return masterEffectsEnabled; }

    // ============================================================
    // Real-time Data Access
    // ============================================================

    float getLeftLevel() const { return leftLevel; }
    float getRightLevel() const { return rightLevel; }
    float getLoudnessLUFS() const { return loudnessLUFS; }
    float getTruePeak() const { return truePeak; }
    float getFalsePeak() const { return falsePeak; }

    // ============================================================
    // Callbacks für Audio-Updates
    // ============================================================

    void setLevelUpdateCallback(std::function<void(float, float)> callback);
    void setLoudnessUpdateCallback(std::function<void(float)> callback);

    // ============================================================
    // Overrides
    // ============================================================

    void paint(juce::Graphics& g) override;
    void resized() override;
    void timerCallback() override;

protected:
    void initializeContent() override;

private:
    // ============================================================
    // Audio Graph Reference
    // ============================================================

    AudioRoutingGraph& audioGraph;

    // ============================================================
    // Metering State
    // ============================================================

    LoudnessMode currentLoudnessMode = LoudnessMode::LUFS;
    float integrationTime = 0.4f;

    // Level-Daten (exponential smoothing)
    float leftLevel = 0.0f;
    float rightLevel = 0.0f;
    float peakHoldTime = 0.0f;  // Zeit seit letztem Peak
    float leftPeak = 0.0f;
    float rightPeak = 0.0f;

    // Loudness-Daten
    float loudnessLUFS = -60.0f;
    float truePeak = -60.0f;
    float falsePeak = -60.0f;
    std::vector<float> loudnessHistory;  // Für gleitenden Durchschnitt

    // ============================================================
    // Master Controls State
    // ============================================================

    float masterVolume = 0.8f;
    float masterPan = 0.0f;
    bool masterMuted = false;

    // ============================================================
    // Export/Render State
    // ============================================================

    juce::String exportFormat = "WAV";
    int sampleRate = 44100;
    int bitDepth = 24;

    bool isExporting = false;
    float renderProgress = 0.0f;

    // ============================================================
    // Master Effects
    // ============================================================

    bool masterEffectsEnabled = false;

    // ============================================================
    // Callbacks
    // ============================================================

    std::function<void(float, float)> levelUpdateCallback;
    std::function<void(float)> loudnessUpdateCallback;

    // ============================================================
    // Layout Konstanten
    // ============================================================

    static constexpr int headerHeight = 40;
    static constexpr int meterWidth = 30;
    static constexpr int meterHeight = 200;
    static constexpr int controlsHeight = 120;

    static constexpr float minDB = -60.0f;
    static constexpr float maxDB = 0.0f;

    static constexpr float targetLUFS = -14.0f;  // EBU R128 Integrated

    // ============================================================
    // Level History (für smoothing)
    // ============================================================

    static constexpr int historySize = 100;
    std::deque<float> leftLevelHistory;
    std::deque<float> rightLevelHistory;

    // ============================================================
    // Helpers
    // ============================================================

    void updateLevels(float newLeft, float newRight);
    void drawStereoMeter(juce::Graphics& g, juce::Rectangle<float> bounds, float left, float right);
    void drawLoudnessMeter(juce::Graphics& g, juce::Rectangle<float> bounds, float loudness);
    void drawMasterControls(juce::Graphics& g, juce::Rectangle<float> bounds);
    void drawExportControls(juce::Graphics& g, juce::Rectangle<float> bounds);
    void drawMasterEffects(juce::Graphics& g, juce::Rectangle<float> bounds);

    float dbToY(float db, juce::Rectangle<float> meterArea) const;
    juce::String dbToString(float db) const;

    void resetLevels();
    void resetLoudness();

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MasterBusPanel)
};
