// ============================================================================
// NeonStepButton.h - Custom Step Button mit Drag-to-Draw Support
// ============================================================================
//
// Ein einzelner Step im Step-Sequencer mit:
// - Abgerundeten Ecken
// - Neon Glow Effekt wenn aktiv
// - Drag-to-Draw Support (Maus gedrückt halten und wischen)

#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include <functional>

/**
 * NeonStepButton - Ein einzelner Step im Sequencer
 *
 * Features:
 * - Aktiv/Inaktiv State mit Neon Glow
 * - Drag-to-Draw: Reagiert auf Mouse-Drag Events
 * - Callback für State-Änderungen
 */
class NeonStepButton : public juce::Component
{
public:
    NeonStepButton();
    ~NeonStepButton() override;

    // === State ===
    bool isActive() const { return active; }
    void setActive(bool isActive, bool notify = true);
    void toggle();

    // === Farben ===
    void setActiveColour(const juce::Colour& colour);
    void setInactiveColour(const juce::Colour& colour);
    void setGlowColour(const juce::Colour& colour);

    // === Selection State für Drag ===
    void setDragOverState(bool isDragOver);

    // === Callbacks ===
    std::function<void(bool newState)> onStateChanged;
    std::function<void(NeonStepButton*, bool isEnter)> onDragEnter;  // Für Drag-to-Draw

    // === Component Override ===
    void paint(juce::Graphics& g) override;
    void mouseDown(const juce::MouseEvent& e) override;
    void mouseEnter(const juce::MouseEvent& e) override;
    void mouseDrag(const juce::MouseEvent& e) override;

    // === Static Design Constants ===
    static constexpr float cornerRadius = 4.0f;
    static constexpr int defaultSize = 36;

private:
    bool active = false;
    bool dragOver = false;

    juce::Colour activeColour{255, 20, 147};      // Neon Pink
    juce::Colour inactiveColour{35, 35, 50};      // Dunkles Anthrazit
    juce::Colour glowColour{255, 20, 147};        // Glow Farbe
    juce::Colour borderColor{60, 60, 80};         // Subtiler Rahmen

    void drawGlowEffect(juce::Graphics& g, const juce::Rectangle<float>& bounds);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(NeonStepButton)
};
