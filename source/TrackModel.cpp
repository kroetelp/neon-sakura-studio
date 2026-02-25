#include "TrackModel.h"

TrackModel::TrackModel()
{
    // Initialize all bank steps to Off state
    for (int bank = 0; bank < numBanks; ++bank)
    {
        for (int step = 0; step < stepsPerBank; ++step)
        {
            bankSteps[bank][step] = StepModifierState{false, ' ', 1};
        }
    }
}

void TrackModel::setStepActive(int step, bool active)
{
    jassert(step >= 0 && step < totalSteps);
    const int bank = step / stepsPerBank;
    const int bankStep = step % stepsPerBank;
    bankSteps[bank][bankStep].active = active;
    if (!active)
    {
        bankSteps[bank][bankStep].modifierType = ' ';
        bankSteps[bank][bankStep].modifierValue = 1;
    }
}

bool TrackModel::isStepActive(int step) const
{
    jassert(step >= 0 && step < totalSteps);
    const int bank = step / stepsPerBank;
    const int bankStep = step % stepsPerBank;
    return bankSteps[bank][bankStep].active;
}

StepModifierState TrackModel::getStepState(int step) const
{
    jassert(step >= 0 && step < totalSteps);
    const int bank = step / stepsPerBank;
    const int bankStep = step % stepsPerBank;
    return bankSteps[bank][bankStep];
}

void TrackModel::setStepState(int step, const StepModifierState& state)
{
    jassert(step >= 0 && step < totalSteps);
    const int bank = step / stepsPerBank;
    const int bankStep = step % stepsPerBank;
    bankSteps[bank][bankStep] = state;
}

void TrackModel::setStepModifier(int step, char modifierType, int modifierValue)
{
    jassert(step >= 0 && step < totalSteps);
    const int bank = step / stepsPerBank;
    const int bankStep = step % stepsPerBank;
    bankSteps[bank][bankStep].active = true;
    bankSteps[bank][bankStep].modifierType = modifierType;
    bankSteps[bank][bankStep].modifierValue = modifierValue;
}

int TrackModel::getModifierValue(int step) const
{
    jassert(step >= 0 && step < totalSteps);
    const int bank = step / stepsPerBank;
    const int bankStep = step % stepsPerBank;
    return bankSteps[bank][bankStep].modifierValue;
}

char TrackModel::getModifierType(int step) const
{
    jassert(step >= 0 && step < totalSteps);
    const int bank = step / stepsPerBank;
    const int bankStep = step % stepsPerBank;
    return bankSteps[bank][bankStep].modifierType;
}

void TrackModel::clearAllSteps()
{
    for (int bank = 0; bank < numBanks; ++bank)
    {
        for (int step = 0; step < stepsPerBank; ++step)
        {
            bankSteps[bank][step] = StepModifierState{false, ' ', 1};
        }
    }
}

void TrackModel::setCurrentSampleFiles(const juce::Array<juce::File>& files)
{
    currentSampleFiles = files;
}

void TrackModel::setCurrentSampleIndex(int index)
{
    if (index >= 0 && index < currentSampleFiles.size())
        currentSampleIndex = index;
}

juce::File TrackModel::getCurrentSampleFile() const
{
    if (currentSampleIndex >= 0 && currentSampleIndex < currentSampleFiles.size())
        return currentSampleFiles[currentSampleIndex];
    return {};
}

void TrackModel::setBank(int bank)
{
    jassert(bank >= 0 && bank < numBanks);
    currentBank = bank;
}
