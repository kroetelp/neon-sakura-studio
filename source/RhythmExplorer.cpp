#include "RhythmExplorer.h"
#include <algorithm>
#include <cmath>

//==============================================================================
// Euclidean Rhythm Implementation
//==============================================================================

std::vector<bool> EuclideanRhythm::generate(int pulses, int steps, int offset)
{
    if (pulses <= 0 || steps <= 0 || pulses > steps)
        return std::vector<bool>(steps, false);

    // Bjorklund's algorithm
    std::vector<int> bucket;
    std::vector<std::vector<int>> pattern;

    // Initialize with pulses and remainders
    int divisor = steps - pulses;
    pattern.resize(pulses, {1});

    int level = 0;
    bucket.clear();

    while (divisor > 0 && level < static_cast<int>(pattern.size()))
    {
        bucket.clear();

        for (int i = 0; i < divisor && i < static_cast<int>(pattern.size()); ++i)
        {
            pattern[i].push_back(0);
            bucket.push_back(i);
        }

        if (bucket.size() < static_cast<size_t>(divisor))
        {
            // Need to concatenate
            int numReps = divisor / static_cast<int>(bucket.size());
            int remainder = divisor % static_cast<int>(bucket.size());

            std::vector<std::vector<int>> newPattern;
            size_t idx = 0;

            while (idx < pattern.size())
            {
                newPattern.push_back(pattern[idx]);
                for (int r = 0; r < numReps && (idx + r + 1) < pattern.size(); ++r)
                {
                    for (int val : pattern[idx + r + 1])
                        newPattern.back().push_back(val);
                }
                idx += numReps + 1;
                if (remainder > 0 && idx < pattern.size())
                {
                    for (int val : pattern[idx])
                        newPattern.back().push_back(val);
                    --remainder;
                    ++idx;
                }
            }

            pattern = newPattern;
        }

        divisor = divisor - static_cast<int>(bucket.size());
        level++;
    }

    // Flatten pattern
    std::vector<bool> result;
    for (const auto& group : pattern)
    {
        for (int val : group)
            result.push_back(val == 1);
    }

    // Ensure correct size
    result.resize(steps, false);

    // Apply rotation (offset)
    if (offset != 0)
    {
        std::rotate(result.begin(), result.begin() + (offset % steps), result.end());
    }

    return result;
}

//==============================================================================
// RhythmExplorer Implementation
//==============================================================================

RhythmExplorer::RhythmExplorer()
{
    initializePresets();

    // Setup sliders
    setupSlider(pulseSlider, 1, 16, 4);
    addAndMakeVisible(pulseLabel);
    pulseLabel.setText("Pulses", juce::dontSendNotification);
    pulseLabel.setColour(juce::Label::textColourId, juce::Colours::white);
    pulseLabel.setFont(juce::Font(11.0f));
    pulseLabel.attachToComponent(&pulseSlider, false);
    pulseSlider.onValueChange = [this] {
        currentPulses = static_cast<int>(pulseSlider.getValue());
        generateEuclidean();
    };

    setupSlider(stepSlider, 4, 16, 16);
    addAndMakeVisible(stepLabel);
    stepLabel.setText("Steps", juce::dontSendNotification);
    stepLabel.setColour(juce::Label::textColourId, juce::Colours::white);
    stepLabel.setFont(juce::Font(11.0f));
    stepLabel.attachToComponent(&stepSlider, false);
    stepSlider.onValueChange = [this] {
        currentSteps = static_cast<int>(stepSlider.getValue());
        generateEuclidean();
    };

    setupSlider(offsetSlider, 0, 15, 0);
    addAndMakeVisible(offsetLabel);
    offsetLabel.setText("Rotate", juce::dontSendNotification);
    offsetLabel.setColour(juce::Label::textColourId, juce::Colours::white);
    offsetLabel.setFont(juce::Font(11.0f));
    offsetLabel.attachToComponent(&offsetSlider, false);
    offsetSlider.onValueChange = [this] {
        currentOffset = static_cast<int>(offsetSlider.getValue());
        generateEuclidean();
    };

    // Action buttons
    setupButton(generateButton, "Generate", getNeonCyan());
    generateButton.onClick = [this] { generateEuclidean(); };

    setupButton(mutateButton, "Mutate", getNeonPurple());
    mutateButton.onClick = [this] { mutatePattern(); };

    setupButton(humanizeButton, "Humanize", getNeonGreen());
    humanizeButton.onClick = [this] { applyVariation(); };

    setupButton(clearButton, "Clear", getNeonPink());
    clearButton.onClick = [this] {
        if (onApplyPattern)
            onApplyPattern(targetTrack, {}, true);
        currentPattern.assign(16, false);
        updatePreview(currentPattern);
    };

    // Preset buttons
    setupButton(kickPresetBtn, "Kick", getNeonPink());
    kickPresetBtn.onClick = [this] { loadPreset(0); };

    setupButton(snarePresetBtn, "Snare", getNeonCyan());
    snarePresetBtn.onClick = [this] { loadPreset(1); };

    setupButton(hihatPresetBtn, "HiHat", getNeonGreen());
    hihatPresetBtn.onClick = [this] { loadPreset(2); };

    setupButton(percussionPresetBtn, "Perc", getNeonPurple());
    percussionPresetBtn.onClick = [this] { loadPreset(3); };

    // Fill buttons
    setupButton(fill8Btn, "Fill 8", getNeonPink().withAlpha(0.8f));
    fill8Btn.onClick = [this] { applyFillPreset(0); };

    setupButton(fill16Btn, "Fill 16", getNeonCyan().withAlpha(0.8f));
    fill16Btn.onClick = [this] { applyFillPreset(1); };

    setupButton(fillRollBtn, "Roll", getNeonPurple().withAlpha(0.8f));
    fillRollBtn.onClick = [this] { applyFillPreset(2); };

    // Preview steps
    for (int i = 0; i < 16; ++i)
    {
        previewSteps[i] = std::make_unique<PreviewStep>();
        previewSteps[i]->setStepIndex(i);
        addAndMakeVisible(previewSteps[i].get());
    }

    // Initial generation
    generateEuclidean();
}

