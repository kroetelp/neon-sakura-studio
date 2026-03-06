// ============================================================================
// FloatingPanelBase.cpp - Implementierung
// ============================================================================

#include "FloatingPanelBase.h"
#include "../Theme/ThemeManager.h"
#include "../DockingManager.h"

//==============================================================================
// Konstruktor
//==============================================================================
FloatingPanelBase::FloatingPanelBase()
{
    // Initialisierung
    setOpaque(false);
    setWantsKeyboardFocus(false);
}

//==============================================================================
// Destruktor
//==============================================================================
FloatingPanelBase::~FloatingPanelBase()
{
    // Stoppe alle laufenden Animationen
    stopAnimation();

    // Entferne den DockingManager-Referenz
    if (dockingManager != nullptr)
    {
        dockingManager = nullptr;
    }
}

//==============================================================================
// Panel State Management
//==============================================================================
void FloatingPanelBase::setPanelState(PanelState newState, bool animate)
{
    if (panelState == newState)
        return;

    PanelState oldState = panelState;
    panelState = newState;

    // Trigger virtuellen Callback
    onPanelStateChanged(oldState, newState);

    // Trigger Callback
    triggerPanelChangeCallback(newState, sizeMode);

    // Animation basierend auf dem neuen State
    switch (newState)
    {
        case PanelState::Hidden:
            if (animate)
                fadeOut(150);
            else
                setVisible(false);
            break;

        case PanelState::Minimized:
            // Bei Minimized nur Header sichtbar
            setPreferredSize(getWidth(), 40);
            if (animate)
                fadeIn(100);
            break;

        case PanelState::Floating:
        case PanelState::Snapped:
            if (animate)
                fadeIn(150);
            else
                setOpacity(1.0f);
            break;

        case PanelState::Collapsed:
            // Symbol-Größe
            setPreferredSize(50, 50);
            if (animate)
                fadeIn(100);
            break;
    }
}

void FloatingPanelBase::onPanelStateChanged(PanelState oldState, PanelState newState)
{
    // Standard-Implementierung: nichts tun
    // Abgeleitete Klassen können dies überschreiben
    juce::ignoreUnused(oldState, newState);
}

//==============================================================================
// Size Mode Management
//==============================================================================
void FloatingPanelBase::setSizeMode(PanelSizeMode newMode, bool animate)
{
    if (sizeMode == newMode)
        return;

    PanelSizeMode oldMode = sizeMode;
    sizeMode = newMode;

    // Trigger virtuellen Callback
    onSizeModeChanged(oldMode, newMode);

    // Trigger Callback
    triggerPanelChangeCallback(panelState, newMode);

    // Größe basierend auf dem neuen Mode anpassen
    switch (newMode)
    {
        case PanelSizeMode::Compact:
            // Klein, nur essentielle Controls
            setPreferredSize(300, 200);
            break;

        case PanelSizeMode::Standard:
            // Standard-Größe
            setPreferredSize(400, 300);
            break;

        case PanelSizeMode::Expanded:
            // Vollständige Features
            setPreferredSize(500, 400);
            break;

        case PanelSizeMode::Full:
            // Maximiert in Zone
            if (getParentComponent() != nullptr)
            {
                auto parentSize = getParentComponent()->getBounds();
                setPreferredSize(parentSize.getWidth(), parentSize.getHeight());
            }
            else
            {
                setPreferredSize(800, 600);
            }
            break;
    }

    if (animate)
        fadeIn(100);
}

void FloatingPanelBase::onSizeModeChanged(PanelSizeMode oldMode, PanelSizeMode newMode)
{
    // Standard-Implementierung: nichts tun
    // Abgeleitete Klassen können dies überschreiben
    juce::ignoreUnused(oldMode, newMode);
}

//==============================================================================
// Snap Zone Management
//==============================================================================
void FloatingPanelBase::setSnapZone(PanelSnapZone zone)
{
    snapZone = zone;

    // Wenn Zone nicht None, dann State auf Snapped setzen
    if (zone != PanelSnapZone::None && panelState != PanelState::Snapped)
    {
        setPanelState(PanelState::Snapped, true);
    }
    else if (zone == PanelSnapZone::None && panelState == PanelState::Snapped)
    {
        setPanelState(PanelState::Floating, true);
    }
}

//==============================================================================
// Animation
//==============================================================================
void FloatingPanelBase::fadeIn(int duration)
{
    if (currentOpacity >= 1.0f)
        return;

    targetOpacity = 1.0f;
    animationDuration = duration;
    startAnimation(duration);

    setVisible(true);
}

void FloatingPanelBase::fadeOut(int duration)
{
    if (currentOpacity <= 0.0f)
        return;

    targetOpacity = 0.0f;
    animationDuration = duration;
    startAnimation(duration);
}

void FloatingPanelBase::setOpacity(float opacity)
{
    currentOpacity = juce::jlimit(0.0f, 1.0f, opacity);
    setAlpha(currentOpacity);
}

void FloatingPanelBase::startAnimation(int duration)
{
    animationDuration = duration;
    animationStartTime = juce::Time::getMillisecondCounter();
    animationProgress = 0.0f;
    animating = true;
    startTimer(16); // ~60 FPS
}

void FloatingPanelBase::stopAnimation()
{
    animating = false;
    stopTimer();
    animationProgress = 0.0f;
}

