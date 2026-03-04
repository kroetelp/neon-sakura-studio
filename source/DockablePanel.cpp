// ============================================================================
// DockablePanel.cpp - Implementierung der Basisklasse
// ============================================================================

#include "DockablePanel.h"
#include "Theme/ThemeManager.h"

// ============================================================================
// PanelHeader Implementation
// ============================================================================

PanelHeader::PanelHeader(const juce::String& name)
    : panelName(name)
{
    auto& theme = ThemeManager::getInstance();

    // Title Label
    titleLabel.setText(panelName, juce::dontSendNotification);
    titleLabel.setColour(juce::Label::textColourId, theme.getTextSecondaryColor());
    titleLabel.setFont(juce::Font(14.0f, juce::Font::bold));
    titleLabel.setJustificationType(juce::Justification::centredLeft);
    addAndMakeVisible(titleLabel);

    // Setup Buttons
    setupButton(undockButton, "Undock - Open in separate window");
    setupButton(closeButton, "Close Panel");

    undockButton.setButtonText(juce::CharPointer_UTF8("\xe2\x97\xbb"));  // ◻ white square
    closeButton.setButtonText(juce::CharPointer_UTF8("\xc3\x97"));       // × multiplication sign
}

void PanelHeader::setupButton(juce::TextButton& btn, const juce::String& tooltip)
{
    auto& theme = ThemeManager::getInstance();

    btn.setTooltip(tooltip);
    btn.setColour(juce::TextButton::buttonColourId, juce::Colours::transparentBlack);
    btn.setColour(juce::TextButton::textColourOffId, theme.getTextSecondaryColor());
    btn.setColour(juce::TextButton::textColourOnId, theme.getInfoColor());  // Accent on hover
    btn.setColour(juce::ComboBox::outlineColourId, juce::Colours::transparentBlack);

    // Button Style
    btn.onClick = [this, &btn]()
    {
        if (&btn == &undockButton && onUndockClicked)
            onUndockClicked();
        else if (&btn == &closeButton && onCloseClicked)
            onCloseClicked();
    };

    addAndMakeVisible(btn);
}

void PanelHeader::paint(juce::Graphics& g)
{
    auto& theme = ThemeManager::getInstance();

    // Header Background mit subtilem Gradient
    auto bounds = getLocalBounds();
    auto bgColor = theme.getPanelHeaderColor();
    auto gradient = juce::ColourGradient::vertical(
        bgColor.brighter(0.05f),
        bounds.toFloat().getY(),
        bgColor,
        bounds.toFloat().getBottom()
    );
    g.setGradientFill(gradient);
    g.fillRect(bounds);

    // Bottom Border (accent line)
    g.setColour(theme.getAccentColor().withAlpha(0.5f));
    g.fillRect(bounds.getX(), bounds.getBottom() - 1, bounds.getWidth(), 1);
}

void PanelHeader::resized()
{
    auto bounds = getLocalBounds().reduced(4, 2);

    const int buttonWidth = 28;
    const int buttonHeight = bounds.getHeight() - 2;

    // Close Button (rechts)
    if (showClose)
    {
        closeButton.setBounds(bounds.getRight() - buttonWidth,
                               bounds.getY() + 1,
                               buttonWidth,
                               buttonHeight);
        bounds.removeFromRight(buttonWidth + 4);
    }
    else
    {
        closeButton.setVisible(false);
    }

    // Undock Button (neben Close)
    if (showUndock)
    {
        undockButton.setBounds(bounds.getRight() - buttonWidth,
                                bounds.getY() + 1,
                                buttonWidth,
                                buttonHeight);
        bounds.removeFromRight(buttonWidth + 4);
    }
    else
    {
        undockButton.setVisible(false);
    }

    // Title Label (restlicher Bereich)
    titleLabel.setBounds(bounds);
}

void PanelHeader::setPanelName(const juce::String& name)
{
    panelName = name;
    titleLabel.setText(name, juce::dontSendNotification);
}

void PanelHeader::setShowUndockButton(bool show)
{
    showUndock = show;
    undockButton.setVisible(show && isVisible());
    resized();
}

void PanelHeader::setShowCloseButton(bool show)
{
    showClose = show;
    closeButton.setVisible(show && isVisible());
    resized();
}

// ============================================================================
// DockablePanel Implementation
// ============================================================================