void RhythmExplorer::resized()
{
    auto area = getLocalBounds().reduced(10);

    // Title area
    auto titleArea = area.removeFromTop(25);

    // Euclidean controls row
    auto controlRow = area.removeFromTop(45);
    auto col1 = controlRow.removeFromLeft(controlRow.getWidth() / 3);
    auto col2 = controlRow.removeFromLeft(controlRow.getWidth() / 2);
    auto col3 = controlRow;

    pulseSlider.setBounds(col1.withTrimmedTop(15).reduced(5, 0));
    stepSlider.setBounds(col2.withTrimmedTop(15).reduced(5, 0));
    offsetSlider.setBounds(col3.withTrimmedTop(15).reduced(5, 0));

    // Preview visualization
    auto previewArea = area.removeFromTop(30);
    const int stepWidth = previewArea.getWidth() / 16;
    for (int i = 0; i < 16; ++i)
    {
        previewSteps[i]->setBounds(previewArea.removeFromLeft(stepWidth).reduced(2));
    }

    // Action buttons row
    auto actionRow = area.removeFromTop(30);
    const int btnWidth = actionRow.getWidth() / 4;
    generateButton.setBounds(actionRow.removeFromLeft(btnWidth).reduced(2));
    mutateButton.setBounds(actionRow.removeFromLeft(btnWidth).reduced(2));
    humanizeButton.setBounds(actionRow.removeFromLeft(btnWidth).reduced(2));
    clearButton.setBounds(actionRow.removeFromLeft(btnWidth).reduced(2));

    // Section label
    area.removeFromTop(5);

    // Preset buttons row
    auto presetRow = area.removeFromTop(30);
    const int presetWidth = presetRow.getWidth() / 4;
    kickPresetBtn.setBounds(presetRow.removeFromLeft(presetWidth).reduced(2));
    snarePresetBtn.setBounds(presetRow.removeFromLeft(presetWidth).reduced(2));
    hihatPresetBtn.setBounds(presetRow.removeFromLeft(presetWidth).reduced(2));
    percussionPresetBtn.setBounds(presetRow.removeFromLeft(presetWidth).reduced(2));

    // Fill buttons row
    auto fillRow = area.removeFromTop(30);
    const int fillWidth = fillRow.getWidth() / 3;
    fill8Btn.setBounds(fillRow.removeFromLeft(fillWidth).reduced(2));
    fill16Btn.setBounds(fillRow.removeFromLeft(fillWidth).reduced(2));
    fillRollBtn.setBounds(fillRow.removeFromLeft(fillWidth).reduced(2));
}

void RhythmExplorer::paint(juce::Graphics& g)
{
    // Background with gradient
    auto gradient = juce::ColourGradient::vertical(
        getDarkBackground().brighter(0.05f), 0,
        getDarkBackground(), getHeight());
    g.setGradientFill(gradient);
    g.fillAll();

    // Border
    g.setColour(getNeonCyan().withAlpha(0.3f));
    g.drawRoundedRectangle(getLocalBounds().toFloat().reduced(2), 5, 1);

    // Title
    g.setColour(getNeonCyan());
    g.setFont(juce::Font(13.0f, juce::Font::bold));
    g.drawText("RHYTHM EXPLORER", getLocalBounds().removeFromTop(22),
               juce::Justification::centred, false);
}

