#include "PluginParameterAutomation.h"
#include <juce_core/juce_core.h>
#include <algorithm>

//==============================================================================
// PluginParameterLane Implementation
//==============================================================================
PluginParameterLane::PluginParameterLane(const juce::String& paramId)
    : parameterId(paramId)
{
}

float PluginParameterLane::getValueAtBeat(double beatPosition) const
{
    if (!isActive || clips.empty())
        return parameterInfo.defaultValue;

    // Find active clip at this position
    const AutomationClip* activeClip = findActiveClip(beatPosition);
    if (!activeClip)
        return parameterInfo.defaultValue;

    // Normalize position within clip
    double normalizedPos = (beatPosition - activeClip->startBeat) /
                         juce::jmax(activeClip->endBeat - activeClip->startBeat, 0.001);

    // Interpolate value based on curve
    if (activeClip->curve == 0.0f)
    {
        // Linear interpolation
        float t = static_cast<float>(juce::jlimit(normalizedPos, 0.0, 1.0));
        return activeClip->startValue * (1.0f - t) + activeClip->endValue * t;
    }
    else if (activeClip->curve > 0.0f && activeClip->curve < 1.0f)
    {
        // Bezier interpolation (simplified ease-in/out)
        float t = static_cast<float>(juce::jlimit(normalizedPos, 0.0, 1.0));
        float tension = activeClip->curve;  // 0-1 range

        // Ease-in curve
        float ease = t * t * (3.0f - 2.0f * t);
        return activeClip->startValue + (activeClip->endValue - activeClip->startValue) * ease;
    }
    else
    {
        // Step interpolation (quantized)
        int numSteps = 8;  // 1/8 note resolution
        float stepValue = juce::roundToInt(normalizedPos * numSteps) / static_cast<float>(numSteps);
        return activeClip->startValue * (1.0f - stepValue) + activeClip->endValue * stepValue;
    }
}

void PluginParameterLane::addClip(double startBeat, double endBeat, float startValue, float endValue, float curve)
{
    AutomationClip clip;
    clip.startBeat = startBeat;
    clip.endBeat = endBeat;
    clip.startValue = startValue;
    clip.endValue = endValue;
    clip.curve = curve;

    clips.push_back(clip);
}

void PluginParameterLane::removeClip(int clipIndex)
{
    if (clipIndex >= 0 && clipIndex < static_cast<int>(clips.size()))
    {
        clips.erase(clips.begin() + clipIndex);
    }
}

const PluginParameterLane::AutomationClip* PluginParameterLane::findActiveClip(double beatPosition) const
{
    for (const auto& clip : clips)
    {
        if (beatPosition >= clip.startBeat && beatPosition < clip.endBeat)
            return &clip;
    }
    return nullptr;
}

//==============================================================================
// PluginParameterAutomation Implementation
//==============================================================================
PluginParameterAutomation::PluginParameterAutomation()
{
}

PluginParameterAutomation::~PluginParameterAutomation()
{
}

void PluginParameterAutomation::scanPlugins(const std::unordered_map<uint32_t, juce::AudioPluginInstance*>& plugins)
{
    pluginParameters.clear();

    // TODO: JUCE 8.0 Parameter API has changed significantly
    // - AudioProcessorParameter fields are no longer directly accessible (name, textValue, category, etc.)
    // - Need to use methods like getName(), getLabel(), getDefaultValue(), getNumSteps(), isDiscrete()
    // - For now, we'll use available methods and skip fields that don't exist

    for (const auto& [nodeId, plugin] : plugins)
    {
        if (!plugin)
            continue;

        std::vector<PluginParameterInfo> params;

        // Scan all parameters
        for (int i = 0; i < plugin->getParameters().size(); ++i)
        {
            const auto* param = plugin->getParameters()[i];

            // Check if parameter is automatable
            if (!param->isAutomatable())
                continue;

            PluginParameterInfo info;
            info.parameterId = createParameterId(nodeId, i);
            info.name = param->getName(100);
            info.label = param->getLabel();
            info.defaultValue = param->getDefaultValue();
            info.stepSize = static_cast<float>(param->getNumSteps());
            info.isAutomatable = true;
            info.isDiscrete = param->isDiscrete();

            params.push_back(info);
        }

        if (!params.empty())
            pluginParameters[nodeId] = params;

        DBG("Plugin " + plugin->getName() + " scanned: " +
            juce::String(params.size()) + " automatable parameters");
    }
}

