#include "PluginBrowserComponent.h"
#include "VSTPluginManager.h"
#include "PluginScanner.h"
#include "PluginInstance.h"
#include "PluginWindow.h"

//==============================================================================
// Neon Color Palette (matching existing theme)
//==============================================================================
namespace NeonColors
{
    const juce::Colour background      {0xff1a1a2e};  // Dark blue-gray
    const juce::Colour panelBg         {0xff16213e};  // Slightly lighter
    const juce::Colour panelBgLight    {0xff0f3460};  // Highlight
    const juce::Colour accent          {0xffe94560};  // Neon pink
    const juce::Colour accentSecondary {0xff00ffff};  // Cyan
    const juce::Colour accentGreen     {0xff00b894};  // Neon green
    const juce::Colour text            {0xffe0e0e0};  // Light gray
    const juce::Colour textDim         {0xff888888};  // Dimmed text
    const juce::Colour selection       {0xff533483};  // Purple selection
    const juce::Colour border          {0xff333355};  // Subtle border
}

//==============================================================================
// SearchBox - Custom search input with neon styling
//==============================================================================

class PluginBrowserComponent::SearchBox : public juce::TextEditor,
                                           private juce::TextEditor::Listener
{
public:
    SearchBox()
    {
        setColour(TextEditor::backgroundColourId, NeonColors::panelBg);
        setColour(TextEditor::textColourId, NeonColors::text);
        setColour(TextEditor::highlightColourId, NeonColors::accent);
        setColour(TextEditor::highlightedTextColourId, juce::Colours::black);
        setColour(TextEditor::outlineColourId, NeonColors::border);
        setColour(TextEditor::focusedOutlineColourId, NeonColors::accentSecondary);

        setFont(juce::Font(14.0f));
        setTextToShowWhenEmpty("Search plugins...", NeonColors::textDim);
        setJustification(juce::Justification::centredLeft);

        addListener(this);
    }

    std::function<void(const juce::String&)> onSearchChanged;

private:
    void textEditorTextChanged(juce::TextEditor&) override
    {
        if (onSearchChanged)
            onSearchChanged(getText());
    }
};

//==============================================================================
// NeonProgressBar - Custom progress bar with neon styling
//==============================================================================

class PluginBrowserComponent::NeonProgressBar : public juce::Component
{
public:
    NeonProgressBar()
    {
        setInterceptsMouseClicks(false, false);
    }

    void setProgress(float prog)
    {
        progress = juce::jlimit(0.0f, 1.0f, prog);
        repaint();
    }

    void setStatusText(const juce::String& text)
    {
        statusText = text;
        repaint();
    }

    void paint(juce::Graphics& g) override
    {
        auto bounds = getLocalBounds().toFloat();

        // Background
        g.setColour(NeonColors::panelBg);
        g.fillRoundedRectangle(bounds, 4.0f);

        // Progress bar background
        auto barBounds = bounds.reduced(4.0f, 4.0f);
        barBounds.removeFromTop(barBounds.getHeight() * 0.4f);
        auto barHeight = barBounds.getHeight();

        g.setColour(NeonColors::border);
        g.fillRoundedRectangle(barBounds, 2.0f);

        // Progress fill
        if (progress > 0.0f)
        {
            auto fillWidth = barBounds.getWidth() * progress;
            auto fillBounds = barBounds.removeFromLeft(fillWidth);

            // Gradient fill
            juce::ColourGradient gradient(
                NeonColors::accentSecondary, 0.0f, 0.0f,
                NeonColors::accent, fillWidth, 0.0f, false);
            g.setGradientFill(gradient);
            g.fillRoundedRectangle(fillBounds, 2.0f);

            // Glow effect
            g.setColour(NeonColors::accentSecondary.withAlpha(0.3f));
            g.fillRoundedRectangle(fillBounds.expanded(0, 1), 3.0f);
        }

        // Status text
        g.setColour(NeonColors::text);
        g.setFont(11.0f);
        g.drawText(statusText, bounds.reduced(6, 2), juce::Justification::topLeft);
    }

private:
    float progress = 0.0f;
    juce::String statusText = "Ready";
};

