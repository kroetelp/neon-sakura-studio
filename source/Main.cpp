#include <juce_gui_basics/juce_gui_basics.h>
#include <juce_gui_extra/juce_gui_extra.h>
#include <juce_opengl/juce_opengl.h>
#include "MainComponent.h"

// Custom logger that writes to a file
class FileLogger : public juce::Logger
{
public:
    FileLogger(const juce::File& file)
        : logFile(file)
    {
        logFile.deleteFile();
        logMessage("=== Neon Sakura Studio Log Started ===");
    }

    void logMessage(const juce::String& message) override
    {
        juce::ScopedLock lock(logLock);
        juce::String timestamp = juce::Time::getCurrentTime().formatted("%H:%M:%S.");
        timestamp += juce::String::formatted("%03d", juce::Time::getCurrentTime().getMilliseconds() % 1000);
        logFile.appendText(timestamp + " " + message + "\n");
    }

private:
    juce::File logFile;
    juce::CriticalSection logLock;
};

class NeonSakuraApplication : public juce::JUCEApplication
{
public:
    const juce::String getApplicationName() override    { return "NeonSakuraStudio"; }
    const juce::String getApplicationVersion() override { return "1.0.0"; }
    bool moreThanOneInstanceAllowed() override          { return true; }

    void initialise(const juce::String& /*commandLine*/) override
    {
        // Setup file logger - writes to Desktop for easy access
        auto logFile = juce::File::getSpecialLocation(juce::File::userDesktopDirectory)
                          .getChildFile("NeonSakuraStudio_Log.txt");
        fileLogger = std::make_unique<FileLogger>(logFile);
        juce::Logger::setCurrentLogger(fileLogger.get());

        DBG("Application starting...");
        DBG("Log file: " + logFile.getFullPathName());

        mainWindow.reset(new MainWindow(getApplicationName()));
    }

    void shutdown() override
    {
        DBG("Application shutting down...");
        mainWindow = nullptr;
        juce::Logger::setCurrentLogger(nullptr);
        fileLogger.reset();
    }

    void systemRequestedQuit() override
    {
        quit();
    }

    void anotherInstanceStarted(const juce::String& /*commandLine*/) override
    {
    }

    class MainWindow : public juce::DocumentWindow
    {
    public:
        MainWindow(juce::String name)
            : DocumentWindow(name, juce::Colours::darkgrey, DocumentWindow::allButtons)
        {
            setUsingNativeTitleBar(true);
            setContentOwned(new MainComponent(), true);

            #if JUCE_IOS || JUCE_ANDROID
            setFullScreen(true);
            #else
            setResizable(true, true);
            centreWithSize(900, 600);
            #endif

            // Enable OpenGL for GPU-accelerated rendering
            openGLContext.attachTo(*getTopLevelComponent());
            openGLContext.setContinuousRepainting(false);  // Only repaint when needed

            setVisible(true);
        }

        ~MainWindow()
        {
            openGLContext.detach();
        }

        void closeButtonPressed() override
        {
            juce::JUCEApplication::getInstance()->systemRequestedQuit();
        }

    private:
        juce::OpenGLContext openGLContext;
        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MainWindow)
    };

private:
    std::unique_ptr<MainWindow> mainWindow;
    std::unique_ptr<FileLogger> fileLogger;
};

START_JUCE_APPLICATION(NeonSakuraApplication)
