#include "PluginWindow.h"
#include "PluginInstance.h"
#include "../Theme/ThemeManager.h"

//==============================================================================
// PluginEditorHolder - Internal component that holds the plugin editor
//==============================================================================

class PluginWindow::PluginEditorHolder : public juce::Component
{
public:
    explicit PluginEditorHolder(juce::AudioProcessorEditor* editor)
        : pluginEditor(editor)
    {
        jassert(pluginEditor != nullptr);

        addAndMakeVisible(pluginEditor);
        setSize(pluginEditor->getWidth(), pluginEditor->getHeight());
    }

    ~PluginEditorHolder() override
    {
        // Plugin editor is owned by the AudioProcessor, not us
        // We just remove it from our component hierarchy
        if (pluginEditor != nullptr)
        {
            removeChildComponent(pluginEditor);
        }
    }

    void resized() override
    {
        if (pluginEditor != nullptr)
        {
            pluginEditor->setBounds(getLocalBounds());
        }
    }

    juce::AudioProcessorEditor* getEditor() const { return pluginEditor; }

private:
    juce::AudioProcessorEditor* pluginEditor = nullptr;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PluginEditorHolder)
};

//==============================================================================
// PluginWindow
//==============================================================================

PluginWindow::PluginWindow(PluginInstance& plugin, std::function<void()> onClose)
    : DocumentWindow(plugin.getName(),
                     ThemeManager::getInstance().getBackgroundColor(),
                     DocumentWindow::closeButton | DocumentWindow::minimiseButton,
                     true)
    , pluginInstance(plugin)
    , onCloseCallback(std::move(onClose))
{
    auto& theme = ThemeManager::getInstance();

    // Create the plugin editor
    DBG("PluginWindow: Creating editor for " + plugin.getName());
    DBG("  Plugin has editor? " + juce::String(pluginInstance.hasEditor() ? "yes" : "no"));

    auto* editor = pluginInstance.createEditor();

    if (editor == nullptr)
    {
        // Plugin has no editor - show placeholder
        DBG("PluginWindow: Plugin has no editor: " + plugin.getName());
        valid = false;
        return;
    }

    // Create holder component
    editorHolder = std::make_unique<PluginEditorHolder>(editor);

    // Set up window
    setContentNonOwned(editorHolder.get(), true);

    // Window styling using theme
    setColour(DocumentWindow::backgroundColourId, theme.getPanelBackgroundColor());
    setColour(DocumentWindow::textColourId, theme.getInfoColor());

    // Set window properties
    setResizable(true, true);
    setUsingNativeTitleBar(false);  // Custom title bar for consistent look
    setTitleBarHeight(28);

    // Constrain size
    updateSizeConstraints();

    // Center on screen initially
    centreWithSize(getWidth(), getHeight());

    setVisible(true);
    addToDesktop(juce::ComponentPeer::StyleFlags::windowAppearsOnTaskbar);

    DBG("PluginWindow: Created window for " + plugin.getName());
}

PluginWindow::~PluginWindow()
{
    DBG("PluginWindow: Destroying window for " + getPluginName());

    // Clear content before destroying
    clearContentComponent();
    editorHolder.reset();
}

//==============================================================================
void PluginWindow::closeButtonPressed()
{
    valid = false;

    if (onCloseCallback)
    {
        onCloseCallback();
    }

    // Remove from desktop and mark for deletion
    removeFromDesktop();
    delete this;
}

void PluginWindow::resized()
{
    DocumentWindow::resized();
    lastBounds = getBounds();
}

void PluginWindow::moved()
{
    DocumentWindow::moved();
    lastBounds = getBounds();
}

//==============================================================================
juce::String PluginWindow::getPluginName() const
{
    return pluginInstance.getName();
}

void PluginWindow::invalidate()
{
    valid = false;
    closeButtonPressed();
}

void PluginWindow::bringToFront()
{
    toFront(true);
    setAlwaysOnTop(true);
    setAlwaysOnTop(false);  // Flash to get attention
}

//==============================================================================
void PluginWindow::toggleBypass()
{
    pluginInstance.setBypassed(!pluginInstance.isBypassed());
}

bool PluginWindow::isBypassed() const
{
    return pluginInstance.isBypassed();
}

//==============================================================================
void PluginWindow::updateSizeConstraints()
{
    if (editorHolder == nullptr)
        return;

    auto* editor = editorHolder->getEditor();
    if (editor == nullptr)
        return;

    // Get editor constraints
    int minWidth = editor->getConstrainer() ? editor->getConstrainer()->getMinimumWidth() : 100;
    int minHeight = editor->getConstrainer() ? editor->getConstrainer()->getMinimumHeight() : 100;
    int maxWidth = editor->getConstrainer() ? editor->getConstrainer()->getMaximumWidth() : 2000;
    int maxHeight = editor->getConstrainer() ? editor->getConstrainer()->getMaximumHeight() : 2000;

    // Add title bar height
    minHeight += getTitleBarHeight();
    maxHeight += getTitleBarHeight();

    getConstrainer()->setMinimumSize(minWidth, minHeight);
    getConstrainer()->setMaximumSize(maxWidth, maxHeight);
}

//==============================================================================
// PluginWindowManager
//==============================================================================

PluginWindowManager::~PluginWindowManager()
{
    closeAllWindows();
    stopTimer();
}

PluginWindow* PluginWindowManager::openWindowForPlugin(PluginInstance& plugin)
{
    // Check if window already exists
    if (auto* existing = getWindowForPlugin(plugin))
    {
        existing->bringToFront();
        return existing;
    }

    // Create new window
    auto* window = new PluginWindow(plugin, [this, &plugin]()
    {
        // Callback when window is closed
        for (int i = windows.size() - 1; i >= 0; --i)
        {
            if (&windows[i]->getPluginInstance() == &plugin)
            {
                if (onWindowClosed)
                    onWindowClosed(windows[i]);
                windows.remove(i);
                break;
            }
        }
    });

    if (!window->isValid())
    {
        DBG("PluginWindowManager: Window is INVALID for plugin " + plugin.getName());
        delete window;
        return nullptr;
    }

    DBG("PluginWindowManager: Adding window for " + plugin.getName());
    windows.add(window);

    // Start timer to check for deleted plugins
    startTimerHz(10);

    return window;
}

void PluginWindowManager::closeWindowForPlugin(PluginInstance& plugin)
{
    for (int i = windows.size() - 1; i >= 0; --i)
    {
        if (&windows[i]->getPluginInstance() == &plugin)
        {
            windows.remove(i);
            break;
        }
    }
}

void PluginWindowManager::closeAllWindows()
{
    windows.clear();
    stopTimer();
}

bool PluginWindowManager::hasWindowForPlugin(PluginInstance& plugin) const
{
    return getWindowForPlugin(plugin) != nullptr;
}

PluginWindow* PluginWindowManager::getWindowForPlugin(PluginInstance& plugin) const
{
    for (auto* window : windows)
    {
        if (&window->getPluginInstance() == &plugin)
            return window;
    }
    return nullptr;
}

juce::Array<PluginWindow*> PluginWindowManager::getAllWindows() const
{
    juce::Array<PluginWindow*> result;
    for (auto* window : windows)
        result.add(window);
    return result;
}

void PluginWindowManager::timerCallback()
{
    // Check for invalid windows (plugin was deleted)
    for (int i = windows.size() - 1; i >= 0; --i)
    {
        if (!windows[i]->isValid())
        {
            if (onWindowClosed)
                onWindowClosed(windows[i]);
            windows.remove(i);
        }
    }

    // Stop timer if no windows
    if (windows.isEmpty())
        stopTimer();
}
