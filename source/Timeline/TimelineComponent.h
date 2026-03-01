#pragma once

#include "TimelineData.h"
#include "RecordingManager.h"
#include "../WavetableUI/NeonSakuraLookAndFeel.h"
#include "../SampleManager.h"
#include <juce_gui_basics/juce_gui_basics.h>
#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_audio_formats/juce_audio_formats.h>
#include <array>

class TimelineComponent : public juce::Component,
                          public juce::Timer,
                          public juce::ChangeListener,
                          public juce::FileDragAndDropTarget
{
public:
    TimelineComponent(TimelineData& data, RecordingManager& recordingMgr);
    ~TimelineComponent() override;

    void paint(juce::Graphics& g) override;
    void resized() override;
    void timerCallback() override;
    void changeListenerCallback(juce::ChangeBroadcaster* source) override;

    // Mouse interaction
    void mouseDown(const juce::MouseEvent& e) override;
    void mouseDrag(const juce::MouseEvent& e) override;
    void mouseUp(const juce::MouseEvent& e) override;
    void mouseWheelMove(const juce::MouseEvent& e, const juce::MouseWheelDetails& wheel) override;

    // Zoom
    void setHorizontalZoom(double zoom);
    void setVerticalZoom(double zoom);
    double getHorizontalZoom() const { return horizontalZoom; }

    // Viewport
    void scrollToBeat(double beat);
    void scrollToTrack(int trackIndex);

    // Selection
    void selectClip(const juce::Uuid& clipId);
    void clearSelection();
    juce::Uuid getSelectedClip() const { return selectedClipId; }

    // Actions
    void deleteSelectedClip();
    void copySelectedClip();
    void pasteClip();

    // Recording
    void armTrack(int trackIndex, bool armed);
    void startRecordingOnArmedTrack();
    void stopRecording();
    void toggleRecording();
    void toggleLoop();

    // Sample Manager
    void setSampleManager(SampleManager* manager) { sampleManager = manager; }

    // === FileDragAndDropTarget ===
    bool isInterestedInFileDrag(const juce::StringArray& files) override;
    void fileDragEnter(const juce::StringArray& files, int x, int y) override;
    void fileDragExit(const juce::StringArray& files) override;
    void filesDropped(const juce::StringArray& files, int x, int y) override;

private:
    TimelineData& timelineData;
    RecordingManager& recordingManager;
    SampleManager* sampleManager = nullptr;

    // Audio format manager for sample loading
    juce::AudioFormatManager formatManager;

    // Toolbar components
    juce::TextButton recordButton;
    juce::TextButton loopButton;
    juce::TextButton deleteButton;
    juce::TextButton importSampleButton;
    juce::Label zoomLabel;
    juce::Slider zoomSlider;
    juce::Label bpmLabel;

    // Track control buttons (Mute/Solo/Arm)
    std::array<std::unique_ptr<juce::TextButton>, TimelineData::numTracks> muteButtons;
    std::array<std::unique_ptr<juce::TextButton>, TimelineData::numTracks> soloButtons;
    std::array<std::unique_ptr<juce::TextButton>, TimelineData::numTracks> armButtons;

    // Drag state for samples
    bool isDraggingFiles = false;
    int dragOverTrackIndex = -1;
    double dragOverBeat = 0.0;

    // Zoom levels
    double horizontalZoom{1.0};  // Pixels per beat
    double verticalZoom{1.0};

    // Scroll position
    double scrollBeat{0.0};
    int scrollTrack{0};

    // Selection
    juce::Uuid selectedClipId;
    int selectedTrackIndex{-1};

    // Drag state
    enum class DragMode { None, MoveClip, ResizeClipLeft, ResizeClipRight, CreateClip };
    DragMode currentDragMode{DragMode::None};
    juce::Uuid draggedClipId;
    double dragStartBeat{0.0};
    double dragOriginalStartBeat{0.0};
    double dragOriginalLengthBeats{0.0};
    juce::Point<int> dragStartMouse;

    // Layout constants
    static constexpr int trackHeight = 80;
    static constexpr int timeRulerHeight = 30;
    static constexpr int trackControlsWidth = 120;
    static constexpr int toolbarHeight = 40;

    // Helper methods
    void initializeToolbar();
    void initializeTrackControls();
    void updateToolbarState();
    void updateTrackControlButtonStates(int trackIndex);

    int beatToX(double beat) const;
    double xToBeat(int x) const;
    int trackToY(int trackIndex) const;
    int yToTrack(int y) const;

    juce::Rectangle<int> getClipBounds(const TimelineClip& clip, int trackIndex) const;
    juce::Rectangle<int> getTrackBounds(int trackIndex) const;
    juce::Rectangle<int> getTimelineArea() const;
    juce::Rectangle<int> getToolbarArea() const;

    void drawTimeRuler(juce::Graphics& g);
    void drawTrack(juce::Graphics& g, int trackIndex);
    void drawClip(juce::Graphics& g, const TimelineClip& clip, int trackIndex, bool isSelected);
    void drawPlayhead(juce::Graphics& g);
    void drawTrackControls(juce::Graphics& g, int trackIndex);
    void drawWaveformPreview(juce::Graphics& g, const TimelineClip& clip,
                             const juce::Rectangle<int>& bounds);
    void drawMidiPreview(juce::Graphics& g, const TimelineClip& clip,
                         const juce::Rectangle<int>& bounds);

    // Sample loading
    void loadSampleIntoTimeline(const juce::File& sampleFile, int trackIndex, double startBeat);
    void showTrackContextMenu(int trackIndex, const juce::Point<int>& position);

    // Colors (Neon Sakura theme)
    juce::Colour getTrackColor(int trackIndex) const;
    juce::Colour getPlayheadColor() const { return juce::Colour(0xff00ffff); } // Cyan
    juce::Colour getSelectionColor() const { return juce::Colour(0xffffff00); } // Yellow
};