void RhythmExplorer::setTargetTrack(int trackIndex)
{
    targetTrack = trackIndex;
}

void RhythmExplorer::generateEuclidean()
{
    currentPattern = EuclideanRhythm::generate(currentPulses, currentSteps, currentOffset);

    // Extend to 16 steps if needed
    currentPattern.resize(16, false);

    updatePreview(currentPattern);

    // Apply to track
    std::vector<int> activeSteps;
    for (int i = 0; i < static_cast<int>(currentPattern.size()); ++i)
    {
        if (currentPattern[i])
            activeSteps.push_back(i);
    }

    if (onApplyPattern)
        onApplyPattern(targetTrack, activeSteps, true);
}

void RhythmExplorer::mutatePattern()
{
    std::random_device rd;
    std::mt19937 rng(rd());

    // Get current active steps
    std::vector<int> activeSteps;
    for (int i = 0; i < 16; ++i)
    {
        if (currentPattern[i])
            activeSteps.push_back(i);
    }

    // Mutate
    auto mutated = PatternTools::mutatePattern(activeSteps, 30, rng);

    // Update pattern
    currentPattern.assign(16, false);
    for (int step : mutated)
    {
        if (step >= 0 && step < 16)
            currentPattern[step] = true;
    }

    updatePreview(currentPattern);

    if (onApplyPattern)
        onApplyPattern(targetTrack, mutated, true);
}

void RhythmExplorer::applyVariation()
{
    std::random_device rd;
    std::mt19937 rng(rd());

    // Get current active steps
    std::vector<int> activeSteps;
    for (int i = 0; i < 16; ++i)
    {
        if (currentPattern[i])
            activeSteps.push_back(i);
    }

    // Humanize
    auto humanized = PatternTools::humanizePattern(activeSteps, 1, rng);

    // Update pattern
    currentPattern.assign(16, false);
    for (int step : humanized)
    {
        if (step >= 0 && step < 16)
            currentPattern[step] = true;
    }

    updatePreview(currentPattern);

    if (onApplyPattern)
        onApplyPattern(targetTrack, humanized, true);
}

void RhythmExplorer::loadPreset(int presetIndex)
{
    if (presetIndex < 0 || presetIndex >= 4)
        return;

    const auto& preset = drumPresets[presetIndex];

    // Update pattern
    currentPattern.assign(16, false);
    for (int step : preset.activeSteps)
    {
        if (step >= 0 && step < 16)
            currentPattern[step] = true;
    }

    updatePreview(currentPattern);

    if (onApplyPattern)
        onApplyPattern(targetTrack, preset.activeSteps, true);
}

void RhythmExplorer::applyFillPreset(int fillIndex)
{
    if (fillIndex < 0 || fillIndex >= 3)
        return;

    const auto& preset = fillPresets[fillIndex];

    if (onApplyFill)
        onApplyFill(targetTrack, preset.activeSteps);
}

void RhythmExplorer::updatePreview(const std::vector<bool>& pattern)
{
    for (int i = 0; i < 16; ++i)
    {
        bool active = (i < static_cast<int>(pattern.size())) && pattern[i];
        previewSteps[i]->setActive(active);
        previewSteps[i]->setStepIndex(i);
    }
}

void RhythmExplorer::initializePresets()
{
    // Kick patterns
    drumPresets[0] = PatternTools::getTechnoKick();

    // Snare patterns
    drumPresets[1] = PatternTools::getHouseClap();

    // HiHat patterns
    drumPresets[2] = PatternTools::getTrapHiHat();

    // Percussion patterns
    drumPresets[3] = PatternTools::getBreakbeat();

    // Fill patterns
    fillPresets[0] = PatternTools::getSnareFill8();
    fillPresets[1] = PatternTools::getSnareFill16();
    fillPresets[2] = PatternTools::getTomRoll();
}

void RhythmExplorer::setupSlider(juce::Slider& slider, int min, int max, int initial)
{
    addAndMakeVisible(slider);
    slider.setRange(min, max, 1);
    slider.setValue(initial);
    slider.setSliderStyle(juce::Slider::LinearHorizontal);
    slider.setTextBoxStyle(juce::Slider::TextBoxRight, false, 30, 18);
    slider.setColour(juce::Slider::thumbColourId, getNeonCyan());
    slider.setColour(juce::Slider::trackColourId, juce::Colours::darkgrey);
    slider.setColour(juce::Slider::textBoxTextColourId, juce::Colours::white);
    slider.setColour(juce::Slider::textBoxBackgroundColourId, getDarkBackground());
}

