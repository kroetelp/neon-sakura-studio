#pragma once

/**
 * PluginHostConfiguration - Configuration for VST Hosting in Neon Sakura Studio
 *
 * Settings for the DAW's plugin hosting capabilities.
 */

namespace PluginHostConfig
{
    // ============================================================
    // Plugin Limits
    // ============================================================

    constexpr int maxPlugins = 32;
    constexpr int maxPluginsPerTrack = 8;

    // ============================================================
    // Default Plugin Scan Paths (Platform-Specific)
    // ============================================================

    #if JUCE_WINDOWS
        // System-wide VST3 directory
        constexpr const char* defaultVST3Path = "C:\\Program Files\\Common Files\\VST3";
        // User-specific VST3 directory
        constexpr const char* userVST3Path = "C:\\Users\\%USERNAME%\\AppData\\Local\\Programs\\Common\\VST3";
        // Legacy VST2 path (deprecated, but some users still have plugins here)
        constexpr const char* defaultVSTPath = "C:\\Program Files\\VSTPlugins";
    #elif JUCE_MAC
        constexpr const char* defaultVST3Path = "/Library/Audio/Plug-Ins/VST3";
        constexpr const char* defaultAUPath   = "/Library/Audio/Plug-Ins/Components";
        constexpr const char* userVST3Path    = "~/Library/Audio/Plug-Ins/VST3";
        constexpr const char* userAUPath      = "~/Library/Audio/Plug-Ins/Components";
    #elif JUCE_LINUX
        constexpr const char* defaultVST3Path = "/usr/lib/vst3";
        constexpr const char* defaultVSTPath  = "/usr/lib/vst";
        constexpr const char* userVST3Path    = "~/.vst3";
    #endif

    // ============================================================
    // Plugin Format Flags
    // ============================================================

    constexpr bool enableVST3 = true;
    constexpr bool enableAU   = true;   // macOS only
    constexpr bool enableVST  = false;  // VST2 is deprecated and requires licensing

    // ============================================================
    // Audio Processing Settings
    // ============================================================

    constexpr int defaultBlockSize = 512;
    constexpr double defaultSampleRate = 44100.0;
    constexpr double maxSampleRate = 192000.0;

    // ============================================================
    // Plugin List Storage
    // ============================================================

    // Filename for the cached plugin list
    constexpr const char* pluginListFileName = "NeonSakuraPluginList.xml";

    // Filename for the plugin blacklist (crashed/failed plugins)
    constexpr const char* pluginBlacklistFileName = "NeonSakuraPluginBlacklist.xml";

    // Company/Developer folder name in AppData
    constexpr const char* appDataFolderName = "NeonSakuraStudio";

    // ============================================================
    // Scanning Settings
    // ============================================================

    // Maximum time (ms) to wait for a plugin to scan before considering it crashed
    constexpr int scanTimeoutMs = 30000; // 30 seconds per plugin

    // Show plugins that crashed during scan in a separate list
    constexpr bool trackFailedPlugins = true;
}
