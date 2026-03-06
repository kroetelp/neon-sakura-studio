// ============================================================================
// EnhancedPanelHeader.cpp - Implementierung
// ============================================================================

#include "EnhancedPanelHeader.h"
#include "FloatingPanelBase.h"
#include "../Theme/ThemeManager.h"

//==============================================================================
// Konstruktor
//==============================================================================
EnhancedPanelHeader::EnhancedPanelHeader(const juce::String& title)
    : title(title)
{
    setInterceptsMouseClicks(true, true);
    setRepaintsOnMouseActivity(true);
}

//==============================================================================
// Destruktor
//==============================================================================
EnhancedPanelHeader::~EnhancedPanelHeader()
{
    stopTimer();
}

//==============================================================================
// Header Actions
//==============================================================================
void EnhancedPanelHeader::setTitle(const juce::String& newTitle)
{
    title = newTitle;
    repaint();
}

//==============================================================================
// Panel Actions
//==============================================================================
void EnhancedPanelHeader::setCloseEnabled(bool enabled)
{
    closeEnabled = enabled;
    setCloseButtonVisible(enabled);
    repaint();
}

void EnhancedPanelHeader::setMinimizeEnabled(bool enabled)
{
    minimizeEnabled = enabled;
    setMinimizeButtonVisible(enabled);
    repaint();
}

void EnhancedPanelHeader::setMaximizeEnabled(bool enabled)
{
    maximizeEnabled = enabled;
    setMaximizeButtonVisible(enabled);
    repaint();
}

void EnhancedPanelHeader::setPanelState(PanelState state)
{
    if (panelState == state)
        return;

    PanelState oldState = panelState;
    panelState = state;

    // Trigger virtuellen Callback
    onPanelStateChanged(state);

    // Update UI basierend auf State
    switch (state)
    {
        case PanelState::Minimized:
            // Bei minimized nur Header sichtbar, aber Buttons deaktivieren
            setMaximizeButtonVisible(true);
            break;

        case PanelState::Floating:
        case PanelState::Snapped:
            // Standard-Buttons
            setCloseButtonVisible(true);
            setMinimizeButtonVisible(true);
            setMaximizeButtonVisible(true);
            break;

        case PanelState::Hidden:
            break;

        case PanelState::Collapsed:
            // Minimal UI
            break;
    }

    repaint();
}

//==============================================================================
// Drag & Drop
//==============================================================================
void EnhancedPanelHeader::setDragEnabled(bool enabled)
{
    dragEnabled = enabled;
}

//==============================================================================
// Callbacks
//==============================================================================
void EnhancedPanelHeader::setHeaderActionCallback(HeaderActionCallback callback)
{
    headerActionCallback = std::move(callback);
}

//==============================================================================
// Visual Appearance
//==============================================================================
void EnhancedPanelHeader::setHeaderHeight(int height)
{
    headerHeight = juce::jlimit(30, 100, height);
    if (getParentComponent() != nullptr)
    {
        getParentComponent()->setSize(getParentComponent()->getWidth(), height);
    }
}

void EnhancedPanelHeader::setUseGradientBackground(bool useGradient)
{
    useGradientBackground = useGradient;
    repaint();
}

//==============================================================================
// Button Visibility
//==============================================================================
void EnhancedPanelHeader::setCloseButtonVisible(bool visible)
{
    closeButtonVisible = visible;
    layoutButtons();
    repaint();
}

void EnhancedPanelHeader::setMinimizeButtonVisible(bool visible)
{
    minimizeButtonVisible = visible;
    layoutButtons();
    repaint();
}

void EnhancedPanelHeader::setMaximizeButtonVisible(bool visible)
{
    maximizeButtonVisible = visible;
    layoutButtons();
    repaint();
}

//==============================================================================
// JUCE Component Overrides
//==============================================================================
void EnhancedPanelHeader::paint(juce::Graphics& g)
{
    paintBackground(g);
    paintTitle(g);

    // Zeichne Buttons
    if (closeButtonVisible)
        paintButton(g, closeButtonBounds, "×", closeButtonHovered);

    if (minimizeButtonVisible)
        paintButton(g, minimizeButtonBounds, "−", minimizeButtonHovered);

    if (maximizeButtonVisible)
        paintButton(g, maximizeButtonBounds, "□", maximizeButtonHovered);
}

