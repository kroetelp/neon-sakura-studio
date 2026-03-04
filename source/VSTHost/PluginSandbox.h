#pragma once

/**
 * PluginSandbox - Plugin Isolation für Absturzsicherheit
 *
 * Phase 6.2: Plugin Sandbox (Optional)
 *
 * Diese Komponente isoliert Plugins in separaten Prozessen, um zu verhindern,
 * dass Plugin-Abstürze die gesamte DAW zum Absturz bringen.
 *
 * Architektur:
 *
 *   Main Process (DAW)                          Sandbox Process
 *   ─────────────────────                            ───────────────
 *   PluginSandboxManager  ──IPC──►  PluginSandboxWorker
 *                                               │
 *                                               ├──► PluginInstance
 *                                               └──► PluginState
 *
 * IPC Mechanismen:
 *   - Shared Memory: Audio/MIDI Buffer Exchange
 *   - Named Pipes: Control Commands & Status
 *   - Event Signals: Sync Points
 *
 * Features:
 *   - Prozess-Isolation pro Plugin
 *   - Auto-Recovery bei Abstürzen
 *   - CPU-Usage Monitoring
 *   - Crash Detection & Reporting
 *   - Graceful Shutdown
 *
 * Plattform-Spezifika:
 *   - Windows: Named Shared Memory (CreateFileMapping)
 *   - macOS: POSIX Shared Memory (shm_open)
 *   - Linux: POSIX Shared Memory (shm_open)
 */

#include <juce_core/juce_core.h>
#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_audio_processors/juce_audio_processors.h>
#include <memory>
#include <functional>
#include <atomic>

// Forward declarations
class PluginInstance;
struct PluginLoadRequest;

//==============================================================================
// Sandbox Status Codes
//==============================================================================

enum class SandboxState
{
    Idle,           // Sandbox nicht aktiv
    Starting,       // Prozess wird gestartet
    Running,        // Plugin wird ausgeführt
    Suspended,      // Sandbox vorübergehend pausiert
    Crashed,        // Plugin ist abgestürzt
    Terminating,    // Shutdown in Bearbeitung
    ShutDown        // Sandbox beendet
};

//==============================================================================
// Shared Memory Header (Platform-Agnostic)
//==============================================================================

#pragma pack(push, 1)  // Ensure no padding
struct SharedMemoryHeader
{
    // Magic number for validation
    uint32_t magic = 0x53414E44;  // "SAND"

    // Protocol version
    uint16_t version = 1;

    // Buffer sizes
    uint32_t audioBufferSize = 0;
    uint32_t audioBufferChannels = 0;
    uint32_t midiBufferSize = 0;

    // Status flags (atomic for lock-free access)
    std::atomic<uint32_t> statusFlags{0};

    // Sync flags
    std::atomic<bool> audioReady{false};
    std::atomic<bool> midiReady{false};
    std::atomic<bool> processing{false};

    // Error reporting
    int32_t lastErrorCode = 0;
    char lastErrorMessage[256] = {0};

    // CPU usage metrics (nanoseconds per block)
    std::atomic<uint64_t> processingTimeNs{0};
};
#pragma pack(pop)

//==============================================================================
// Shared Memory Buffer Layout
//==============================================================================

struct SharedMemoryBuffers
{
    SharedMemoryHeader header;

    // Audio buffers (stereo, pre-allocated)
    float* audioInput = nullptr;
    float* audioOutput = nullptr;

    // MIDI buffer (variable length events)
    struct MidiEvent
    {
        int32_t sampleOffset;
        int32_t messageSize;
        uint8_t data[4];  // Variable-length, 4 bytes max for standard messages
    };

    MidiEvent* midiInput = nullptr;
    MidiEvent* midiOutput = nullptr;

    int32_t midiInputCount = 0;
    int32_t midiOutputCount = 0;
};

//==============================================================================
// IPC Channel Types
//==============================================================================

enum class IPCChannel
{
    SharedMemory,   // High-performance audio/MIDI exchange
    NamedPipe,     // Control commands
    EventSignal     // Synchronization signals
};

//==============================================================================
// Sandbox Configuration
//==============================================================================

struct SandboxConfig
{
    uint32_t nodeId = 0;               // Plugin node ID
    juce::String pluginName;             // Display name
    juce::String pluginPath;             // Path to plugin binary
    double sampleRate = 44100.0;        // Target sample rate
    int blockSize = 512;                   // Block size

