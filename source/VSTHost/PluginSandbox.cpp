#include "PluginSandbox.h"
#include "PluginInstance.h"
#include "../AudioRouting/AudioRoutingGraph.h"
#include <cstring>
#include <iostream>

#if JUCE_WINDOWS
    #include <windows.h>
#elif JUCE_MAC
    #include <sys/mman.h>
    #include <signal.h>
    #include <unistd.h>
#elif JUCE_LINUX
    #include <sys/mman.h>
    #include <signal.h>
    #include <unistd.h>
#endif

//==============================================================================
// Constants
//==============================================================================

static constexpr uint32_t SANDBOX_MAGIC = 0x53414E44;  // "SAND"
static constexpr uint16_t SANDBOX_VERSION = 1;
static constexpr size_t DEFAULT_SHARED_MEMORY_SIZE = 1024 * 1024;  // 1MB
static constexpr size_t AUDIO_BUFFER_SIZE = 4096;  // Max samples per buffer

//==============================================================================
// Helper Functions
//==============================================================================

static juce::String getSharedMemoryName(uint32_t sandboxId)
{
    return "NeonSakura_Sandbox_" + juce::String(sandboxId);
}

static juce::String getNamedPipeName(uint32_t sandboxId)
{
#if JUCE_WINDOWS
    return "\\\\.\\pipe\\NeonSakura_Sandbox_" + juce::String(sandboxId);
#else
    return "/tmp/NeonSakura_Sandbox_" + juce::String(sandboxId) + ".pipe";
#endif
}

//==============================================================================
// PluginSandboxManager Implementation
//==============================================================================

PluginSandboxManager::PluginSandboxManager()
{
}

PluginSandboxManager::~PluginSandboxManager()
{
    shutdown();
}

//==============================================================================
// Lifecycle Management
//==============================================================================

void PluginSandboxManager::initialize()
{
    // Initialize monitoring thread
    monitorThreadRunning.store(true);
    monitorThread = std::make_unique<juce::Thread>("SandboxMonitor", [this]() {
        monitorThreadFunc();
    });
    monitorThread->startThread();

    DBG("PluginSandboxManager initialized");
}

void PluginSandboxManager::shutdown()
{
    // Terminate all sandboxes
    std::vector<uint32_t> sandboxIds = getActiveSandboxes().toStdString();

    for (uint32_t sandboxId : sandboxIds)
    {
        terminateSandbox(sandboxId, false);  // Force shutdown
    }

    // Stop monitor thread
    monitorThreadRunning.store(false);
    if (monitorThread && monitorThread->isThreadRunning())
    {
        monitorThread->stopThread(2000);
    }

    DBG("PluginSandboxManager shutdown complete");
}

//==============================================================================
// Sandbox Creation
//==============================================================================

uint32_t PluginSandboxManager::launchSandbox(const SandboxConfig& config)
{
    uint32_t sandboxId = generateSandboxId();

    // Create sandbox instance
    auto instance = std::make_unique<SandboxInstance>();
    instance->id = sandboxId;
    instance->config = config;
    instance->state = SandboxState::Starting;
    instance->restartAttempts = 0;

    // Create shared memory
    if (!createSharedMemory(*instance))
    {
        DBG("Failed to create shared memory for sandbox " + juce::String(sandboxId));
        return 0;
    }

    // Initialize buffers
    if (instance->buffers)
    {
        instance->buffers->header.magic = SANDBOX_MAGIC;
        instance->buffers->header.version = SANDBOX_VERSION;
        instance->buffers->header.audioBufferSize = AUDIO_BUFFER_SIZE;
        instance->buffers->header.audioBufferChannels = 2;
        instance->buffers->header.midiBufferSize = config.maxMidiEvents * sizeof(SharedMemoryBuffers::MidiEvent);
        instance->buffers->header.audioReady.store(false, std::memory_order_release);
        instance->buffers->header.midiReady.store(false, std::memory_order_release);
        instance->buffers->header.processing.store(false, std::memory_order_release);
        instance->buffers->header.processingTimeNs.store(0, std::memory_order_release);
    }

    // Launch sandbox process
    if (!launchProcess(*instance))
    {
        DBG("Failed to launch sandbox process for " + juce::String(sandboxId));
        cleanupSharedMemory(*instance);
        return 0;
    }

    // Store instance
    sandboxes[sandboxId] = std::move(instance);

    DBG("Sandbox " + juce::String(sandboxId) + " launched for plugin: " + config.pluginName);

    // Notify state change
    if (stateChangeCallback)
        stateChangeCallback(sandboxId, SandboxState::Starting);

    return sandboxId;
}