std::vector<PluginParameterInfo> PluginParameterAutomation::getPluginParameters(uint32_t pluginNodeId) const
{
    auto it = pluginParameters.find(pluginNodeId);
    if (it != pluginParameters.end())
        return it->second;
    return {};
}

const PluginParameterInfo* PluginParameterAutomation::getParameterInfo(uint32_t pluginNodeId, int paramIndex) const
{
    auto params = getPluginParameters(pluginNodeId);
    if (paramIndex >= 0 && paramIndex < static_cast<int>(params.size()))
        return &params[paramIndex];
    return nullptr;
}

juce::String PluginParameterAutomation::createParameterId(uint32_t pluginNodeId, int paramIndex) const
{
    return "plugin_" + juce::String(pluginNodeId) + "_param_" + juce::String(paramIndex);
}

//==============================================================================
// Mapping Management
//==============================================================================
int PluginParameterAutomation::addMapping(const PluginParameterMapping& mapping)
{
    if (!mapping.isValid())
        return -1;

    // Check if mapping already exists
    for (const auto& existing : mappings)
    {
        if (existing.automationParameterId == mapping.automationParameterId)
            return -1;  // Already exists
    }

    // Create new mapping with unique ID
    PluginParameterMapping newMapping;
    newMapping.automationParameterId = mapping.automationParameterId;
    newMapping.pluginNodeId = mapping.pluginNodeId;
    newMapping.pluginParameterIndex = mapping.pluginParameterIndex;
    newMapping.gain = mapping.gain;
    newMapping.minOverride = mapping.minOverride;
    newMapping.maxOverride = mapping.maxOverride;
    newMapping.enabled = mapping.enabled;
    newMapping.pluginParameterIndex = static_cast<int>(nextMappingId++);

    mappings.push_back(newMapping);

    // Create automation lane if not exists
    if (lanes.find(newMapping.automationParameterId) == lanes.end())
    {
        lanes[newMapping.automationParameterId] = std::make_unique<PluginParameterLane>(newMapping.automationParameterId);
    }

    DBG("Plugin parameter mapping added: " + newMapping.toString());

    return nextMappingId - 1;
}

bool PluginParameterAutomation::removeMapping(int mappingId)
{
    auto it = std::find_if(mappings.begin(), mappings.end(),
        [mappingId](const PluginParameterMapping& m) {
            return m.pluginParameterIndex == mappingId;
        });

    if (it != mappings.end())
    {
        juce::String paramId = it->automationParameterId;

        // Remove lane
        lanes.erase(paramId);

        // Remove mapping
        mappings.erase(it);

        DBG("Plugin parameter mapping removed: " + paramId);

        return true;
    }

    return false;
}

bool PluginParameterAutomation::updateMapping(int mappingId, const PluginParameterMapping& mapping)
{
    auto* m = const_cast<PluginParameterMapping*>(getMapping(mappingId));
    if (!m)
        return false;

    // Update all fields except ID
    m->pluginNodeId = mapping.pluginNodeId;
    m->pluginParameterIndex = mapping.pluginParameterIndex;
    m->gain = mapping.gain;
    m->minOverride = mapping.minOverride;
    m->maxOverride = mapping.maxOverride;

    DBG("Plugin parameter mapping updated: " + mapping.toString());

    return true;
}

const PluginParameterMapping* PluginParameterAutomation::getMapping(int mappingId) const
{
    auto it = std::find_if(mappings.begin(), mappings.end(),
        [mappingId](const PluginParameterMapping& m) {
            return m.pluginParameterIndex == mappingId;
        });

    return (it != mappings.end()) ? &(*it) : nullptr;
}

std::vector<PluginParameterMapping*> PluginParameterAutomation::getMappingsForPlugin(uint32_t pluginNodeId)
{
    std::vector<PluginParameterMapping*> result;

    for (auto& mapping : mappings)
    {
        if (mapping.pluginNodeId == pluginNodeId && mapping.isValid() && mapping.enabled)
            result.push_back(&mapping);
    }

    return result;
}