void RhythmExplorer::setupButton(juce::TextButton& btn, const juce::String& text, juce::Colour color)
{
    addAndMakeVisible(btn);
    btn.setButtonText(text);
    btn.setColour(juce::TextButton::buttonColourId, getDarkBackground());
    btn.setColour(juce::TextButton::textColourOffId, color);
    btn.setColour(juce::TextButton::buttonOnColourId, color.withAlpha(0.3f));
}

//==============================================================================
// PatternTools Implementation
//==============================================================================

PatternPreset PatternTools::getFourOnFloor()
{
    return {"Four on Floor", {0, 4, 8, 12}};
}

PatternPreset PatternTools::getTwoStep()
{
    return {"Two Step", {0, 5, 8, 10}};
}

PatternPreset PatternTools::getBreakbeat()
{
    return {"Breakbeat", {0, 3, 5, 7, 9, 11, 13, 15}};
}

PatternPreset PatternTools::getTrapHiHat()
{
    return {"Trap HiHat", {0, 2, 4, 6, 8, 10, 12, 14}};
}

PatternPreset PatternTools::getBoomBap()
{
    return {"Boom Bap", {0, 4, 6, 10, 12}};
}

PatternPreset PatternTools::getTechnoKick()
{
    return {"Techno Kick", {0, 4, 8, 12}};
}

PatternPreset PatternTools::getHouseClap()
{
    return {"House Clap", {2, 10}};
}

PatternPreset PatternTools::getDnBBreak()
{
    return {"DnB Break", {0, 3, 4, 7, 9, 11, 13}};
}

PatternPreset PatternTools::getSnareFill8()
{
    return {"Snare Fill 8", {8, 10, 11, 12, 13, 14, 15}, ' ', 1, true};
}

PatternPreset PatternTools::getSnareFill16()
{
    return {"Snare Roll", {12, 13, 14, 14, 15, 15}, ' ', 1, true};
}

PatternPreset PatternTools::getKickFill()
{
    return {"Kick Fill", {12, 14, 15}, ' ', 1, true};
}

PatternPreset PatternTools::getTomRoll()
{
    return {"Tom Roll", {10, 11, 12, 13, 14, 15}, ' ', 1, true};
}

std::vector<int> PatternTools::generateAutoFill(int steps, int intensity, std::mt19937& rng)
{
    std::vector<int> fill;
    std::uniform_int_distribution<int> dist(0, 100);

    // Fill starts at last quarter of pattern
    int fillStart = steps - (steps / 4);

    for (int i = fillStart; i < steps; ++i)
    {
        // Increasing probability towards end
        float probability = (static_cast<float>(i - fillStart) / (steps - fillStart)) * (intensity / 100.0f);
        if (dist(rng) < probability * 100)
            fill.push_back(i);
    }

    return fill;
}

std::vector<int> PatternTools::humanizePattern(const std::vector<int>& steps, int variation, std::mt19937& rng)
{
    std::vector<int> result = steps;
    std::uniform_int_distribution<int> offsetDist(-variation, variation);

    for (auto& step : result)
    {
        int offset = offsetDist(rng);
        step = juce::jlimit(0, 15, step + offset);
    }

    // Remove duplicates and sort
    std::sort(result.begin(), result.end());
    result.erase(std::unique(result.begin(), result.end()), result.end());

    return result;
}

std::vector<int> PatternTools::mutatePattern(const std::vector<int>& steps, int mutationRate, std::mt19937& rng)
{
    std::vector<int> result = steps;
    std::uniform_int_distribution<int> chanceDist(0, 100);
    std::uniform_int_distribution<int> stepDist(0, 15);

    // Randomly add, remove, or move notes
    for (size_t i = 0; i < result.size(); ++i)
    {
        if (chanceDist(rng) < mutationRate)
        {
            int action = rng() % 3;

            switch (action)
            {
                case 0: // Move note
                    result[i] = stepDist(rng);
                    break;
                case 1: // Add note nearby
                    result.push_back(stepDist(rng));
                    break;
                case 2: // Remove note (if enough notes)
                    if (result.size() > 2)
                        result.erase(result.begin() + static_cast<int>(i));
                    break;
            }
        }
    }

    // Remove duplicates and sort
    std::sort(result.begin(), result.end());
    result.erase(std::unique(result.begin(), result.end()), result.end());

    return result;
}

std::vector<int> PatternTools::generateByDensity(float density, int steps, std::mt19937& rng)
{
    std::vector<int> result;
    std::uniform_real_distribution<float> dist(0.0f, 1.0f);

    for (int i = 0; i < steps; ++i)
    {
        if (dist(rng) < density)
            result.push_back(i);
    }

    return result;
}