void PluginSandboxManager::terminateSandbox(uint32_t sandboxId, bool graceful)
{
    auto it = sandboxes.find(sandboxId);
    if (it == sandboxes.end())
        return;

    auto& instance = it->second;
    instance->state = SandboxState::Terminating;

    // Notify state change
    if (stateChangeCallback)
        stateChangeCallback(sandboxId, SandboxState::Terminating);

    if (graceful && instance->process)
    {
        // Send graceful shutdown signal via shared memory
        if (instance->buffers)
        {
            instance->buffers->header.statusFlags.fetch_or(0x01, std::memory_order_release);  // Shutdown flag
        }

        // Wait for process to terminate (up to 5 seconds)
        int timeout = 50;  // 50 * 100ms = 5 seconds
        while (instance->process->isRunning() && timeout > 0)
        {
            juce::Thread::sleep(100);
            timeout--;
        }

        // Force terminate if still running
        if (instance->process->isRunning())
        {
            instance->process->kill();
        }
    }
    else if (instance->process)
    {
        // Force terminate immediately
        instance->process->kill();
    }

    // Cleanup
    cleanupSharedMemory(*instance);
    sandboxes.erase(it);

    // Notify state change
    if (stateChangeCallback)
        stateChangeCallback(sandboxId, SandboxState::ShutDown);

    DBG("Sandbox " + juce::String(sandboxId) + " terminated");
}

bool PluginSandboxManager::restartSandbox(uint32_t sandboxId)
{
    auto it = sandboxes.find(sandboxId);
    if (it == sandboxes.end())
        return false;

    auto& instance = it->second;

    // Check if we've exceeded max restart attempts
    if (instance->restartAttempts >= instance->config.maxRestartAttempts)
    {
        DBG("Max restart attempts exceeded for sandbox " + juce::String(sandboxId));
        return false;
    }

    // Terminate current sandbox
    terminateSandbox(sandboxId, false);

    // Small delay before restart
    juce::Thread::sleep(instance->config.crashRecoveryDelayMs);

    // Relaunch with same config
    instance->restartAttempts++;
    uint32_t newId = launchSandbox(instance->config);

    return newId == sandboxId;  // Should be same ID
}

//==============================================================================
// Audio/MIDI Exchange (Called from Audio Thread)
//==============================================================================

void PluginSandboxManager::sendAudioToSandbox(uint32_t sandboxId, const float* leftChannel,
                                             const float* rightChannel, int numSamples)
{
    auto it = sandboxes.find(sandboxId);
    if (it == sandboxes.end() || !it->second->buffers)
        return;

    auto& instance = it->second;

    // Wait for buffer to be ready (simple spinlock with timeout)
    int timeout = 1000;  // ~1 second
    while (instance->buffers->header.processing.load(std::memory_order_acquire) && timeout > 0)
    {
        juce::Thread::yield();
        timeout--;
    }

    // Copy audio data to shared memory
    if (instance->buffers->audioInput && numSamples <= AUDIO_BUFFER_SIZE)
    {
        // Interleaved stereo copy
        int samplesToCopy = juce::jmin(numSamples, AUDIO_BUFFER_SIZE);
        for (int i = 0; i < samplesToCopy; ++i)
        {
            instance->buffers->audioInput[i * 2] = leftChannel ? leftChannel[i] : 0.0f;
            instance->buffers->audioInput[i * 2 + 1] = rightChannel ? rightChannel[i] : 0.0f;
        }

        instance->buffers->header.audioReady.store(true, std::memory_order_release);
    }
}