void FloatingPanelBase::updateAnimation()
{
    if (!animating)
        return;

    int elapsed = juce::Time::getMillisecondCounter() - animationStartTime;
    animationProgress = juce::jlimit(0.0f, 1.0f, static_cast<float>(elapsed) / static_cast<float>(animationDuration));

    // Interpoliere Opacity
    float newOpacity = currentOpacity + (targetOpacity - currentOpacity) * 0.15f;
    setOpacity(newOpacity);

    // Prüfe ob Animation beendet
    if (animationProgress >= 1.0f || std::abs(currentOpacity - targetOpacity) < 0.01f)
    {
        setOpacity(targetOpacity);
        stopAnimation();

        // Wenn Fade-out beendet, Component verstecken
        if (targetOpacity <= 0.0f)
        {
            setVisible(false);
        }
    }
}

//==============================================================================
// Visibility Management
//==============================================================================
void FloatingPanelBase::showPanel()
{
    setPanelState(PanelState::Floating, true);
    setVisible(true);
    fadeIn(150);
}

void FloatingPanelBase::hidePanel()
{
    setPanelState(PanelState::Hidden, true);
}

void FloatingPanelBase::toggleVisibility()
{
    if (isHidden())
        showPanel();
    else
        hidePanel();
}

//==============================================================================
// Docking Support
//==============================================================================
void FloatingPanelBase::setDockingManager(DockingManager* manager)
{
    dockingManager = manager;
}

//==============================================================================
// Panel ID
//==============================================================================
void FloatingPanelBase::setPanelID(const juce::String& id)
{
    panelID = id;
}

//==============================================================================
// Panel Title
//==============================================================================
void FloatingPanelBase::setPanelTitle(const juce::String& title)
{
    panelTitle = title;
    repaint();
}

//==============================================================================
// Preferred Size
//==============================================================================
void FloatingPanelBase::setPreferredSize(int width, int height)
{
    preferredSize = juce::Rectangle<int>(0, 0, width, height);
    setSize(preferredSize.getWidth(), preferredSize.getHeight());
}

void FloatingPanelBase::setPreferredSize(const juce::Rectangle<int>& size)
{
    preferredSize = size;
    setSize(preferredSize.getWidth(), preferredSize.getHeight());
}

void FloatingPanelBase::setMinSize(int width, int height)
{
    minSize = juce::Rectangle<int>(0, 0, width, height);
}

void FloatingPanelBase::setMinSize(const juce::Rectangle<int>& size)
{
    minSize = size;
}

void FloatingPanelBase::setMaxSize(int width, int height)
{
    maxSize = juce::Rectangle<int>(0, 0, width, height);
}

void FloatingPanelBase::setMaxSize(const juce::Rectangle<int>& size)
{
    maxSize = size;
}

//==============================================================================
// Callbacks
//==============================================================================
void FloatingPanelBase::setPanelChangeCallback(PanelChangeCallback callback)
{
    panelChangeCallback = std::move(callback);
}

void FloatingPanelBase::triggerPanelChangeCallback(PanelState newState, PanelSizeMode newMode)
{
    if (panelChangeCallback)
    {
        panelChangeCallback(this, newState, newMode);
    }
}

//==============================================================================
// JUCE Component Overrides
//==============================================================================
void FloatingPanelBase::paint(juce::Graphics& g)
{
    // Aufruf der virtuellen Methode für abgeleitete Klassen
    paintPanelBackground(g);
}

void FloatingPanelBase::paintPanelBackground(juce::Graphics& g)
{
    auto bounds = getLocalBounds().toFloat();

    // Gradient Hintergrund
    juce::ColourGradient gradient(
        juce::Colour(30, 30, 35),      // Dunkles Grau-Blau oben
        0.0f, 0.0f,
        juce::Colour(20, 20, 25),      // Noch dunkler unten
        0.0f, bounds.getHeight(),
        false
    );

    g.setGradientFill(gradient);
    g.fillRoundedRectangle(bounds, 8.0f);

    // Subtiler Border
    g.setColour(juce::Colour(60, 60, 70).withAlpha(0.5f * currentOpacity));
    g.drawRoundedRectangle(bounds.reduced(0.5f), 8.0f, 1.0f);

    // Inner Glow
    juce::Path glowPath;
    glowPath.addRoundedRectangle(bounds.reduced(1.0f), 7.0f);

    juce::ColourGradient glowGradient(
        juce::Colours::white.withAlpha(0.03f),
        bounds.getCentreX(), bounds.getCentreY(),
        juce::Colours::transparentBlack,
        bounds.getCentreX(), bounds.getBottom(),
        true
    );

    g.setGradientFill(glowGradient);
    g.fillPath(glowPath);
}

void FloatingPanelBase::resized()
{
    // Aufruf der virtuellen Methode für abgeleitete Klassen
    layoutChildren();
}

void FloatingPanelBase::visibilityChanged()
{
    // Wird aufgerufen, wenn sich die Sichtbarkeit ändert
    Component::visibilityChanged();
}

void FloatingPanelBase::parentHierarchyChanged()
{
    // Wird aufgerufen, wenn die Parent-Hierarchie sich ändert
    Component::parentHierarchyChanged();
}

//==============================================================================
// JUCE Timer Callback
//==============================================================================
void FloatingPanelBase::timerCallback()
{
    updateAnimation();
}