void EnhancedPanelHeader::paintBackground(juce::Graphics& g)
{
    auto bounds = getLocalBounds().toFloat();

    if (useGradientBackground)
    {
        // Gradient Hintergrund
        juce::ColourGradient gradient(
            juce::Colour(45, 45, 55),      // Dunkel oben
            0.0f, 0.0f,
            juce::Colour(30, 30, 40),      // Dunkler unten
            0.0f, bounds.getHeight(),
            false
        );
        g.setGradientFill(gradient);
    }
    else
    {
        g.setColour(juce::Colour(40, 40, 50));
    }

    g.fillRoundedRectangle(bounds, 6.0f);

    // Bottom Border
    g.setColour(juce::Colour(60, 60, 70).withAlpha(0.5f));
    g.drawHorizontalLine(bounds.getHeight() - 1, 0, bounds.getWidth());
}

void EnhancedPanelHeader::paintTitle(juce::Graphics& g)
{
    g.setColour(juce::Colours::white.withAlpha(0.9f));
    g.setFont(juce::Font(14.0f, juce::Font::bold));

    auto textBounds = titleBounds.toFloat();
    g.drawText(title, textBounds, juce::Justification::centredLeft, true);
}

void EnhancedPanelHeader::paintButton(juce::Graphics& g, const juce::Rectangle<int>& bounds, const juce::String& symbol, bool hovered)
{
    if (bounds.isEmpty())
        return;

    auto buttonBounds = bounds.toFloat();

    // Button Hintergrund
    if (hovered)
    {
        juce::ColourGradient gradient(
            juce::Colour(80, 80, 100),
            buttonBounds.getCentreX(), buttonBounds.getCentreY(),
            juce::Colour(60, 60, 80),
            buttonBounds.getCentreX(), buttonBounds.getBottom(),
            true
        );
        g.setGradientFill(gradient);
        g.fillRoundedRectangle(buttonBounds, 4.0f);
    }

    // Symbol
    g.setColour(hovered ? juce::Colours::white : juce::Colour(180, 180, 190));
    g.setFont(juce::Font(16.0f, juce::Font::bold));
    g.drawText(symbol, buttonBounds, juce::Justification::centred);
}

void EnhancedPanelHeader::resized()
{
    layoutButtons();
}

void EnhancedPanelHeader::layoutButtons()
{
    auto bounds = getLocalBounds();
    int buttonSize = headerHeight - 8;
    int buttonSpacing = 4;
    int rightMargin = 8;

    // Buttons auf der rechten Seite
    auto buttonArea = getButtonArea();

    // Close Button
    if (closeButtonVisible)
    {
        closeButtonBounds = juce::Rectangle<int>(
            buttonArea.getRight() - buttonSize,
            (buttonArea.getHeight() - buttonSize) / 2,
            buttonSize, buttonSize
        );
        buttonArea.setWidth(buttonArea.getWidth() - buttonSize - buttonSpacing);
    }
    else
    {
        closeButtonBounds = juce::Rectangle<int>();
    }

    // Maximize Button
    if (maximizeButtonVisible)
    {
        maximizeButtonBounds = juce::Rectangle<int>(
            buttonArea.getRight() - buttonSize,
            (buttonArea.getHeight() - buttonSize) / 2,
            buttonSize, buttonSize
        );
        buttonArea.setWidth(buttonArea.getWidth() - buttonSize - buttonSpacing);
    }
    else
    {
        maximizeButtonBounds = juce::Rectangle<int>();
    }

    // Minimize Button
    if (minimizeButtonVisible)
    {
        minimizeButtonBounds = juce::Rectangle<int>(
            buttonArea.getRight() - buttonSize,
            (buttonArea.getHeight() - buttonSize) / 2,
            buttonSize, buttonSize
        );
    }
    else
    {
        minimizeButtonBounds = juce::Rectangle<int>();
    }

    // Title Bereich
    titleBounds = getTitleArea();
}

juce::Rectangle<int> EnhancedPanelHeader::getButtonArea() const
{
    auto bounds = getLocalBounds();
    int buttonAreaWidth = headerHeight * 3 + 20; // Platz für 3 Buttons
    return bounds.removeFromRight(buttonAreaWidth);
}