bool PluginSandboxManager::receiveAudioFromSandbox(uint32_t sandboxId, float* leftChannel,
                                                float* rightChannel, int numSamples)
{
    auto it = sandboxes.find(sandboxId);
    if (it == sandboxes.end() || !it->second->buffers)
        return false;

    auto& instance = it->second;

    // Check if output is ready
    if (!instance->buffers->header.audioReady.load(std::memory_order_acquire))
        return false;

    // Copy audio data from shared memory
    if (instance->buffers->audioOutput && numSamples <= AUDIO_BUFFER_SIZE)
    {
        int samplesToCopy = juce::jmin(numSamples, AUDIO_BUFFER_SIZE);
        for (int i = 0; i < samplesToCopy; ++i)
        {
            if (leftChannel)
                leftChannel[i] = instance->buffers->audioOutput[i * 2];
            if (rightChannel)
                rightChannel[i] = instance->buffers->audioOutput[i * 2 + 1];
        }

        instance->buffers->header.audioReady.store(false, std::memory_order_release);
        return true;
    }

    return false;
}

void PluginSandboxManager::sendMidiToSandbox(uint32_t sandboxId, const juce::MidiBuffer& midiBuffer)
{
    auto it = sandboxes.find(sandboxId);
    if (it == sandboxes.end() || !it->second->buffers)
        return;

    auto& instance = it->second;

    // Copy MIDI events to shared memory
    int eventCount = 0;
    for (const auto& metadata : midiBuffer)
    {
        if (eventCount >= instance->config.maxMidiEvents)
            break;

        auto* dest = &instance->buffers->midiInput[eventCount];
        dest->sampleOffset = metadata.samplePosition;
        dest->messageSize = juce::jmin(metadata.numBytes, 4);

        std::memcpy(dest->data, metadata.data, dest->messageSize);
        eventCount++;
    }

    instance->buffers->midiInputCount = eventCount;
    instance->buffers->header.midiReady.store(true, std::memory_order_release);
}

int PluginSandboxManager::receiveMidiFromSandbox(uint32_t sandboxId, juce::MidiBuffer& midiBuffer)
{
    auto it = sandboxes.find(sandboxId);
    if (it == sandboxes.end() || !it->second->buffers)
        return 0;

    auto& instance = it->second;

    // Check if MIDI output is ready
    if (!instance->buffers->header.midiReady.load(std::memory_order_acquire))
        return 0;

    // Copy MIDI events from shared memory
    midiBuffer.clear();
    for (int i = 0; i < instance->buffers->midiOutputCount; ++i)
    {
        const auto* src = &instance->buffers->midiOutput[i];
        midiBuffer.addEvent(src->data, src->messageSize, src->sampleOffset);
    }

    instance->buffers->header.midiReady.store(false, std::memory_order_release);
    return instance->buffers->midiOutputCount;
}

//==============================================================================
// Status Monitoring
//==============================================================================

SandboxState PluginSandboxManager::getSandboxState(uint32_t sandboxId) const
{
    auto it = sandboxes.find(sandboxId);
    if (it == sandboxes.end())
        return SandboxState::Idle;

    return it->second->state;
}

bool PluginSandboxManager::isSandboxCrashed(uint32_t sandboxId) const
{
    return getSandboxState(sandboxId) == SandboxState::Crashed;
}

float PluginSandboxManager::getSandboxCPUUsage(uint32_t sandboxId) const
{
    auto it = sandboxes.find(sandboxId);
    if (it == sandboxes.end())
        return 0.0f;

    return it->second->currentCPUUsage.load(std::memory_order_relaxed);
}

CrashReport PluginSandboxManager::getCrashReport(uint32_t sandboxId) const
{
    CrashReport report;
    auto it = sandboxes.find(sandboxId);
    if (it == sandboxes.end())
        return report;

    const auto& instance = it->second;
    report.nodeId = instance->id;
    report.pluginName = instance->config.pluginName;
    report.crashTime = instance->lastCrashTime;
    report.restartAttempt = instance->restartAttempts;

    if (instance->buffers)
    {
        report.errorMessage = juce::String(instance->buffers->header.lastErrorMessage);
    }

    return report;
}

