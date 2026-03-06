#include "TabbedPanelContainer.h"
#include "ZoneSnapManager.h"
#include "../Theme/ThemeManager.h"

//==============================================================================
TabbedPanelContainer::TabbedPanelContainer()
{
    setOpaque(true);
}

TabbedPanelContainer::~TabbedPanelContainer() = default;

//==============================================================================
void TabbedPanelContainer::setTabPosition(TabPosition pos)
{
    if (tabPosition != pos)
    {
        tabPosition = pos;
        updateTabBounds();
        repaint();
    }
}

void TabbedPanelContainer::setCompactTabs(bool compact)
{
    if (compactTabs != compact)
    {
        compactTabs = compact;
        tabBarHeight = compact ? compactTabBarHeight : defaultTabBarHeight;
        updateTabBounds();
        repaint();
    }
}

void TabbedPanelContainer::setTabBarHeight(int height)
{
    if (tabBarHeight != height)
    {
        tabBarHeight = height;
        updateTabBounds();
        repaint();
    }
}

//==============================================================================
void TabbedPanelContainer::addPanel(PanelType type, const juce::String& tabName)
{
    // Erstelle neuen Tab
    Tab newTab;
    newTab.type = type;
    newTab.name = tabName;
    newTab.isActive = false;

    // TODO: Erstelle Panel-Instanz basierend auf PanelType
    // Dies erfordert Zugriff auf Factory oder Manager

    tabs.push_back(std::move(newTab));

    // Wenn dies der erste Tab ist, mache ihn aktiv
    if (tabs.size() == 1)
    {
        setActivePanel(type);
    }

    updateTabBounds();
    resized();
    repaint();
}

void TabbedPanelContainer::removePanel(PanelType type)
{
    // Finde und entferne den Tab
    auto it = std::find_if(tabs.begin(), tabs.end(),
        [type](const Tab& tab) { return tab.type == type; });

    if (it != tabs.end())
    {
        // Wenn der aktive Tab entfernt wurde, wechsle zum nächsten
        bool wasActive = it->isActive;
        tabs.erase(it);

        if (wasActive && !tabs.empty())
        {
            setActivePanel(tabs[0].type);
        }

        updateTabBounds();
        resized();
        repaint();
    }
}

void TabbedPanelContainer::setActivePanel(PanelType type)
{
    // Deaktiviere alle Tabs
    for (auto& tab : tabs)
    {
        tab.isActive = false;
    }

    // Aktiviere den gewählten Tab
    auto it = std::find_if(tabs.begin(), tabs.end(),
        [type](const Tab& tab) { return tab.type == type; });

    if (it != tabs.end())
    {
        it->isActive = true;
        activePanel = type;

        // Zeige das Panel
        for (auto& tab : tabs)
        {
            if (tab.panel)
            {
                tab.panel->setVisible(tab.isActive);
            }
        }
    }

    repaint();
}

//==============================================================================
void TabbedPanelContainer::paint(juce::Graphics& g)
{
    auto& theme = ThemeManager::getInstance();
    auto bounds = getLocalBounds().toFloat();

    // Zeichne Tab-Bar-Hintergrund
    g.fillAll(theme.getPanelHeaderColor());

    // Zeichne unteren Border
    g.setColour(juce::Colour(0xFF000000).withAlpha(0.3f));
    g.drawLine(juce::Line<float>(
        bounds.getBottomLeft(),
        bounds.getBottomRight(),
        1.0f
    ));

    // Zeichne Tabs
    for (int i = 0; i < static_cast<int>(tabs.size()); ++i)
    {
        auto tabBounds = getTabBounds(i);
        drawTab(g, tabs[i], tabBounds.toFloat());
    }
}

void TabbedPanelContainer::resized()
{
    updateTabBounds();

    // Content-Bereich für alle Panels aktualisieren
    auto contentBounds = getContentBounds();

    for (auto& tab : tabs)
    {
        if (tab.panel)
        {
            tab.panel->setBounds(contentBounds);
        }
    }
}

