#include "PluginInstance.h"

//==============================================================================
PluginInstance::PluginInstance(std::unique_ptr<juce::AudioPluginInstance> plugin_)
    : plugin(std::move(plugin_))
{
}

PluginInstance::~PluginInstance()
{
    if (plugin)
        plugin->releaseResources();
}

//==============================================================================
juce::PluginDescription PluginInstance::getDescription() const
{
    juce::PluginDescription desc;
    if (plugin)
        plugin->fillInPluginDescription(desc);
    return desc;
}

//==============================================================================
int PluginInstance::getNumParameters() const
{
    if (!plugin)
        return 0;

    // Use modern parameter count
    return static_cast<int>(plugin->getParameters().size());
}

//==============================================================================
juce::AudioProcessorParameter* PluginInstance::getParameter(int index)
{
    if (!plugin || index < 0)
        return nullptr;

    const auto& params = plugin->getParameters();
    if (index >= params.size())
        return nullptr;

    return params[index];
}

//==============================================================================
const juce::AudioProcessorParameter* PluginInstance::getParameter(int index) const
{
    if (!plugin || index < 0)
        return nullptr;

    const auto& params = plugin->getParameters();
    if (index >= params.size())
        return nullptr;

    return params[index];
}

//==============================================================================
juce::AudioProcessorParameter* PluginInstance::getParameterByID(const juce::String& parameterID)
{
    if (!plugin)
        return nullptr;

    // Search through parameters by index (parameterID is the index as string)
    int index = parameterID.getIntValue();
    if (index >= 0 && index < getNumParameters())
        return getParameter(index);

    return nullptr;
}

//==============================================================================
const juce::AudioProcessorParameter* PluginInstance::getParameterByID(const juce::String& parameterID) const
{
    if (!plugin)
        return nullptr;

    int index = parameterID.getIntValue();
    if (index >= 0 && index < getNumParameters())
        return getParameter(index);

    return nullptr;
}

//==============================================================================
juce::Array<juce::AudioProcessorParameter*> PluginInstance::getParameters() const
{
    if (!plugin)
        return {};

    juce::Array<juce::AudioProcessorParameter*> result;
    for (auto* param : plugin->getParameters())
        result.add(param);
    return result;
}

//==============================================================================
juce::String PluginInstance::getParameterName(int index) const
{
    auto* param = getParameter(index);
    return param ? param->getName(512) : juce::String();
}

//==============================================================================
juce::String PluginInstance::getParameterID(int index) const
{
    // In JUCE 8.0, use index as ID string
    return juce::String(index);
}

//==============================================================================
float PluginInstance::getParameterValue(int index) const
{
    auto* param = getParameter(index);
    return param ? param->getValue() : 0.0f;
}

//==============================================================================
float PluginInstance::getParameterValueByID(const juce::String& parameterID) const
{
    auto* param = getParameterByID(parameterID);
    return param ? param->getValue() : 0.0f;
}

//==============================================================================
void PluginInstance::setParameterValue(int index, float value)
{
    auto* param = getParameter(index);
    if (param)
        param->setValue(value);
}

//==============================================================================
void PluginInstance::setParameterValueByID(const juce::String& parameterID, float value)
{
    auto* param = getParameterByID(parameterID);
    if (param)
        param->setValue(value);
}

//==============================================================================
juce::String PluginInstance::getParameterValueAsText(int index) const
{
    auto* param = getParameter(index);
    return param ? param->getCurrentValueAsText() : juce::String();
}

//==============================================================================
std::unique_ptr<juce::XmlElement> PluginInstance::getParametersAsXml() const
{
    if (!plugin)
        return nullptr;

    auto xml = std::make_unique<juce::XmlElement>("PARAMETERS");

    // Use modern AudioProcessorParameter API
    for (auto* param : plugin->getParameters())
    {
        // Skip non-automatable or meta parameters if needed
        if (param->isMetaParameter())
            continue;

        auto* paramElement = xml->createNewChildElement("PARAM");
        paramElement->setAttribute("id", juce::String(param->getParameterIndex()));
        paramElement->setAttribute("name", param->getName(512));
        paramElement->setAttribute("value", param->getValue());

        // Also store text value for better compatibility
        paramElement->setAttribute("textValue", param->getCurrentValueAsText());
    }

    return xml;
}

//==============================================================================
void PluginInstance::setParametersFromXml(const juce::XmlElement& xml)
{
    if (!plugin)
        return;

    for (auto* paramElement : xml.getChildIterator())
    {
        if (paramElement->getTagName() != "PARAM")
            continue;

        // Try ID first (modern approach)
        juce::String paramID = paramElement->getStringAttribute("id");
        float value = static_cast<float>(paramElement->getDoubleAttribute("value"));

        if (paramID.isNotEmpty())
        {
            if (auto* param = getParameterByID(paramID))
            {
                param->setValue(value);
                continue;
            }
        }

        // Fallback to name-based lookup (legacy approach)
        auto name = paramElement->getStringAttribute("name");
        if (name.isNotEmpty())
        {
            for (auto* param : plugin->getParameters())
            {
                if (param->getName(512) == name)
                {
                    param->setValue(value);
                    break;
                }
            }
        }
    }
}

//==============================================================================
void PluginInstance::getState(juce::MemoryBlock& destData) const
{
    if (plugin)
        plugin->getStateInformation(destData);
}

void PluginInstance::setState(const void* data, int sizeInBytes)
{
    if (plugin)
        plugin->setStateInformation(data, sizeInBytes);
}

//==============================================================================
void PluginInstance::prepareToPlay(double sampleRate, int blockSize)
{
    if (plugin)
        plugin->prepareToPlay(sampleRate, blockSize);
}

void PluginInstance::releaseResources()
{
    if (plugin)
        plugin->releaseResources();
}

void PluginInstance::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    if (!plugin || bypassed.load())
        return;

    plugin->processBlock(buffer, midiMessages);
}

//==============================================================================
juce::AudioProcessorEditor* PluginInstance::createEditor()
{
    if (plugin && plugin->hasEditor())
        return plugin->createEditor();
    return nullptr;
}