juce::Array<uint32_t> PluginSandboxManager::getActiveSandboxes() const
{
    juce::Array<uint32_t> result;
    for (const auto& pair : sandboxes)
    {
        if (pair.second->state == SandboxState::Running)
            result.add(pair.first);
    }
    return result;
}

//==============================================================================
// Callbacks
//==============================================================================

void PluginSandboxManager::setCrashCallback(std::function<void(const CrashReport&)> callback)
{
    crashCallback = callback;
}

void PluginSandboxManager::setStateChangeCallback(std::function<void(uint32_t, SandboxState)> callback)
{
    stateChangeCallback = callback;
}

void PluginSandboxManager::setCPUWarningCallback(std::function<void(uint32_t, float)> callback)
{
    cpuWarningCallback = callback;
}

//==============================================================================
// Statistics
//==============================================================================

int PluginSandboxManager::getSandboxCrashCount(uint32_t sandboxId) const
{
    auto it = sandboxes.find(sandboxId);
    if (it == sandboxes.end())
        return 0;

    return it->second->crashCount.load();
}

void PluginSandboxManager::resetCrashStatistics()
{
    totalCrashes.store(0);
    for (auto& pair : sandboxes)
    {
        pair.second->crashCount.store(0);
        pair.second->cpuUsageHistory.clear();
    }
}

float PluginSandboxManager::getAverageCPUUsage() const
{
    if (sandboxes.empty())
        return 0.0f;

    float total = 0.0f;
    for (const auto& pair : sandboxes)
    {
        total += pair.second->currentCPUUsage.load();
    }

    return total / sandboxes.size();
}

//==============================================================================
// Internal Methods
//==============================================================================

uint32_t PluginSandboxManager::generateSandboxId()
{
    return nextSandboxId.fetch_add(1, std::memory_order_relaxed);
}

