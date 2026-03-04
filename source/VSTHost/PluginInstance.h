#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <memory>
#include <atomic>

/**
 * PluginInstance - Wrapper for a loaded plugin
 *
 * Manages a single plugin instance with:
 *   - Bypass state
 *   - Parameter access
 *   - State save/restore
 *   - UI creation
 */
class PluginInstance
{
public:
    explicit PluginInstance(std::unique_ptr<juce::AudioPluginInstance> plugin);
    ~PluginInstance();

    // ============================================================
    // Plugin Access
    // ============================================================

    /** Get the underlying AudioPluginInstance. */
    juce::AudioPluginInstance* getAudioPluginInstance() { return plugin.get(); }
    const juce::AudioPluginInstance* getAudioPluginInstance() const { return plugin.get(); }

    /** Release ownership of the plugin (for transfer to AudioProcessorGraph). */
    std::unique_ptr<juce::AudioPluginInstance> releasePlugin() { return std::move(plugin); }

    /** Get the plugin's name. */
    juce::String getName() const { return plugin ? plugin->getName() : "Unknown"; }

    /** Get the plugin's description. */
    juce::PluginDescription getDescription() const;

    // ============================================================
    // Bypass
    // ============================================================

    bool isBypassed() const { return bypassed.load(); }
    void setBypassed(bool shouldBypass) { bypassed.store(shouldBypass); }

    // ============================================================
    // Parameters (Modern AudioProcessorParameter API)
    // ============================================================

    /** Get the number of parameters. */
    int getNumParameters() const;

    /** Get parameter by index (modern API). */
    juce::AudioProcessorParameter* getParameter(int index);
    const juce::AudioProcessorParameter* getParameter(int index) const;

    /** Get parameter by ID (modern API). */
    juce::AudioProcessorParameter* getParameterByID(const juce::String& parameterID);
    const juce::AudioProcessorParameter* getParameterByID(const juce::String& parameterID) const;

    /** Get all parameters as a list. */
    juce::Array<juce::AudioProcessorParameter*> getParameters() const;

    /** Get parameter name. */
    juce::String getParameterName(int index) const;

    /** Get parameter ID (for modern parameter identification). */
    juce::String getParameterID(int index) const;

    /** Get parameter value (0.0 to 1.0). */
    float getParameterValue(int index) const;

    /** Get parameter value by ID. */
    float getParameterValueByID(const juce::String& parameterID) const;

    /** Set parameter value. */
    void setParameterValue(int index, float value);

    /** Set parameter value by ID. */
    void setParameterValueByID(const juce::String& parameterID, float value);

    /** Get parameter value as text (e.g., "0.5" or "Cutoff: 500Hz"). */
    juce::String getParameterValueAsText(int index) const;

    /** Get all parameters as XML for saving. */
    std::unique_ptr<juce::XmlElement> getParametersAsXml() const;

    /** Set parameters from XML. */
    void setParametersFromXml(const juce::XmlElement& xml);

    // ============================================================
    // State (for DAW project save/load)
    // ============================================================

    /** Get the plugin's state as a memory block. */
    void getState(juce::MemoryBlock& destData) const;

    /** Restore the plugin's state from a memory block. */
    void setState(const void* data, int sizeInBytes);

    // ============================================================
    // Audio Processing
    // ============================================================

    /** Prepare the plugin for playback. */
    void prepareToPlay(double sampleRate, int blockSize);

    /** Release resources. */
    void releaseResources();

    /** Process audio block. */
    void processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages);

    // ============================================================
    // Editor
    // ============================================================

    /** Check if the plugin has an editor. */
    bool hasEditor() const { return plugin && plugin->hasEditor(); }

    /** Create the plugin's editor. Returns nullptr if no editor available. */
    juce::AudioProcessorEditor* createEditor();

private:
    std::unique_ptr<juce::AudioPluginInstance> plugin;
    std::atomic<bool> bypassed{false};

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PluginInstance)
};