    // Memory allocation
    size_t sharedMemorySize = 1024 * 1024;  // 1MB default
    int maxMidiEvents = 256;              // Max MIDI events per block

    // Timeout settings
    int startupTimeoutMs = 10000;         // 10 seconds
    int responseTimeoutMs = 1000;         // 1 second
    int crashRecoveryDelayMs = 5000;     // 5 seconds

    // Auto-recovery
    bool enableAutoRecovery = true;
    int maxRestartAttempts = 3;

    // Monitoring
    bool enableCPUMonitoring = true;
    float cpuWarningThreshold = 0.9f;     // 90% CPU
};

//==============================================================================
// Crash Report
//==============================================================================

struct CrashReport
{
    uint32_t nodeId = 0;
    juce::String pluginName;
    juce::int64 crashTime = 0;
    juce::String errorMessage;
    juce::String stackTrace;

    int restartAttempt = 0;
    bool autoRecovered = false;
};

//==============================================================================
// Plugin Sandbox Manager (Host Process)
//==============================================================================

class PluginSandboxManager
{
public:
    PluginSandboxManager();
    ~PluginSandboxManager();

    // ============================================================
    // Lifecycle Management
    // ============================================================

    /** Initialize the sandbox manager. */
    void initialize();

    /** Shutdown all sandboxes. */
    void shutdown();

    // ============================================================
    // Sandbox Creation
    // ============================================================

    /**
     * Launch a plugin in a sandboxed process.
     * @param config Sandbox configuration
     * @return Sandbox ID for tracking, or 0 on failure
     */
    uint32_t launchSandbox(const SandboxConfig& config);

    /**
     * Terminate a sandbox.
     * @param sandboxId The sandbox ID to terminate
     * @param graceful If true, try graceful shutdown first
     */
    void terminateSandbox(uint32_t sandboxId, bool graceful = true);

    /**
     * Restart a crashed sandbox.
     * @param sandboxId The sandbox ID to restart
     * @return true if restart initiated successfully
     */
    bool restartSandbox(uint32_t sandboxId);

    // ============================================================
    // Audio/MIDI Exchange (Called from Audio Thread)
    // ============================================================

    /**
     * Send audio data to sandbox.
     * @param sandboxId The target sandbox
     * @param buffer Audio buffer (stereo)
     * @param numSamples Number of samples
     */
    void sendAudioToSandbox(uint32_t sandboxId, const float* leftChannel,
                           const float* rightChannel, int numSamples);

    /**
     * Receive audio data from sandbox.
     * @param sandboxId The source sandbox
     * @param buffer Destination buffer
     * @param numSamples Number of samples to read
     * @return true if data available
     */
    bool receiveAudioFromSandbox(uint32_t sandboxId, float* leftChannel,
                                float* rightChannel, int numSamples);

    /**
     * Send MIDI data to sandbox.
     * @param sandboxId The target sandbox
     * @param midiBuffer MIDI buffer
     */
    void sendMidiToSandbox(uint32_t sandboxId, const juce::MidiBuffer& midiBuffer);

    /**
     * Receive MIDI data from sandbox.
     * @param sandboxId The source sandbox
     * @param midiBuffer Destination MIDI buffer
     * @return Number of MIDI events received
     */
    int receiveMidiFromSandbox(uint32_t sandboxId, juce::MidiBuffer& midiBuffer);

    // ============================================================
    // Status Monitoring
    // ============================================================

    /** Get the state of a sandbox. */
    SandboxState getSandboxState(uint32_t sandboxId) const;

    /** Check if a sandbox has crashed. */
    bool isSandboxCrashed(uint32_t sandboxId) const;

    /** Get CPU usage of a sandbox (0.0 to 1.0). */
    float getSandboxCPUUsage(uint32_t sandboxId) const;

    /** Get the last crash report for a sandbox. */
    CrashReport getCrashReport(uint32_t sandboxId) const;

    /** Get all active sandbox IDs. */
    juce::Array<uint32_t> getActiveSandboxes() const;

    // ============================================================
    // Callbacks
    // ============================================================

    /** Set callback for sandbox crashes. */
    void setCrashCallback(std::function<void(const CrashReport&)> callback);

    /** Set callback for sandbox state changes. */
    void setStateChangeCallback(std::function<void(uint32_t sandboxId,
                                                 SandboxState newState)> callback);

    /** Set callback for CPU warning. */
    void setCPUWarningCallback(std::function<void(uint32_t sandboxId, float cpuUsage)> callback);