//==============================================================================
// Value Processing
//==============================================================================
std::unordered_map<uint32_t, std::unordered_map<int, float>> PluginParameterAutomation::processAutomation(
    double currentBeat)
{
    if (!automationEnabled.load())
        return {};

    std::unordered_map<uint32_t, std::unordered_map<int, float>> result;

    for (const auto& mapping : mappings)
    {
        if (!mapping.enabled || !mapping.isValid())
            continue;

        // Get lane for this parameter
        auto it = lanes.find(mapping.automationParameterId);
        if (it == lanes.end())
            continue;

        const auto& lane = *it->second;

        // Get automation value
        float rawValue = lane.getValueAtBeat(currentBeat);

        // Apply gain
        float value = rawValue * mapping.gain;

        // Apply min/max overrides (if parameter info available)
        // Note: minValue/maxValue may not be available in JUCE 8.0 without additional work
        const auto& paramInfo = lane.getParameterInfo();
        if (mapping.minOverride != 0.0f || mapping.maxOverride != 1.0f)
        {
            value = juce::jlimit(value,
                                 mapping.minOverride != 0.0f ? mapping.minOverride : paramInfo.minValue,
                                 mapping.maxOverride != 1.0f ? mapping.maxOverride : paramInfo.maxValue);
        }

        // Add to result for this plugin
        result[mapping.pluginNodeId][mapping.pluginParameterIndex] = value;
    }

    return result;
}

float PluginParameterAutomation::getParameterValue(uint32_t pluginNodeId, int paramIndex, double beat, float defaultValue) const
{
    if (!automationEnabled.load())
        return defaultValue;

    // Find mapping for this parameter
    for (const auto& mapping : mappings)
    {
        if (!mapping.enabled || !mapping.isValid())
            continue;

        if (mapping.pluginNodeId == pluginNodeId && mapping.pluginParameterIndex == paramIndex)
        {
            // Get lane for this parameter
            auto it = lanes.find(mapping.automationParameterId);
            if (it == lanes.end())
                return defaultValue;

            const auto& lane = *it->second;
            float rawValue = lane.getValueAtBeat(beat);

            // Apply gain
            float value = rawValue * mapping.gain;

            // Apply min/max overrides (if parameter info available)
            const auto& paramInfo = lane.getParameterInfo();
            if (mapping.minOverride != 0.0f || mapping.maxOverride != 1.0f)
            {
                return juce::jlimit(value,
                                     mapping.minOverride != 0.0f ? mapping.minOverride : paramInfo.minValue,
                                     mapping.maxOverride != 1.0f ? mapping.maxOverride : paramInfo.maxValue);
            }

            return value;
        }
    }

    return defaultValue;
}

//==============================================================================
// Bypass Management
//==============================================================================
void PluginParameterAutomation::setEnabled(bool enabled)
{
    automationEnabled = enabled;
    DBG("Plugin automation " + juce::String(enabled ? "enabled" : "disabled"));
}

bool PluginParameterAutomation::setMappingEnabled(int mappingId, bool enabled)
{
    const PluginParameterMapping* mapping = getMapping(mappingId);
    if (mapping)
    {
        const_cast<PluginParameterMapping*>(mapping)->enabled = enabled;
        DBG("Plugin mapping " + juce::String(mappingId) + " " + (enabled ? "enabled" : "disabled"));
        return true;
    }
    return false;
}

bool PluginParameterAutomation::isMappingEnabled(int mappingId) const
{
    const PluginParameterMapping* mapping = getMapping(mappingId);
    return mapping ? mapping->enabled : false;
}

//==============================================================================
// Clear
//==============================================================================
void PluginParameterAutomation::clearMappings()
{
    mappings.clear();
    clearLanes();
}

void PluginParameterAutomation::clearLanes()
{
    lanes.clear();
}

//==============================================================================
// Debug
//==============================================================================
juce::String PluginParameterAutomation::getDebugInfo() const
{
    juce::String info = "PluginParameterAutomation Debug Info:\n";
    info += "=====================================\n";
    info += "Automation Enabled: " + juce::String(automationEnabled.load() ? "Yes" : "No") + "\n";
    info += "Total Mappings: " + juce::String(mappings.size()) + "\n";
    info += "Active Mappings: " + juce::String(std::count_if(mappings.begin(), mappings.end(),
        [](const PluginParameterMapping& m) { return m.enabled && m.isValid(); })) + "\n\n";

    info += "Plugin Parameters Discovered:\n";
    for (const auto& [nodeId, params] : pluginParameters)
    {
        info += "  Plugin " + juce::String(nodeId) + ": " + juce::String(params.size()) + " automatable parameters\n";
        for (const auto& param : params)
        {
            info += "    " + param.name + " (" + param.parameterId + ")\n";
        }
    }

    info += "\nActive Mappings:\n";
    for (const auto& mapping : mappings)
    {
        if (mapping.enabled && mapping.isValid())
            info += "  " + mapping.toString() + "\n";
    }

    return info;
}
