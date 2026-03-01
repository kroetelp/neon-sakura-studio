// ============================================================================
// NeonStepButton.cpp - Implementierung
// ============================================================================

#include "NeonStepButton.h"

NeonStepButton::NeonStepButton()
{
    setInterceptsMouseClicks(true, true);
    setRepaintsOnMouseActivity(true);
}

NeonStepButton::~NeonStepButton() = default;

void NeonStepButton::setActive(bool isActive, bool notify)
{
    if (active != isActive)
    {
        active = isActive;
        repaint();

        if (notify && onStateChanged)
            onStateChanged(active);
    }
}

void NeonStepButton::toggle()
{
    setActive(!active, true);
}

void NeonStepButton::setActiveColour(const juce::Colour& colour)
{
    activeColour = colour;
    repaint();
}

void NeonStepButton::setInactiveColour(const juce::Colour& colour)
{
    inactiveColour = colour;
    repaint();
}

void NeonStepButton::setGlowColour(const juce::Colour& colour)
{
    glowColour = colour;
    repaint();
}

void NeonStepButton::setDragOverState(bool isDragOver)
{
    if (dragOver != isDragOver)
    {
        dragOver = isDragOver;
        repaint();
    }
}

void NeonStepButton::paint(juce::Graphics& g)
{
    auto bounds = getLocalBounds().toFloat().reduced(2.0f);

    // === Hintergrund ===
    if (active)
    {
        // Aktiver State mit Glow
        drawGlowEffect(g, bounds);

        // Gradient für aktiven Step
        auto gradient = juce::ColourGradient::vertical(
            activeColour.brighter(0.3f),
            bounds.getY(),
            activeColour.darker(0.2f),
            bounds.getBottom()
        );
        g.setGradientFill(gradient);
        g.fillRoundedRectangle(bounds, cornerRadius);

        // Heller Innenbereich (Subtle)
        auto innerBounds = bounds.reduced(3.0f);
        g.setColour(activeColour.brighter(0.5f).withAlpha(0.3f));
        g.fillRoundedRectangle(innerBounds, cornerRadius - 1.0f);
    }
    else
    {
        // Inaktiver State
        g.setColour(inactiveColour);
        g.fillRoundedRectangle(bounds, cornerRadius);

        // Subtiler Rahmen
        g.setColour(dragOver ? borderColor.brighter(0.5f) : borderColor);
        g.drawRoundedRectangle(bounds, cornerRadius, 1.0f);

        // Drag-Over Highlight
        if (dragOver)
        {
            g.setColour(activeColour.withAlpha(0.2f));
            g.fillRoundedRectangle(bounds, cornerRadius);
        }
    }

    // === Akzent-Linie unten (wenn aktiv) ===
    if (active)
    {
        auto bottomLine = bounds.removeFromBottom(3.0f);
        g.setColour(activeColour.brighter(0.8f));
        g.fillRoundedRectangle(bottomLine, 1.0f);
    }
}

void NeonStepButton::drawGlowEffect(juce::Graphics& g, const juce::Rectangle<float>& bounds)
{
    // Äußerer Glow
    auto glowBounds = bounds.expanded(4.0f);
    g.setColour(glowColour.withAlpha(0.15f));
    g.fillRoundedRectangle(glowBounds, cornerRadius + 2.0f);

    // Mittlerer Glow
    auto midGlowBounds = bounds.expanded(2.0f);
    g.setColour(glowColour.withAlpha(0.25f));
    g.fillRoundedRectangle(midGlowBounds, cornerRadius + 1.0f);
}

void NeonStepButton::mouseDown(const juce::MouseEvent& e)
{
    if (e.mods.isLeftButtonDown())
    {
        toggle();
    }
}

void NeonStepButton::mouseEnter(const juce::MouseEvent& e)
{
    // Wenn die Maus bei gedrückter Taste eintritt, feuere Drag-Callback
    if (e.mods.isLeftButtonDown() && onDragEnter)
    {
        onDragEnter(this, true);
    }
}

void NeonStepButton::mouseDrag(const juce::MouseEvent& /*e*/)
{
    // Drag wird vom Parent (StepSequencerPanel) gehandhabt
    // Hier nur sicherstellen, dass wir Drag-Events empfangen
}