    // ============================================================
    // Statistics
    // ============================================================

    /** Get total number of crashes across all sandboxes. */
    int getTotalCrashes() const { return totalCrashes.load(); }

    /** Get number of crashes for a specific sandbox. */
    int getSandboxCrashCount(uint32_t sandboxId) const;

    /** Reset crash statistics. */
    void resetCrashStatistics();

    /** Get average CPU usage across all sandboxes. */
    float getAverageCPUUsage() const;

private:
    // Sandbox instance tracking
    struct SandboxInstance
    {
        uint32_t id;
        SandboxConfig config;
        SandboxState state;
        juce::ChildProcess* process = nullptr;
        juce::SharedMemoryHandle sharedMemoryHandle;
        SharedMemoryBuffers* buffers = nullptr;

        // Statistics
        std::atomic<int> crashCount{0};
        std::atomic<float> currentCPUUsage{0.0f};
        std::vector<float> cpuUsageHistory;

        // Recovery
        int restartAttempts = 0;
        juce::int64 lastCrashTime = 0;
    };

    std::unordered_map<uint32_t, std::unique_ptr<SandboxInstance>> sandboxes;
    std::atomic<uint32_t> nextSandboxId{1};
    std::atomic<int> totalCrashes{0};

    // Callbacks
    std::function<void(const CrashReport&)> crashCallback;
    std::function<void(uint32_t, SandboxState)> stateChangeCallback;
    std::function<void(uint32_t, float)> cpuWarningCallback;

    // Thread for monitoring sandbox processes
    std::unique_ptr<juce::Thread> monitorThread;
    std::atomic<bool> monitorThreadRunning{false};

    // ============================================================
    // Internal Methods
    // ============================================================

    /** Generate unique sandbox ID. */
    uint32_t generateSandboxId();

    /** Create shared memory for sandbox. */
    bool createSharedMemory(SandboxInstance& instance);

    /** Cleanup shared memory. */
    void cleanupSharedMemory(SandboxInstance& instance);

    /** Launch sandbox process. */
    bool launchProcess(SandboxInstance& instance);

    /** Monitor thread loop. */
    void monitorThreadFunc();

    /** Handle sandbox crash. */
    void handleSandboxCrash(SandboxInstance& instance);

    /** Calculate CPU usage from processing time. */
    float calculateCPUUsage(uint64_t processingTimeNs, int blockSize, double sampleRate);

    /** Send crash report to host. */
    void reportCrash(const SandboxInstance& instance, const juce::String& error);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PluginSandboxManager)
};

//==============================================================================
// Plugin Sandbox Worker (Sandbox Process)
//==============================================================================

/**
 * This class runs inside the sandboxed process and communicates
 * with the host process via shared memory and IPC.
 *
 * IMPORTANT: This must be compiled as a separate executable!
 */
class PluginSandboxWorker
{
public:
    PluginSandboxWorker();
    ~PluginSandboxWorker();

    /** Run the sandbox worker main loop. */
    int run(int argc, char* argv[]);

private:
    // ============================================================
    // IPC Communication
    // ============================================================

    /** Attach to shared memory. */
    bool attachToSharedMemory(const juce::String& memoryName, size_t size);

    /** Process audio block from shared memory. */
    void processAudioBlock();

    /** Send MIDI events to shared memory. */
    void sendMidiEvents();

    // ============================================================
    // Plugin Management
    // ============================================================

    /** Load the plugin. */
    bool loadPlugin(const SandboxConfig& config);

    /** Process audio through plugin. */
    void processPlugin();

    /** Cleanup plugin resources. */
    void cleanupPlugin();

    // ============================================================
    // Signal Handling (Crash Recovery)
    // ============================================================

    /** Setup signal handlers for crash detection. */
    void setupSignalHandlers();

    /** Handle crash signal. */
    void handleCrash(int signal);

    /** Write crash information to shared memory. */
    void reportCrash(const juce::String& message);

    // ============================================================
    // Member Variables
    // ============================================================

    juce::SharedMemoryHandle sharedMemoryHandle;
    SharedMemoryBuffers* buffers = nullptr;
    std::unique_ptr<juce::AudioPluginInstance> pluginInstance;

    double currentSampleRate = 44100.0;
    int currentBlockSize = 512;
    bool running = false;

    // Signal handling
    struct sigaction oldSigAction[10];
    bool crashHandled = false;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PluginSandboxWorker)
};