//==============================================================================
// PluginTreeItem - TreeViewItem for individual plugins
//==============================================================================

class PluginBrowserComponent::PluginTreeItem : public juce::TreeViewItem
{
public:
    PluginTreeItem(const juce::PluginDescription& desc, PluginBrowserComponent& owner)
        : description(desc), browser(owner)
    {
        setDrawsInLeftMargin(true);
    }

    bool mightContainSubItems() override { return false; }

    void paintItem(juce::Graphics& g, int width, int height) override
    {
        // Background
        if (isSelected())
        {
            g.setColour(NeonColors::selection);
            g.fillRect(0, 0, width, height);
        }

        // Plugin name
        g.setColour(isSelected() ? NeonColors::accentSecondary : NeonColors::text);
        g.setFont(13.0f);
        g.drawText(description.name, 20, 0, width - 24, height, juce::Justification::centredLeft, true);

        // Format badge
        auto formatWidth = g.getCurrentFont().getStringWidth(description.pluginFormatName) + 10;
        auto badgeBounds = juce::Rectangle<int>(width - formatWidth - 4, 2, formatWidth, height - 4);

        g.setColour(NeonColors::accent.withAlpha(0.3f));
        g.fillRoundedRectangle(badgeBounds.toFloat(), 3.0f);

        g.setColour(NeonColors::accent);
        g.setFont(10.0f);
        g.drawText(description.pluginFormatName, badgeBounds, juce::Justification::centred, true);
    }

    void itemClicked(const juce::MouseEvent&) override
    {
        setSelected(true, true);
        browser.onPluginSelectedSafe(description);
    }

    void itemDoubleClicked(const juce::MouseEvent&) override
    {
        browser.onPluginDoubleClickedSafe(description);
    }

    bool canBeSelected() const override { return true; }

    juce::String getUniqueName() const override
    {
        return description.fileOrIdentifier + "_" + description.name;
    }

    const juce::PluginDescription& getDescription() const { return description; }

private:
    juce::PluginDescription description;
    PluginBrowserComponent& browser;
};

//==============================================================================
// PluginGroupItem - TreeViewItem for plugin groups (manufacturer, format, etc.)
//==============================================================================

class PluginBrowserComponent::PluginGroupItem : public juce::TreeViewItem
{
public:
    PluginGroupItem(const juce::String& name, const juce::String& count, PluginBrowserComponent& owner)
        : groupName(name), pluginCount(count), browser(owner)
    {
        setDrawsInLeftMargin(true);
        setOpen(true);
    }

    bool mightContainSubItems() override { return true; }

    void paintItem(juce::Graphics& g, int width, int height) override
    {
        // Background
        g.setColour(NeonColors::panelBg);
        g.fillRect(0, 0, width, height);

        // Bottom border
        g.setColour(NeonColors::border);
        g.drawHorizontalLine(height - 1, 0.0f, (float)width);

        // Expand/collapse indicator
        g.setColour(NeonColors::accentSecondary);
        g.setFont(14.0f);
        g.drawText(isOpen() ? "▼" : "▶", 4, 0, 16, height, juce::Justification::centred);

        // Group name
        g.setColour(NeonColors::accentSecondary);
        g.setFont(juce::Font(13.0f, juce::Font::bold));
        g.drawText(groupName, 20, 0, width - 80, height, juce::Justification::centredLeft, true);

        // Count badge
        auto countWidth = g.getCurrentFont().getStringWidth(pluginCount) + 12;
        auto badgeBounds = juce::Rectangle<int>(width - countWidth - 6, 3, countWidth, height - 6);

        g.setColour(NeonColors::accentSecondary.withAlpha(0.2f));
        g.fillRoundedRectangle(badgeBounds.toFloat(), 8.0f);

        g.setColour(NeonColors::accentSecondary);
        g.setFont(11.0f);
        g.drawText(pluginCount, badgeBounds, juce::Justification::centred, true);
    }