DockablePanel::DockablePanel(PanelType type, const juce::String& name)
    : panelType(type)
    , panelName(name)
{
    createHeader();
    setOpaque(true);
}

DockablePanel::~DockablePanel()
{
    // Header wird automatisch durch unique_ptr gelöscht
}

juce::String DockablePanel::getPanelID() const
{
    return panelName + "_" + juce::String(static_cast<int>(panelType));
}

void DockablePanel::setShowHeader(bool show)
{
    showHeader = show;
    updateHeaderVisibility();
    resized();
}

void DockablePanel::setDockStateInternal(DockState state)
{
    if (dockState != state)
    {
        dockState = state;
        onDockStateChanged(state);
        updateHeaderVisibility();
    }
}

void DockablePanel::setDockPositionInternal(DockPosition position)
{
    dockPosition = position;
}

juce::ValueTree DockablePanel::saveState() const
{
    juce::ValueTree state("PanelState");
    state.setProperty("panelType", static_cast<int>(panelType), nullptr);
    state.setProperty("panelName", panelName, nullptr);
    state.setProperty("dockState", static_cast<int>(dockState), nullptr);
    state.setProperty("dockPosition", static_cast<int>(dockPosition), nullptr);
    state.setProperty("showHeader", showHeader, nullptr);

    // Subklassen fügen ihre eigenen Properties hinzu
    return state;
}

void DockablePanel::restoreState(const juce::ValueTree& state)
{
    if (!state.isValid())
        return;

    // Nur nicht-kritische Werte restoren
    // (panelType und panelName werden vom Panel selbst bestimmt)

    if (state.hasProperty("showHeader"))
        showHeader = state.getProperty("showHeader");

    updateHeaderVisibility();
}

juce::Rectangle<int> DockablePanel::getPreferredDockedBounds() const
{
    return juce::Rectangle<int>(0, 0, 350, 500);
}

juce::Rectangle<int> DockablePanel::getPreferredFloatingBounds() const
{
    return juce::Rectangle<int>(100, 100, 800, 600);
}

juce::Rectangle<int> DockablePanel::getContentBounds() const
{
    auto bounds = getLocalBounds();

    if (showHeader && header && header->isVisible())
    {
        bounds.removeFromTop(PanelHeader::headerHeight);
    }

    return bounds;
}

void DockablePanel::resized()
{
    if (!header)
        return;

    auto bounds = getLocalBounds();

    // Header oben positionieren
    if (showHeader && isDocked())
    {
        header->setBounds(bounds.removeFromTop(PanelHeader::headerHeight));
        header->setVisible(true);
    }
    else
    {
        header->setVisible(false);
    }

    // Subklassen können getContentBounds() für ihren Content verwenden
}

void DockablePanel::paint(juce::Graphics& g)
{
    auto& theme = ThemeManager::getInstance();

    // Dunkler Hintergrund als Basis
    g.fillAll(theme.getBackgroundColor());

    // Optional: Subtile Border wenn angedockt
    if (isDocked())
    {
        auto bounds = getLocalBounds();
        g.setColour(theme.getPanelBorderColor());
        g.drawRect(bounds, 1);
    }
}

void DockablePanel::createHeader()
{
    header = std::make_unique<PanelHeader>(panelName);

    // Header Callbacks mit den Panel-Callbacks verbinden
    header->onUndockClicked = [this]()
    {
        if (onRequestUndock)
            onRequestUndock();
    };

    header->onCloseClicked = [this]()
    {
        if (onRequestClose)
            onRequestClose();
    };

    addChildComponent(*header);
}

void DockablePanel::updateHeaderVisibility()
{
    if (header)
    {
        // Header nur zeigen wenn angedockt und showHeader = true
        bool shouldShow = showHeader && isDocked();
        header->setVisible(shouldShow);
    }
}

// ============================================================================
// Static Color Helpers (delegated to ThemeManager)
// ============================================================================

juce::Colour DockablePanel::getHeaderBackgroundColor()
{
    return ThemeManager::getInstance().getPanelHeaderColor();
}

juce::Colour DockablePanel::getHeaderTextColor()
{
    return ThemeManager::getInstance().getTextSecondaryColor();
}

juce::Colour DockablePanel::getHeaderButtonColor()
{
    return ThemeManager::getInstance().getInfoColor();
}