juce::Rectangle<int> EnhancedPanelHeader::getTitleArea() const
{
    auto bounds = getLocalBounds();
    int buttonAreaWidth = headerHeight * 3 + 20;
    bounds.removeFromRight(buttonAreaWidth);
    bounds.reduce(12, 0);
    return bounds;
}

void EnhancedPanelHeader::updateButtonHoverStates(const juce::Point<int>& mousePos)
{
    bool oldCloseHovered = closeButtonHovered;
    bool oldMinimizeHovered = minimizeButtonHovered;
    bool oldMaximizeHovered = maximizeButtonHovered;

    closeButtonHovered = closeButtonBounds.contains(mousePos);
    minimizeButtonHovered = minimizeButtonBounds.contains(mousePos);
    maximizeButtonHovered = maximizeButtonBounds.contains(mousePos);

    if (closeButtonHovered != oldCloseHovered ||
        minimizeButtonHovered != oldMinimizeHovered ||
        maximizeButtonHovered != oldMaximizeHovered)
    {
        repaint();
    }
}

void EnhancedPanelHeader::triggerHeaderAction(HeaderAction action)
{
    // Trigger virtuellen Callback
    onHeaderAction(action);

    // Trigger Callback
    if (headerActionCallback)
    {
        headerActionCallback(action);
    }
}

void EnhancedPanelHeader::mouseDown(const juce::MouseEvent& e)
{
    // Prüfe ob auf einen Button geklickt wurde
    if (closeButtonHovered && closeEnabled)
    {
        triggerHeaderAction(HeaderAction::Close);
        return;
    }

    if (minimizeButtonHovered && minimizeEnabled)
    {
        triggerHeaderAction(HeaderAction::Minimize);
        return;
    }

    if (maximizeButtonHovered && maximizeEnabled)
    {
        triggerHeaderAction(HeaderAction::Maximize);
        return;
    }

    // Drag start
    if (dragEnabled && e.mods.isLeftButtonDown())
    {
        isDragging = true;
        dragStartPos = e.getMouseDownPosition();
        triggerHeaderAction(HeaderAction::DragStart);
    }
}

void EnhancedPanelHeader::mouseDrag(const juce::MouseEvent& e)
{
    updateButtonHoverStates(e.getPosition());
}

void EnhancedPanelHeader::mouseUp(const juce::MouseEvent& e)
{
    updateButtonHoverStates(e.getPosition());

    if (isDragging)
    {
        isDragging = false;
        triggerHeaderAction(HeaderAction::DragEnd);
    }
}

void EnhancedPanelHeader::mouseDoubleClick(const juce::MouseEvent& e)
{
    triggerHeaderAction(HeaderAction::DoubleClick);
}

//==============================================================================
// Protected Methods
//==============================================================================
void EnhancedPanelHeader::onPanelStateChanged(PanelState newState)
{
    juce::ignoreUnused(newState);
}

void EnhancedPanelHeader::onHeaderAction(HeaderAction action)
{
    juce::ignoreUnused(action);
}

//==============================================================================
// JUCE Timer Callback
//==============================================================================
void EnhancedPanelHeader::timerCallback()
{
    // Animation für Hover-Effekte
    if (hovering)
    {
        hoverAlpha += 0.05f;
        if (hoverAlpha >= 1.0f)
        {
            hoverAlpha = 1.0f;
            stopTimer();
        }
    }
    else
    {
        hoverAlpha -= 0.05f;
        if (hoverAlpha <= 0.0f)
        {
            hoverAlpha = 0.0f;
            stopTimer();
        }
    }

    repaint();
}

//==============================================================================
// Mouse Enter/Exit für Hover-Effekte
//==============================================================================
void EnhancedPanelHeader::mouseEnter(const juce::MouseEvent& e)
{
    Component::mouseEnter(e);
    hovering = true;
    startTimer(16); // ~60 FPS
}

void EnhancedPanelHeader::mouseExit(const juce::MouseEvent& e)
{
    Component::mouseExit(e);
    hovering = false;
    closeButtonHovered = false;
    minimizeButtonHovered = false;
    maximizeButtonHovered = false;
    repaint();
}