    bool canBeSelected() const override { return false; }

    juce::String getUniqueName() const override { return "group_" + groupName; }

    void addPlugin(const juce::PluginDescription& desc)
    {
        addSubItem(new PluginTreeItem(desc, browser));
    }

private:
    juce::String groupName;
    juce::String pluginCount;
    PluginBrowserComponent& browser;
};

//==============================================================================
// RootTreeItem - Concrete root item for the tree
//==============================================================================

class RootTreeItem : public juce::TreeViewItem
{
public:
    RootTreeItem() = default;

    bool mightContainSubItems() override { return true; }
    juce::String getUniqueName() const override { return "ROOT"; }
};

//==============================================================================
// PluginBrowserComponent
//==============================================================================

PluginBrowserComponent::PluginBrowserComponent(VSTPluginManager& manager,
                                                 PluginWindowManager* wm)
    : pluginManager(manager)
    , windowManager(wm)
{
    setupUI();
    setupCallbacks();
    applyNeonStyle();

    // Initial refresh
    refreshList();
}

PluginBrowserComponent::~PluginBrowserComponent()
{
    stopTimer();
}

//==============================================================================
void PluginBrowserComponent::paint(juce::Graphics& g)
{
    // Background
    g.fillAll(NeonColors::background);

    // Header gradient
    auto headerBounds = getLocalBounds().removeFromTop(44);
    juce::ColourGradient headerGradient(
        NeonColors::panelBg, 0.0f, 0.0f,
        NeonColors::background, 0.0f, 44.0f, false);
    g.setGradientFill(headerGradient);
    g.fillRect(headerBounds);

    // Subtle border around entire component
    g.setColour(NeonColors::border);
    g.drawRect(getLocalBounds(), 1);
}

void PluginBrowserComponent::resized()
{
    auto bounds = getLocalBounds();

    // Header area (search + buttons)
    auto headerBounds = bounds.removeFromTop(44).reduced(6, 6);

    // Search box takes most of the header
    searchBox->setBounds(headerBounds.removeFromLeft(headerBounds.getWidth() - 250));

    // Buttons
    headerBounds.removeFromLeft(8);
    addFolderButton->setBounds(headerBounds.removeFromLeft(75));
    headerBounds.removeFromLeft(4);
    rescanButton->setBounds(headerBounds.removeFromLeft(70));
    headerBounds.removeFromLeft(4);
    optionsButton->setBounds(headerBounds.removeFromLeft(70));

    // Progress bar area
    bounds.removeFromTop(2);
    progressBar->setBounds(bounds.removeFromBottom(32).reduced(6, 4));

    // Tree view takes the rest
    pluginTree->setBounds(bounds.reduced(2, 0));
}

void PluginBrowserComponent::lookAndFeelChanged()
{
    applyNeonStyle();
}

//==============================================================================
void PluginBrowserComponent::setGroupMode(GroupMode mode)
{
    currentGroupMode = mode;
    rebuildTree();
}

void PluginBrowserComponent::refreshList()
{
    // Cache plugins from manager
    cachedPlugins = pluginManager.searchPlugins("");

    DBG("PluginBrowser: refreshList - Found " + juce::String(cachedPlugins.size()) + " total plugins");
    for (const auto& desc : cachedPlugins)
    {
        DBG("  - " + desc.name + " by " + desc.manufacturerName + " (" + desc.pluginFormatName + ")");
    }

    rebuildTree();

    // Update status
    auto numPlugins = cachedPlugins.size();
    progressBar->setStatusText(juce::String(numPlugins) + " plugins loaded");
}

void PluginBrowserComponent::startScan()
{
    pluginManager.scanForPlugins();
    startTimerHz(15);  // Update progress at 15 Hz
}

void PluginBrowserComponent::cancelScan()
{
    pluginManager.getScanner().cancelScan();
    stopTimer();
}

void PluginBrowserComponent::clearSearch()
{
    searchBox->setText("");
    filterTree("");
}

