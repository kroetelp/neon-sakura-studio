#include "PluginWindow.h"
#include "../Theme/ThemeManager.h"

//==============================================================================
// PluginEditorHolder - Internal component that holds plugin editor
//==============================================================================

class PluginEditorHolder : public juce::Component
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
        // Plugin editor is owned by AudioProcessor, not us
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

private:
    juce::AudioProcessorEditor* pluginEditor = nullptr;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PluginEditorHolder)
};

//==============================================================================
// PluginWindow - Floating window that hosts an external plugin's GUI editor
//==============================================================================

PluginWindow::PluginWindow(PluginInstance& plugin, std::function<void()> onCloseCallback)
    : juce::DocumentWindow(juce::Colours::black,
                        juce::DocumentWindow::closeButtonPress,
                        plugin.getName())
    , plugin(plugin)
    , onCloseCallback(onCloseCallback)
{
    setContentOwned(std::make_unique<PluginEditorHolder>(plugin.getEditor().release()));

    // Set window size and constraints
    setResizeLimits(600, 800, false, false, 600, 800);

    centreWithScreenBounds(getScreenWorkingArea().toFloat(), 0.7f);

    // Create close button
    closeButton.setLookAndFeel(&getLookAndFeel());
    closeButton.onClick = [this] { closeButtonPressed(); };
    addAndMakeVisible(&closeButton);

    startTimer(100);
}

PluginWindow::~PluginWindow()
{
    stopTimer();
    closeButtonPressed();
}

void PluginWindow::closeButtonPressed()
{
    closeButtonPressed();
}

void PluginWindow::resized()
{
    if (auto* editor = plugin.getEditor())
        {
            setSize(editor->getWidth(), editor->getHeight());
        }
}

void PluginWindow::moved()
{
    // Store last bounds for saving/restoring
    lastBounds = getBoundsInParent();
}

void PluginWindow::userTriedToCloseWindow()
{
    // User clicked X button - mark as invalid
    valid = false;
}

void PluginWindow::timerCallback()
{
    // Check if plugin was deleted
    if (!plugin.isValid())
    {
        invalidate();
    }
}

bool PluginWindow::isValid() const
{
    return valid;
}

void PluginWindow::invalidate()
{
    valid = false;
}

void PluginWindow::bringToFront()
{
    toFront(true);
}

void PluginWindow::toggleBypass()
{
    if (plugin.isValid())
    {
        plugin.setBypass(!plugin.getBypass());
    }
}

bool PluginWindow::isBypassed() const
{
    return plugin.isBypassed();
}

juce::Rectangle<int> PluginWindow::getLastBounds() const
{
    return lastBounds;
}

void PluginWindow::updateSizeConstraints()
{
    if (auto* editor = plugin.getEditor())
    {
        auto constrains = editor->getConstrains();
        setResizeLimits(juce::jmax(300, constrains.getMaximumWidth()),
                    juce::jmax(200, constrains.getMaximumHeight()),
                    false, false, 300, 200);
    }
}

juce::String PluginWindow::getPluginName() const
{
    return plugin.getName();
}

PluginInstance& PluginWindow::getPluginInstance()
{
    return plugin;
}

const PluginInstance& PluginWindow::getPluginInstance() const
{
    return plugin;
}

//==============================================================================
// PluginWindowManager - Manages all open plugin windows
//==============================================================================

PluginWindowManager::PluginWindowManager()
{
    startTimerHz(30);
}

PluginWindowManager::~PluginWindowManager()
{
    closeAllWindows();
    stopTimer();
}

PluginWindow* PluginWindowManager::openWindowForPlugin(PluginInstance& plugin)
{
    // Check if window already exists
    if (hasWindowForPlugin(plugin))
    {
        return getWindowForPlugin(plugin);
    }

    // Create new window
    auto* window = new PluginWindow(plugin, [this]{
        plugin.removeEditorListener(this);
    });

    windows.add(window);
    return window;
}

void PluginWindowManager::closeWindowForPlugin(PluginInstance& plugin)
{
    // Find and remove the window
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
    for (auto* window : windows)
    {
        window->closeButtonPressed();
    }
    windows.clear();
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
            return window.get();
    }
    return nullptr;
}

juce::Array<PluginWindow*> PluginWindowManager::getAllWindows() const
{
    return windows;
}

int PluginWindowManager::getNumWindows() const
{
    return windows.size();
}

void PluginWindowManager::setOnWindowClosed(std::function<void(PluginWindow*)> callback)
{
    onWindowClosed = callback;
}

void PluginWindowManager::timerCallback()
{
    // Check for invalid windows and close them
    for (int i = windows.size() - 1; i >= 0; --i)
    {
        if (!windows[i]->isValid())
        {
            windows.remove(i);
            if (onWindowClosed)
                onWindowClosed(windows[i]);
        }
    }
}