bool PluginSandboxManager::createSharedMemory(SandboxInstance& instance)
{
    juce::String memoryName = getSharedMemoryName(instance.id);
    size_t totalSize = sizeof(SharedMemoryBuffers)
                     + AUDIO_BUFFER_SIZE * 2 * sizeof(float)  // Stereo input/output
                     + instance.config.maxMidiEvents * sizeof(SharedMemoryBuffers::MidiEvent) * 2;  // MIDI in/out

#if JUCE_WINDOWS
    HANDLE hMap = CreateFileMappingA(
        INVALID_HANDLE_VALUE,
        PAGE_READWRITE,
        totalSize,
        memoryName.toStdString().c_str()
    );

    if (!hMap)
    {
        DBG("Failed to create file mapping: " + memoryName);
        return false;
    }

    void* ptr = MapViewOfFile(hMap, FILE_MAP_ALL_ACCESS, 0, 0, totalSize);
    if (!ptr)
    {
        CloseHandle(hMap);
        DBG("Failed to map view of file");
        return false;
    }

    instance.sharedMemoryHandle = hMap;
    instance.buffers = static_cast<SharedMemoryBuffers*>(ptr);

    // Setup buffer pointers
    char* data = static_cast<char*>(ptr) + sizeof(SharedMemoryBuffers);
    instance.buffers->audioInput = reinterpret_cast<float*>(data);
    instance.buffers->audioOutput = instance.buffers->audioInput + AUDIO_BUFFER_SIZE * 2;  // After input
    instance.buffers->midiInput = reinterpret_cast<SharedMemoryBuffers::MidiEvent*>(
        data + AUDIO_BUFFER_SIZE * 4 * sizeof(float));  // After audio buffers
    instance.buffers->midiOutput = instance.buffers->midiInput + instance.config.maxMidiEvents;

#else
    int fd = shm_open(memoryName.toStdString().c_str(), O_CREAT | O_RDWR, 0666);
    if (fd < 0)
    {
        DBG("Failed to open shared memory: " + memoryName);
        return false;
    }

    if (ftruncate(fd, totalSize) < 0)
    {
        close(fd);
        DBG("Failed to truncate shared memory");
        return false;
    }

    void* ptr = mmap(nullptr, totalSize, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if (ptr == MAP_FAILED)
    {
        close(fd);
        DBG("Failed to mmap shared memory");
        return false;
    }

    instance.sharedMemoryHandle = reinterpret_cast<juce::SharedMemoryHandle>(fd);
    instance.buffers = static_cast<SharedMemoryBuffers*>(ptr);

    // Setup buffer pointers (same as Windows)
    char* data = static_cast<char*>(ptr) + sizeof(SharedMemoryBuffers);
    instance.buffers->audioInput = reinterpret_cast<float*>(data);
    instance.buffers->audioOutput = instance.buffers->audioInput + AUDIO_BUFFER_SIZE * 2;
    instance.buffers->midiInput = reinterpret_cast<SharedMemoryBuffers::MidiEvent*>(
        data + AUDIO_BUFFER_SIZE * 4 * sizeof(float));
    instance.buffers->midiOutput = instance.buffers->midiInput + instance.config.maxMidiEvents;

#endif

    // Clear buffers
    if (instance.buffers->audioInput)
        std::memset(instance.buffers->audioInput, 0, AUDIO_BUFFER_SIZE * 2 * sizeof(float));

    DBG("Created shared memory for sandbox " + juce::String(instance.id));
    return true;
}

void PluginSandboxManager::cleanupSharedMemory(SandboxInstance& instance)
{
    if (!instance.buffers)
        return;

#if JUCE_WINDOWS
    if (instance.sharedMemoryHandle)
    {
        UnmapViewOfFile(instance.buffers);
        CloseHandle(instance.sharedMemoryHandle);
    }
#else
    if (instance.sharedMemoryHandle >= 0)
    {
        munmap(instance.buffers, 0);  // Size tracked internally by OS
        close(instance.sharedMemoryHandle);
    }

    // Also unlink the shared memory
    juce::String memoryName = getSharedMemoryName(instance.id);
    shm_unlink(memoryName.toStdString().c_str());
#endif

    instance.buffers = nullptr;
    instance.sharedMemoryHandle = nullptr;
}

bool PluginSandboxManager::launchProcess(SandboxInstance& instance)
{
    // Get path to sandbox worker executable
    juce::File appFile = juce::File::getSpecialLocation(juce::File::currentExecutableFile);
    juce::File sandboxExe = appFile.getSiblingFile("NeonSakuraSandboxWorker");

    juce::StringArray args;
    args.add("--sandbox-id=" + juce::String(instance.id));
    args.add("--memory-name=" + getSharedMemoryName(instance.id));
    args.add("--plugin-path=" + instance.config.pluginPath);
    args.add("--sample-rate=" + juce::String(instance.config.sampleRate));
    args.add("--block-size=" + juce::String(instance.config.blockSize));

    instance.process = appFile.startChildProcess(args);
    if (!instance.process || !instance.process->isRunning())
    {
        DBG("Failed to start sandbox worker process");
        return false;
    }

    // Wait for process to signal ready
    juce::Thread::sleep(100);  // Small delay

    // Check if process is still running
    if (!instance.process->isRunning())
    {
        DBG("Sandbox worker process exited immediately");
        return false;
    }

    instance.state = SandboxState::Running;

    // Notify state change
    if (stateChangeCallback)
        stateChangeCallback(instance.id, SandboxState::Running);

    return true;
}

void PluginSandboxManager::monitorThreadFunc()
{
    DBG("Sandbox monitor thread started");

    while (monitorThreadRunning.load())
    {
        // Check all sandboxes for crashes
        for (auto& pair : sandboxes)
        {
            auto& instance = pair.second;

            // Check if process is still running
            if (instance.process && !instance.process->isRunning() &&
                instance.state == SandboxState::Running)
            {
                // Process has crashed
                handleSandboxCrash(instance);
            }

            // Monitor CPU usage
            if (instance.buffers && instance.config.enableCPUMonitoring)
            {
                uint64_t processingTime = instance.buffers->header.processingTimeNs.load(std::memory_order_relaxed);
                float cpuUsage = calculateCPUUsage(processingTime, instance.config.blockSize,
                                                 instance.config.sampleRate);
                instance.currentCPUUsage.store(cpuUsage, std::memory_order_relaxed);

                // Update CPU history (keep last 100 values)
                instance.cpuUsageHistory.push_back(cpuUsage);
                if (instance.cpuUsageHistory.size() > 100)
                    instance.cpuUsageHistory.erase(instance.cpuUsageHistory.begin());

                // Check CPU warning threshold
                if (cpuUsage > instance.config.cpuWarningThreshold && cpuWarningCallback)
                {
                    cpuWarningCallback(instance.id, cpuUsage);
                }
            }
        }

        // Sleep for 100ms between checks
        juce::Thread::sleep(100);
    }

    DBG("Sandbox monitor thread stopped");
}

void PluginSandboxManager::handleSandboxCrash(SandboxInstance& instance)
{
    instance.state = SandboxState::Crashed;
    instance.crashCount++;
    instance.lastCrashTime = juce::Time::currentTimeMillis();
    totalCrashes++;

    // Generate crash report
    CrashReport report;
    report.nodeId = instance.id;
    report.pluginName = instance.config.pluginName;
    report.crashTime = instance.lastCrashTime;
    report.restartAttempt = instance.restartAttempts + 1;
    report.autoRecovered = instance.config.enableAutoRecovery;

    if (instance.buffers)
    {
        report.errorMessage = juce::String(instance.buffers->header.lastErrorMessage);
    }

    // Notify callbacks
    if (crashCallback)
        crashCallback(report);

    if (stateChangeCallback)
        stateChangeCallback(instance.id, SandboxState::Crashed);

    // Auto-recovery if enabled
    if (instance.config.enableAutoRecovery && instance.restartAttempts < instance.config.maxRestartAttempts)
    {
        DBG("Attempting auto-recovery for sandbox " + juce::String(instance.id));
        restartSandbox(instance.id);
    }
}

float PluginSandboxManager::calculateCPUUsage(uint64_t processingTimeNs, int blockSize, double sampleRate)
{
    if (blockSize <= 0 || sampleRate <= 0)
        return 0.0f;

    // Calculate ideal processing time for this block
    double blockSizeSeconds = static_cast<double>(blockSize) / sampleRate;
    uint64_t idealTimeNs = static_cast<uint64_t>(blockSizeSeconds * 1e9);

    if (idealTimeNs == 0)
        return 0.0f;

    // CPU usage is actual / ideal (clamped to 0.0 - 1.0)
    float cpuUsage = static_cast<float>(static_cast<double>(processingTimeNs) / idealTimeNs);
    return juce::jlimit(0.0f, 1.0f, cpuUsage);
}

void PluginSandboxManager::reportCrash(const SandboxInstance& instance, const juce::String& error)
{
    DBG("CRASH REPORT - Sandbox ID: " + juce::String(instance.id));
    DBG("  Plugin: " + instance.config.pluginName);
    DBG("  Error: " + error);
    DBG("  Restart Attempts: " + juce::String(instance.restartAttempts));
}

//==============================================================================
// PluginSandboxWorker Implementation (Sandbox Process)
//==============================================================================

PluginSandboxWorker::PluginSandboxWorker()
{
}

PluginSandboxWorker::~PluginSandboxWorker()
{
    cleanupPlugin();
}

int PluginSandboxWorker::run(int argc, char* argv[])
{
    // Parse command line arguments
    uint32_t sandboxId = 0;
    juce::String memoryName;
    juce::String pluginPath;
    double sampleRate = 44100.0;
    int blockSize = 512;

    for (int i = 1; i < argc; ++i)
    {
        juce::String arg = argv[i];

        if (arg.startsWith("--sandbox-id="))
            sandboxId = arg.substring(12).getIntValue();
        else if (arg.startsWith("--memory-name="))
            memoryName = arg.substring(13);
        else if (arg.startsWith("--plugin-path="))
            pluginPath = arg.substring(13);
        else if (arg.startsWith("--sample-rate="))
            sampleRate = arg.substring(13).getDoubleValue();
        else if (arg.startsWith("--block-size="))
            blockSize = arg.substring(12).getIntValue();
    }

    currentSampleRate = sampleRate;
    currentBlockSize = blockSize;

    DBG("SandboxWorker started - ID: " + juce::String(sandboxId));
    DBG("  Memory: " + memoryName);
    DBG("  Plugin: " + pluginPath);
    DBG("  Sample Rate: " + juce::String(sampleRate));
    DBG("  Block Size: " + juce::String(blockSize));

    // Attach to shared memory
    if (!attachToSharedMemory(memoryName, DEFAULT_SHARED_MEMORY_SIZE))
    {
        DBG("Failed to attach to shared memory");
        return 1;
    }

    // Setup signal handlers for crash detection
    setupSignalHandlers();

    // Load the plugin
    SandboxConfig config;
    config.pluginPath = pluginPath;
    config.sampleRate = sampleRate;
    config.blockSize = blockSize;

    if (!loadPlugin(config))
    {
        reportCrash("Failed to load plugin: " + pluginPath);
        return 1;
    }

    running = true;

    // Main processing loop
    while (running)
    {
        // Check for shutdown signal
        if (buffers && buffers->header.statusFlags.load(std::memory_order_acquire) & 0x01)
        {
            DBG("Shutdown signal received");
            break;
        }

        // Process audio block
        processAudioBlock();

        // Send MIDI events
        sendMidiEvents();

        // Small sleep to prevent busy-waiting
        juce::Thread::sleep(1);
    }

    cleanupPlugin();
    DBG("SandboxWorker terminated normally");
    return 0;
}

bool PluginSandboxWorker::attachToSharedMemory(const juce::String& memoryName, size_t size)
{
#if JUCE_WINDOWS
    HANDLE hMap = OpenFileMappingA(FILE_MAP_ALL_ACCESS, false, memoryName.toStdString().c_str());
    if (!hMap)
    {
        DBG("Failed to open file mapping");
        return false;
    }

    void* ptr = MapViewOfFile(hMap, FILE_MAP_ALL_ACCESS, 0, 0, size);
    if (!ptr)
    {
        CloseHandle(hMap);
        DBG("Failed to map view of file");
        return false;
    }

    sharedMemoryHandle = hMap;
    buffers = static_cast<SharedMemoryBuffers*>(ptr);

#else
    int fd = shm_open(memoryName.toStdString().c_str(), O_RDWR, 0666);
    if (fd < 0)
    {
        DBG("Failed to open shared memory");
        return false;
    }

    void* ptr = mmap(nullptr, size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if (ptr == MAP_FAILED)
    {
        close(fd);
        DBG("Failed to mmap shared memory");
        return false;
    }

    sharedMemoryHandle = reinterpret_cast<juce::SharedMemoryHandle>(fd);
    buffers = static_cast<SharedMemoryBuffers*>(ptr);

#endif

    // Validate header
    if (buffers->header.magic != SANDBOX_MAGIC || buffers->header.version != SANDBOX_VERSION)
    {
        DBG("Invalid shared memory header");
        return false;
    }

    // Setup buffer pointers
    char* data = static_cast<char*>(ptr) + sizeof(SharedMemoryBuffers);
    buffers->audioInput = reinterpret_cast<float*>(data);
    buffers->audioOutput = buffers->audioInput + AUDIO_BUFFER_SIZE * 2;
    buffers->midiInput = reinterpret_cast<SharedMemoryBuffers::MidiEvent*>(
        data + AUDIO_BUFFER_SIZE * 4 * sizeof(float));
    buffers->midiOutput = buffers->midiInput + 256;  // Max events

    DBG("Attached to shared memory successfully");
    return true;
}

void PluginSandboxWorker::processAudioBlock()
{
    if (!buffers || !pluginInstance)
        return;

    auto startTime = std::chrono::high_resolution_clock::now();

    // Wait for input data
    int timeout = 1000;  // ~1 second
    while (!buffers->header.audioReady.load(std::memory_order_acquire) && timeout > 0)
    {
        juce::Thread::yield();
        timeout--;
    }

    if (timeout <= 0)
        return;  // Timeout, skip this block

    // Mark as processing
    buffers->header.processing.store(true, std::memory_order_release);

    // Create audio buffer from shared memory
    juce::AudioBuffer<float> buffer(2, currentBlockSize);

    // De-interleave stereo data
    int samplesToProcess = juce::jmin(currentBlockSize, AUDIO_BUFFER_SIZE);
    for (int i = 0; i < samplesToProcess; ++i)
    {
        buffer.setSample(0, i, buffers->audioInput[i * 2]);
        buffer.setSample(1, i, buffers->audioInput[i * 2 + 1]);
    }

    // Create MIDI buffer
    juce::MidiBuffer midiBuffer;
    for (int i = 0; i < buffers->midiInputCount; ++i)
    {
        const auto& event = buffers->midiInput[i];
        midiBuffer.addEvent(event.data, event.messageSize, event.sampleOffset);
    }

    // Process through plugin
    processPlugin();

    // Copy output to shared memory
    for (int i = 0; i < samplesToProcess; ++i)
    {
        buffers->audioOutput[i * 2] = buffer.getSample(0, i);
        buffers->audioOutput[i * 2 + 1] = buffer.getSample(1, i);
    }

    buffers->audioReady.store(true, std::memory_order_release);
    buffers->header.audioReady.store(false, std::memory_order_release);
    buffers->header.processing.store(false, std::memory_order_release);

    // Update processing time for CPU monitoring
    auto endTime = std::chrono::high_resolution_clock::now();
    uint64_t durationNs = std::chrono::duration_cast<std::chrono::nanoseconds>(endTime - startTime).count();
    buffers->header.processingTimeNs.store(durationNs, std::memory_order_release);
}

void PluginSandboxWorker::sendMidiEvents()
{
    if (!buffers)
        return;

    // MIDI output would be collected from plugin
    // and written to buffers->midiOutput
    // For this basic implementation, we'll skip this
}

bool PluginSandboxWorker::loadPlugin(const SandboxConfig& config)
{
    // TODO: Implement actual plugin loading
    // This would involve using juce::AudioPluginFormatManager
    // to load the plugin at config.pluginPath

    DBG("Loading plugin: " + config.pluginPath);

    // For now, just return success
    return true;
}

void PluginSandboxWorker::processPlugin()
{
    if (!pluginInstance || !buffers)
        return;

    juce::AudioBuffer<float> buffer(2, currentBlockSize);

    // Process through plugin
    juce::MidiBuffer midiBuffer;
    pluginInstance->processBlock(buffer, midiBuffer);
}

void PluginSandboxWorker::cleanupPlugin()
{
    running = false;

    if (pluginInstance)
    {
        pluginInstance->releaseResources();
        pluginInstance.reset();
    }

    if (buffers)
    {
        // Write final status
        std::strncpy(buffers->header.lastErrorMessage, "Sandbox shutdown", sizeof(buffers->header.lastErrorMessage) - 1);
    }
}

void PluginSandboxWorker::setupSignalHandlers()
{
    // Setup signal handlers for common crash signals
    // This is platform-specific

#if JUCE_LINUX || JUCE_MAC
    // Signal handlers for SIGSEGV, SIGABRT, SIGFPE, etc.
    struct sigaction sa;
    sa.sa_handler = [](int sig) {
        // Write crash info to shared memory
        // and terminate gracefully
        exit(1);
    };
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;

    sigaction(SIGSEGV, &sa, &oldSigAction[0]);
    sigaction(SIGABRT, &sa, &oldSigAction[1]);
    sigaction(SIGFPE, &sa, &oldSigAction[2]);
    sigaction(SIGBUS, &sa, &oldSigAction[3]);
#endif

    DBG("Signal handlers configured");
}

void PluginSandboxWorker::handleCrash(int signal)
{
    if (crashHandled)
        return;

    crashHandled = true;

    juce::String message = "Signal: " + juce::String(signal);
    reportCrash(message);

    // Terminate
    exit(1);
}

void PluginSandboxWorker::reportCrash(const juce::String& message)
{
    if (!buffers)
        return;

    // Write crash info to shared memory
    std::strncpy(buffers->header.lastErrorMessage, message.toStdString().c_str(),
                 sizeof(buffers->header.lastErrorMessage) - 1);

    // Set error code
    buffers->header.lastErrorCode = 1;

    DBG("Crash reported: " + message);
}