bool PluginBrowserComponent::isScanning() const
{
    return pluginManager.isScanning();
}

float PluginBrowserComponent::getScanProgress() const
{
    return pluginManager.getScanProgress();
}

//==============================================================================
void PluginBrowserComponent::timerCallback()
{
    if (isScanning())
    {
        lastScanProgress = getScanProgress();
        progressBar->setProgress(lastScanProgress);

        auto& scanner = pluginManager.getScanner();
        auto currentFile = scanner.getCurrentFile();

        if (currentFile.isNotEmpty())
        {
            juce::File f(currentFile);
            progressBar->setStatusText("Scanning: " + f.getFileName());
        }

        // Refresh tree periodically during scan
        static int refreshCounter = 0;
        if (++refreshCounter >= 15)  // Refresh every second
        {
            refreshCounter = 0;
            refreshList();
        }
    }
    else
    {
        // Scan complete
        stopTimer();
        progressBar->setProgress(1.0f);
        progressBar->setStatusText("Scan complete - " + juce::String(cachedPlugins.size()) + " plugins");
        refreshList();
    }
}

bool PluginBrowserComponent::keyPressed(const juce::KeyPress& key, juce::Component*)
{
    if (key == juce::KeyPress::returnKey)
    {
        loadSelectedPlugin();
        return true;
    }

    if (key == juce::KeyPress::F5Key)
    {
        startScan();
        return true;
    }

    if (key == juce::KeyPress::escapeKey)
    {
        if (isScanning())
        {
            cancelScan();
            return true;
        }
        clearSearch();
        return true;
    }

    return false;
}