//==============================================================================
void TabbedPanelContainer::updateTabBounds()
{
    auto bounds = getLocalBounds();
    auto width = bounds.getWidth();
    auto height = bounds.getHeight();

    // Tab-Bereich basierend auf Position
    juce::Rectangle<int> tabBarArea;

    switch (tabPosition)
    {
        case TabPosition::Top:
            tabBarArea = juce::Rectangle<int>(0, 0, width, tabBarHeight);
            break;

        case TabPosition::Bottom:
            tabBarArea = juce::Rectangle<int>(0, height - tabBarHeight, width, tabBarHeight);
            break;

        case TabPosition::Left:
            tabBarArea = juce::Rectangle<int>(0, 0, tabBarHeight, height);
            break;

        case TabPosition::Right:
            tabBarArea = juce::Rectangle<int>(width - tabBarHeight, 0, tabBarHeight, height);
            break;
    }

    // Berechne Tab-Größen
    if (!tabs.empty())
    {
        auto tabArea = (tabPosition == TabPosition::Top || tabPosition == TabPosition::Bottom)
            ? tabBarArea
            : juce::Rectangle<int>(0, 0, tabBarHeight, height);

        auto availableWidth = (tabPosition == TabPosition::Top || tabPosition == TabPosition::Bottom)
            ? tabArea.getWidth()
            : tabArea.getHeight();

        auto availableHeight = (tabPosition == TabPosition::Top || tabPosition == TabPosition::Bottom)
            ? tabArea.getHeight()
            : tabArea.getWidth();

        auto tabSpacing = compactTabs ? 2 : 4;
        auto tabPadding = 8;

        int totalTabWidth = tabs.size() * (tabPadding * 2 + tabSpacing);
        int tabWidth = juce::jmin(
            (availableWidth - totalTabWidth) / static_cast<int>(tabs.size()),
            maxTabWidth
        );
        tabWidth = juce::jmax(tabWidth, minTabWidth);

        // Positioniere Tabs
        for (size_t i = 0; i < tabs.size(); ++i)
        {
            if (tabPosition == TabPosition::Top || tabPosition == TabPosition::Bottom)
            {
                // Horizontale Tabs
                auto x = tabPadding + i * (tabWidth + tabSpacing);
                tabs[i].bounds = juce::Rectangle<int>(x, 0, tabWidth, tabBarHeight);
            }
            else
            {
                // Vertikale Tabs (Left/Right)
                auto y = tabPadding + i * (tabWidth + tabSpacing);
                tabs[i].bounds = juce::Rectangle<int>(0, y, tabBarHeight, tabWidth);
            }
        }
    }
}

juce::Rectangle<int> TabbedPanelContainer::getContentBounds() const
{
    auto bounds = getLocalBounds();

    switch (tabPosition)
    {
        case TabPosition::Top:
            return bounds.withTrimmedTop(tabBarHeight);

        case TabPosition::Bottom:
            return bounds.withTrimmedBottom(tabBarHeight);

        case TabPosition::Left:
            return bounds.withTrimmedLeft(tabBarHeight);

        case TabPosition::Right:
            return bounds.withTrimmedRight(tabBarHeight);
    }
}

juce::Rectangle<int> TabbedPanelContainer::getTabBounds(int tabIndex) const
{
    if (tabIndex >= 0 && tabIndex < static_cast<int>(tabs.size()))
    {
        return tabs[tabIndex].bounds;
    }
    return juce::Rectangle<int>();
}

//==============================================================================
void TabbedPanelContainer::drawTab(juce::Graphics& g, const Tab& tab, juce::Rectangle<float> bounds)
{
    auto bgColor = getTabBackgroundColor(tab);
    auto textColor = getTabTextColor(tab);

    // Zeichne Tab-Hintergrund
    g.setColour(bgColor);
    g.fillRoundedRectangle(bounds, 4.0f);

    // Zeichne Tab-Rahmen
    if (!tab.isActive)
    {
        g.setColour(juce::Colour(0xFF000000).withAlpha(0.2f));
        g.drawRoundedRectangle(bounds, 4.0f, 1.0f);
    }

    // Zeichne Tab-Name
    g.setColour(textColor);
    g.setFont(juce::Font(compactTabs ? 11.0f : 12.0f));

    auto textArea = bounds.reduced(8.0f, 0.0f);
    g.drawText(tab.name, textArea,
              juce::Justification::centred, true);

    // Aktiv-Indikator
    if (tab.isActive)
    {
        auto indicatorBounds = (tabPosition == TabPosition::Top || tabPosition == TabPosition::Bottom)
            ? juce::Rectangle<float>(
                bounds.getX(),
                bounds.getBottom() - 2,
                bounds.getWidth(),
                2
              )
            : juce::Rectangle<float>(
                bounds.getRight() - 2,
                bounds.getY(),
                2,
                bounds.getHeight()
              );

        g.setColour(ThemeManager::getInstance().getAccentColor());
        g.fillRect(indicatorBounds);
    }
}

//==============================================================================
juce::Colour TabbedPanelContainer::getTabBackgroundColor(const Tab& tab) const
{
    auto& theme = ThemeManager::getInstance();

    if (tab.isActive)
    {
        return theme.getAccentColor();
    }
    if (tab.isHovered)
    {
        return theme.getButtonHoverColor();
    }

    return theme.getButtonColor();
}

juce::Colour TabbedPanelContainer::getTabTextColor(const Tab& tab) const
{
    auto& theme = ThemeManager::getInstance();

    if (tab.isActive)
    {
        return juce::Colours::white;
    }

    return theme.getTextPrimaryColor();
}
