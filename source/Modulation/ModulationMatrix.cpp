#include "ModulationMatrix.h"

ModulationMatrix::ModulationMatrix()
{
    targetValues.fill(0.0f);
}

int ModulationMatrix::addRouting(ModulationSource source, ModulationTarget target, float amount, bool bipolar)
{
    // Check if this routing already exists
    int existingIndex = findRouting(source, target);
    if (existingIndex >= 0)
    {
        // Update existing routing
        routings[existingIndex].amount = amount;
        routings[existingIndex].bipolar = bipolar;
        routings[existingIndex].active = true;
        return existingIndex;
    }

    // Add new routing
    if (static_cast<int>(routings.size()) >= maxRoutings)
        return -1;

    ModulationRouting routing;
    routing.source = source;
    routing.target = target;
    routing.amount = amount;
    routing.bipolar = bipolar;
    routing.active = true;

    routings.push_back(routing);
    return static_cast<int>(routings.size() - 1);
}

void ModulationMatrix::removeRouting(int index)
{
    if (index >= 0 && index < static_cast<int>(routings.size()))
    {
        routings.erase(routings.begin() + index);
    }
}

void ModulationMatrix::clearAllRoutings()
{
    routings.clear();
    targetValues.fill(0.0f);
}

void ModulationMatrix::setRoutingAmount(int index, float amount)
{
    if (index >= 0 && index < static_cast<int>(routings.size()))
    {
        routings[index].amount = juce::jlimit(-1.0f, 1.0f, amount);
        routings[index].active = std::abs(amount) > 0.001f;
    }
}

void ModulationMatrix::setRoutingBipolar(int index, bool bipolar)
{
    if (index >= 0 && index < static_cast<int>(routings.size()))
    {
        routings[index].bipolar = bipolar;
    }
}

void ModulationMatrix::setRoutingActive(int index, bool active)
{
    if (index >= 0 && index < static_cast<int>(routings.size()))
    {
        routings[index].active = active;
    }
}

const ModulationRouting& ModulationMatrix::getRouting(int index) const
{
    static ModulationRouting emptyRouting;
    if (index >= 0 && index < static_cast<int>(routings.size()))
        return routings[index];
    return emptyRouting;
}

int ModulationMatrix::findRouting(ModulationSource source, ModulationTarget target) const
{
    for (int i = 0; i < static_cast<int>(routings.size()); ++i)
    {
        if (routings[i].source == source && routings[i].target == target)
            return i;
    }
    return -1;
}

float ModulationMatrix::getModulationValue(ModulationTarget target) const
{
    int targetIndex = static_cast<int>(target);
    if (targetIndex >= 0 && targetIndex < static_cast<int>(ModulationTarget::Count))
        return targetValues[targetIndex];
    return 0.0f;
}

void ModulationMatrix::process()
{
    // Reset target values
    targetValues.fill(0.0f);

    if (!getModulatorValue)
        return;

    // Sum up all modulations for each target
    for (const auto& routing : routings)
    {
        if (!routing.active || std::abs(routing.amount) < 0.001f)
            continue;

        // Get source value
        float sourceValue = getModulatorValue(routing.source);

        // Convert to bipolar if needed
        if (!routing.bipolar)
        {
            // Source is 0-1, convert to -1 to 1 for bipolar targets
            sourceValue = sourceValue * 2.0f - 1.0f;
        }

        // Apply amount and add to target
        int targetIndex = static_cast<int>(routing.target);
        if (targetIndex >= 0 && targetIndex < static_cast<int>(ModulationTarget::Count))
        {
            targetValues[targetIndex] += sourceValue * routing.amount;
        }
    }
}