//==============================================================================
void PluginBrowserComponent::setupUI()
{
    // Search box
    searchBox = std::make_unique<SearchBox>();
    searchBox->onSearchChanged = [this](const juce::String& term)
    {
        filterTree(term);
    };
    addAndMakeVisible(*searchBox);

    // Tree view
    pluginTree = std::make_unique<juce::TreeView>();
    pluginTree->setColour(juce::TreeView::backgroundColourId, NeonColors::background);
    pluginTree->setColour(juce::TreeView::linesColourId, NeonColors::border);
    pluginTree->setColour(juce::TreeView::dragAndDropIndicatorColourId, NeonColors::accent);
    pluginTree->setRootItemVisible(false);
    pluginTree->setDefaultOpenness(true);
    pluginTree->setMultiSelectEnabled(false);
    addAndMakeVisible(*pluginTree);

    // Rescan button
    rescanButton = std::make_unique<juce::TextButton>("Rescan");
    rescanButton->setTooltip("Scan for new plugins (F5)");
    rescanButton->onClick = [this]()
    {
        if (isScanning())
            cancelScan();
        else
            startScan();
    };
    addAndMakeVisible(*rescanButton);

    // Add Folder button
    addFolderButton = std::make_unique<juce::TextButton>("Add Folder");
    addFolderButton->setTooltip("Add custom VST3 plugin folder");
    addFolderButton->onClick = [this]()
    {
        folderChooser = std::make_unique<juce::FileChooser>(
            "Select VST3 Plugin Folder",
            juce::File::getSpecialLocation(juce::File::userDocumentsDirectory));

        folderChooser->launchAsync(juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectDirectories,
            [this](const juce::FileChooser& fc)
            {
                if (fc.getResults().size() > 0)
                {
                    auto selectedFolder = fc.getResult();
                    DBG("Scanning custom folder: " + selectedFolder.getFullPathName());

                    juce::FileSearchPath searchPath;
                    searchPath.add(selectedFolder);

                    pluginManager.scanForPlugins(searchPath);
                    startTimerHz(15);
                }
            });
    };
    addAndMakeVisible(*addFolderButton);

    // Options button
    optionsButton = std::make_unique<juce::TextButton>("Options");
    optionsButton->onClick = [this]()
    {
        juce::PopupMenu menu;
        menu.addItem("Group by Manufacturer", true, currentGroupMode == GroupMode::ByManufacturer,
                     [this]() { setGroupMode(GroupMode::ByManufacturer); });
        menu.addItem("Group by Format", true, currentGroupMode == GroupMode::ByFormat,
                     [this]() { setGroupMode(GroupMode::ByFormat); });
        menu.addItem("Group by Category", true, currentGroupMode == GroupMode::ByCategory,
                     [this]() { setGroupMode(GroupMode::ByCategory); });
        menu.addItem("Flat List", true, currentGroupMode == GroupMode::Flat,
                     [this]() { setGroupMode(GroupMode::Flat); });
        menu.addSeparator();
        menu.addItem("Load Plugin File...", [this]()
        {
            // VST3 bundles are directories on Windows, so allow selecting directories
            folderChooser = std::make_unique<juce::FileChooser>(
                "Select VST3 Plugin Bundle",
                juce::File("C:\\Program Files\\Common Files\\VST3"),
                "*.vst3", true);  // true = treat as directories

            DBG("========================================");
            DBG("PluginBrowser: Load Plugin File menu item clicked!");
            DBG("Opening file chooser for VST3 plugins...");

            juce::AlertWindow::showMessageBoxAsync(
                juce::AlertWindow::InfoIcon,
                "Load Plugin File",
                "File chooser is opening. Please select a VST3 plugin.",
                "OK"
            );

            folderChooser->launchAsync(juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectDirectories,
                [this](const juce::FileChooser& fc)
                {
                    DBG("========================================");
                    DBG("PluginBrowser: File chooser callback invoked!");
                    DBG("  Results size: " + juce::String(fc.getResults().size()));

                    if (fc.getResults().size() > 0)
                    {
                        auto pluginFile = fc.getResult();
                        DBG("========================================");
                        DBG("Loading plugin file directly: " + pluginFile.getFullPathName());
                        DBG("  File exists: " + juce::String(pluginFile.exists() ? "yes" : "no"));
                        DBG("  File is directory: " + juce::String(pluginFile.isDirectory() ? "yes" : "no"));

                        // Try to load directly
                        pluginManager.loadPlugin(pluginFile, [this, filename = pluginFile.getFullPathName()](std::unique_ptr<PluginInstance> instance)
                        {
                            DBG("========================================");
                            DBG("PluginBrowser: loadPlugin (file) callback invoked!");
                            DBG("  File: " + filename);
                            DBG("  Instance: " + juce::String(instance ? "VALID" : "NULLPTR"));

                            if (instance && onPluginLoaded)
                            {
                                DBG("Plugin loaded successfully from file!");
                                juce::MessageManager::callAsync([this, inst = instance.release()]() mutable
                                {
                                    DBG("PluginBrowser: MessageManager callback executed (file load)!");
                                    onPluginLoaded(std::unique_ptr<PluginInstance>(inst));
                                });
                            }
                            else
                            {
                                DBG("Failed to load plugin file!");
                                juce::AlertWindow::showMessageBoxAsync(
                                    juce::AlertWindow::WarningIcon,
                                    "Plugin Load Failed",
                                    "Could not load the plugin file:\n" + filename,
                                    "OK"
                                );
                            }
                        });
                    }
                    else
                    {
                        DBG("PluginBrowser: No file selected in file chooser!");
                        juce::AlertWindow::showMessageBoxAsync(
                            juce::AlertWindow::InfoIcon,
                            "No File Selected",
                            "Please select a plugin file to load.",
                            "OK"
                        );
                    }
                });
        });
        menu.addSeparator();
        menu.addItem("Show Blacklist", [this]()
        {
            auto blacklist = pluginManager.getBlacklist();
            juce::String message = "Blacklisted plugins:\n\n";

            if (blacklist.isEmpty())
            {
                message += "No plugins on blacklist.";
            }
            else
            {
                for (const auto& file : blacklist)
                {
                    message += file + "\n";
                }
            }

            juce::AlertWindow::showMessageBoxAsync(
                juce::AlertWindow::InfoIcon,
                "Blacklisted Plugins",
                message,
                "OK"
            );
        });
        menu.addItem("Clear Blacklist", [this]()
        {
            pluginManager.clearPluginListCache();
            juce::AlertWindow::showMessageBoxAsync(
                juce::AlertWindow::InfoIcon,
                "Blacklist Cleared",
                "All blacklisted plugins have been removed. Starting rescan...",
                "OK"
            );
            // Start scan after alert is shown
            juce::Timer::callAfterDelay(100, [this]() { startScan(); });
        });
        menu.addSeparator();
        menu.addItem("Clear Cache && Rescan", [this]()
        {
            pluginManager.clearPluginListCache();
            startScan();
        });

        menu.showMenuAsync(juce::PopupMenu::Options()
                           .withTargetComponent(optionsButton.get()));
    };
    addAndMakeVisible(*optionsButton);

    // Progress bar
    progressBar = std::make_unique<NeonProgressBar>();
    addAndMakeVisible(*progressBar);

    // Add key listener
    addKeyListener(this);
    setWantsKeyboardFocus(true);
}

void PluginBrowserComponent::setupCallbacks()
{
    pluginManager.setScanProgressCallback([this](float progress)
    {
        lastScanProgress = progress;
    });

    pluginManager.setScanCompleteCallback([this](int numFound)
    {
        progressBar->setProgress(1.0f);
        progressBar->setStatusText("Found " + juce::String(numFound) + " new plugins");
        refreshList();
    });
}

void PluginBrowserComponent::applyNeonStyle()
{
    // Rescan button
    rescanButton->setColour(juce::TextButton::buttonColourId, NeonColors::panelBgLight);
    rescanButton->setColour(juce::TextButton::buttonOnColourId, NeonColors::accent);
    rescanButton->setColour(juce::TextButton::textColourOffId, NeonColors::text);
    rescanButton->setColour(juce::TextButton::textColourOnId, NeonColors::accentSecondary);

    // Add Folder button
    addFolderButton->setColour(juce::TextButton::buttonColourId, NeonColors::accentGreen.withAlpha(0.3f));
    addFolderButton->setColour(juce::TextButton::buttonOnColourId, NeonColors::accentGreen);
    addFolderButton->setColour(juce::TextButton::textColourOffId, NeonColors::text);
    addFolderButton->setColour(juce::TextButton::textColourOnId, juce::Colours::black);

    // Options button
    optionsButton->setColour(juce::TextButton::buttonColourId, NeonColors::panelBgLight);
    optionsButton->setColour(juce::TextButton::buttonOnColourId, NeonColors::accent);
    optionsButton->setColour(juce::TextButton::textColourOffId, NeonColors::text);
    optionsButton->setColour(juce::TextButton::textColourOnId, NeonColors::accentSecondary);
}

void PluginBrowserComponent::rebuildTree()
{
    // Clear existing tree
    pluginTree->setRootItem(nullptr);
    auto rootItem = std::make_unique<RootTreeItem>();
    rootItem->setDrawsInLeftMargin(false);

    // Group plugins based on current mode
    juce::HashMap<juce::String, juce::Array<juce::PluginDescription>> groups;

    for (const auto& plugin : cachedPlugins)
    {
        juce::String groupKey;

        switch (currentGroupMode)
        {
            case GroupMode::ByManufacturer:
                groupKey = plugin.manufacturerName.isEmpty() ? "Unknown" : plugin.manufacturerName;
                break;

            case GroupMode::ByFormat:
                groupKey = plugin.pluginFormatName;
                break;

            case GroupMode::ByCategory:
                groupKey = plugin.category.isEmpty() ? "Uncategorized" : plugin.category;
                break;

            case GroupMode::Flat:
                groupKey = "All Plugins";
                break;
        }

        if (!groups.contains(groupKey))
            groups.set(groupKey, juce::Array<juce::PluginDescription>());

        groups.getReference(groupKey).add(plugin);
    }

    // Sort group keys alphabetically
    juce::StringArray sortedKeys;
    for (auto it = groups.begin(); it != groups.end(); ++it)
        sortedKeys.add(it.getKey());
    sortedKeys.sort(true);

    // Create tree items
    for (const auto& key : sortedKeys)
    {
        auto& plugins = groups.getReference(key);
        auto countStr = juce::String(plugins.size()) + " plugins";

        auto* groupItem = new PluginGroupItem(key, countStr, *this);

        // Sort plugins within group by creating a sorted copy
        juce::Array<juce::PluginDescription> sortedPlugins;
        for (const auto& p : plugins)
            sortedPlugins.add(p);

        // Use JUCE's built-in sort with a comparator class
        struct PluginNameComparator
        {
            static int compareElements(const juce::PluginDescription& a, const juce::PluginDescription& b)
            {
                return a.name.compareNatural(b.name);
            }
        };

        PluginNameComparator comparator;
        sortedPlugins.sort(comparator);

        for (const auto& plugin : sortedPlugins)
            groupItem->addPlugin(plugin);

        rootItem->addSubItem(groupItem);
    }

    pluginTree->setRootItem(rootItem.release());
}

void PluginBrowserComponent::filterTree(const juce::String& searchTerm)
{
    // Rebuild tree with filtered plugins
    juce::Array<juce::PluginDescription> filtered;

    if (searchTerm.isEmpty())
    {
        filtered = cachedPlugins;
    }
    else
    {
        for (const auto& plugin : cachedPlugins)
        {
            if (plugin.name.containsIgnoreCase(searchTerm) ||
                plugin.manufacturerName.containsIgnoreCase(searchTerm) ||
                plugin.category.containsIgnoreCase(searchTerm))
            {
                filtered.add(plugin);
            }
        }
    }

    // Temporarily swap cached plugins and rebuild
    auto originalPlugins = cachedPlugins;
    cachedPlugins = filtered;
    rebuildTree();
    cachedPlugins = originalPlugins;

    // Update status
    if (searchTerm.isNotEmpty())
    {
        progressBar->setStatusText("Found " + juce::String(filtered.size()) + " matching plugins");
    }
}

void PluginBrowserComponent::loadSelectedPlugin()
{
    if (auto* selected = dynamic_cast<PluginTreeItem*>(pluginTree->getSelectedItem(0)))
    {
        onPluginDoubleClickedSafe(selected->getDescription());
    }
}

void PluginBrowserComponent::onPluginDoubleClickedSafe(const juce::PluginDescription& desc)
{
    DBG("========================================");
    DBG("PluginBrowser: Loading " + desc.name);
    DBG("  Manufacturer: " + desc.manufacturerName);
    DBG("  Format: " + desc.pluginFormatName);
    DBG("  Category: " + desc.category);
    DBG("  File: " + desc.fileOrIdentifier);

    // Load plugin asynchronously
    pluginManager.loadPlugin(desc, [this, callback = onPluginLoaded, name = desc.name](std::unique_ptr<PluginInstance> instance)
    {
        DBG("========================================");
        DBG("PluginBrowser: loadPlugin callback invoked!");
        DBG("  Name: " + name);
        DBG("  Instance: " + juce::String(instance ? "VALID" : "NULLPTR"));

        if (instance && callback)
        {
            DBG("PluginBrowser: Successfully loaded " + name);
            // Use MessageManager to safely call on the message thread
            auto instancePtr = instance.release();
            DBG("  Calling callback via MessageManager...");
            juce::MessageManager::callAsync([callback, instancePtr]() mutable
            {
                DBG("PluginBrowser: MessageManager callback executed!");
                callback(std::unique_ptr<PluginInstance>(instancePtr));
            });
        }
        else if (!instance)
        {
            DBG("PluginBrowser: Failed to load plugin: " + name);
        }
        else
        {
            DBG("PluginBrowser: Plugin loaded but no callback set!");
        }
    });
}

// Helper methods for PluginTreeItem callbacks
void PluginBrowserComponent::onPluginSelectedSafe(const juce::PluginDescription& desc)
{
    if (onPluginSelected)
        onPluginSelected(desc);
}
