#include "WavetableDisplay.h"
#include "melatonin_blur.h"
#include <cmath>
#include <algorithm>

WavetableDisplay::WavetableDisplay()
{
    // Initialize drawn waveform buffer
    drawnWaveform.resize(WavetableData::defaultSamplesPerFrame, 0.0f);

    // Edit mode toggle button
    addAndMakeVisible(editButton);
    editButton.setButtonText("EDIT");
    editButton.setColour(juce::TextButton::buttonColourId, getPanelBackground());
    editButton.setColour(juce::TextButton::textColourOnId, getNeonPink());
    editButton.setColour(juce::TextButton::textColourOffId, getNeonCyan());
    editButton.onClick = [this] {
        if (editMode == EditMode::View)
        {
            setEditMode(EditMode::Draw);
            editButton.setButtonText("VIEW");
            editButton.setColour(juce::TextButton::textColourOffId, getNeonPink());
        }
        else
        {
            setEditMode(EditMode::View);
            editButton.setButtonText("EDIT");
            editButton.setColour(juce::TextButton::textColourOffId, getNeonCyan());
        }
    };

    // Clear button
    addAndMakeVisible(clearButton);
    clearButton.setButtonText("CLR");
    clearButton.setColour(juce::TextButton::buttonColourId, getPanelBackground());
    clearButton.setColour(juce::TextButton::textColourOffId, getNeonOrange());
    clearButton.onClick = [this] {
        std::fill(drawnWaveform.begin(), drawnWaveform.end(), 0.0f);
        if (wavetableData)
        {
            wavetableData->createEmptyWavetable();
            if (onWavetableModified)
                onWavetableModified();
        }
        updateWaveform();
        repaint();
    };

    // Generate button
    addAndMakeVisible(generateButton);
    generateButton.setButtonText("GEN");
    generateButton.setColour(juce::TextButton::buttonColourId, getPanelBackground());
    generateButton.setColour(juce::TextButton::textColourOffId, getNeonGreen());
    generateButton.onClick = [this] {
        if (wavetableData && !drawnWaveform.empty())
        {
            wavetableData->generateAllFramesFromDrawing(drawnWaveform);
            if (onWavetableModified)
                onWavetableModified();
            updateWaveform();
            repaint();
        }
    };

    // Basic waveform selector
    addAndMakeVisible(basicWaveCombo);
    basicWaveCombo.addItem("Wave", 0);
    basicWaveCombo.addItem("Sine", 1);
    basicWaveCombo.addItem("Tri", 2);
    basicWaveCombo.addItem("Saw", 3);
    basicWaveCombo.addItem("Sqr", 4);
    basicWaveCombo.setSelectedId(0);
    basicWaveCombo.setColour(juce::ComboBox::outlineColourId, getNeonPurple());
    basicWaveCombo.setColour(juce::ComboBox::textColourId, juce::Colours::white);
    basicWaveCombo.onChange = [this] {
        int selected = basicWaveCombo.getSelectedId();
        if (selected > 0)
        {
            loadBasicWaveform(selected - 1);
            basicWaveCombo.setSelectedId(0, juce::dontSendNotification);
        }
    };

    wavetableData = std::make_shared<WavetableData>();
    updateWaveform();
    startTimerHz(30);
}

WavetableDisplay::~WavetableDisplay()
{
    stopTimer();
}

void WavetableDisplay::connectToParams(WavetableVoice::VoiceParams& p)
{
    params = &p;
    sharedParams = nullptr;
    startTimerHz(60);
}

void WavetableDisplay::connectToSharedParams(std::shared_ptr<WavetableParams> p)
{
    sharedParams = p;
    params = nullptr;
    startTimerHz(60);
}

void WavetableDisplay::setWavetable(std::shared_ptr<WavetableData> data)
{
    wavetableData = data;
    updateWaveform();
    repaint();
}

void WavetableDisplay::setEditMode(EditMode mode)
{
    editMode = mode;

    if (mode == EditMode::Draw)
    {
        setMouseCursor(juce::MouseCursor::CrosshairCursor);
        // Copy current waveform to drawing buffer
        if (wavetableData)
        {
            drawnWaveform = wavetableData->getInterpolatedFrame(currentMorphPosition);
        }
    }
    else
    {
        setMouseCursor(juce::MouseCursor::NormalCursor);
    }

    repaint();
}

void WavetableDisplay::refresh()
{
    if (sharedParams)
        currentMorphPosition = sharedParams->getOscMorph(selectedOscillator);
    else if (params)
        currentMorphPosition = params->oscMorphs[selectedOscillator].load();

    updateWaveform();
    repaint();
}

void WavetableDisplay::updateWaveform()
{
    if (wavetableData)
    {
        wavetableData->getCurrentWaveform(currentMorphPosition, waveformBuffer);
    }
}

void WavetableDisplay::timerCallback()
{
    float newMorph = currentMorphPosition;

    if (sharedParams)
    {
        newMorph = sharedParams->getOscMorph(selectedOscillator);
    }
    else if (params)
    {
        newMorph = params->oscMorphs[selectedOscillator].load();
    }

    if (std::abs(newMorph - currentMorphPosition) > 0.001f)
    {
        currentMorphPosition = newMorph;
        updateWaveform();
        repaint();
    }

    animationPhase += 0.05f;
    if (animationPhase > 1.0f)
        animationPhase -= 1.0f;
}

void WavetableDisplay::mouseDown(const juce::MouseEvent& e)
{
    if (editMode == EditMode::Draw)
    {
        isDrawing = true;
        lastDrawnIndex = -1;

        auto bounds = getLocalBounds();
        bounds.removeFromTop(22);
        auto drawArea = bounds.reduced(10, 5);
        drawAtPosition(e.x, e.y, drawArea);
    }
    else
    {
        mouseDrag(e);
    }
}

void WavetableDisplay::mouseDrag(const juce::MouseEvent& e)
{
    if (editMode == EditMode::Draw && isDrawing)
    {
        auto bounds = getLocalBounds();
        bounds.removeFromTop(22);
        auto drawArea = bounds.reduced(10, 5);
        drawAtPosition(e.x, e.y, drawArea);
    }
    else if (editMode == EditMode::View)
    {
        auto bounds = getLocalBounds();
        bounds.removeFromTop(22);
        auto drawArea = bounds.reduced(10, 5);

        float newMorph = static_cast<float>(e.x - drawArea.getX()) / drawArea.getWidth();
        newMorph = juce::jlimit(0.0f, 1.0f, newMorph);
        currentMorphPosition = newMorph;

        if (sharedParams)
        {
            sharedParams->setOscMorph(selectedOscillator, newMorph);
        }
        else if (params)
        {
            params->oscMorphs[selectedOscillator].store(newMorph);
        }

        updateWaveform();
        repaint();
    }
}

void WavetableDisplay::mouseUp(const juce::MouseEvent& e)
{
    if (editMode == EditMode::Draw && isDrawing)
    {
        isDrawing = false;
        updateWavetableFromDrawing();
    }
}

void WavetableDisplay::drawAtPosition(int x, int y, const juce::Rectangle<int>& drawArea)
{
    if (drawnWaveform.empty())
        return;

    // Check if within draw area (with some tolerance)
    if (x < drawArea.getX() - 5 || x > drawArea.getRight() + 5)
        return;

    float normalizedX = static_cast<float>(x - drawArea.getX()) / drawArea.getWidth();
    float normalizedY = static_cast<float>(y - drawArea.getY()) / drawArea.getHeight();

    normalizedX = juce::jlimit(0.0f, 1.0f, normalizedX);
    normalizedY = juce::jlimit(0.0f, 1.0f, normalizedY);

    int sampleIndex = static_cast<int>(normalizedX * (drawnWaveform.size() - 1));
    sampleIndex = juce::jlimit(0, static_cast<int>(drawnWaveform.size() - 1), sampleIndex);

    float value = normalizedY * 2.0f - 1.0f;  // Map 0-1 to -1 to 1 (inverted Y)
    value = juce::jlimit(-1.0f, 1.0f, value);

    // Draw with interpolation for smooth lines
    if (lastDrawnIndex >= 0 && lastDrawnIndex != sampleIndex)
    {
        int startIdx = std::min(lastDrawnIndex, sampleIndex);
        int endIdx = std::max(lastDrawnIndex, sampleIndex);
        float startVal = drawnWaveform[lastDrawnIndex];
        float endVal = value;

        for (int i = startIdx; i <= endIdx; ++i)
        {
            float t = (endIdx == startIdx) ? 0.0f : static_cast<float>(i - startIdx) / (endIdx - startIdx);
            drawnWaveform[i] = startVal * (1.0f - t) + endVal * t;
        }
    }
    else
    {
        drawnWaveform[sampleIndex] = value;
    }

    lastDrawnIndex = sampleIndex;
    waveformBuffer = drawnWaveform;
    repaint();
}

void WavetableDisplay::updateWavetableFromDrawing()
{
    if (wavetableData && !drawnWaveform.empty())
    {
        wavetableData->setFrame(0, drawnWaveform);
        if (onWavetableModified)
            onWavetableModified();
    }
}

void WavetableDisplay::loadBasicWaveform(int waveformType)
{
    if (drawnWaveform.empty())
        drawnWaveform.resize(WavetableData::defaultSamplesPerFrame);

    int size = static_cast<int>(drawnWaveform.size());

    for (int i = 0; i < size; ++i)
    {
        float phase = static_cast<float>(i) / size;

        switch (waveformType)
        {
            case 0:  // Sine
                drawnWaveform[i] = std::sin(phase * 2.0f * juce::MathConstants<float>::pi);
                break;

            case 1:  // Triangle
                {
                    float t = phase * 4.0f;
                    if (t < 2.0f)
                        drawnWaveform[i] = t - 1.0f;
                    else
                        drawnWaveform[i] = 3.0f - t;
                }
                break;

            case 2:  // Saw
                drawnWaveform[i] = 2.0f * phase - 1.0f;
                break;

            case 3:  // Square
                drawnWaveform[i] = (phase < 0.5f) ? 1.0f : -1.0f;
                break;

            default:
                drawnWaveform[i] = 0.0f;
        }
    }

    updateWavetableFromDrawing();
    waveformBuffer = drawnWaveform;
    repaint();
}

juce::Path WavetableDisplay::createWaveformPath(const juce::Rectangle<float>& bounds)
{
    juce::Path path;

    if (waveformBuffer.empty())
        return path;

    float width = bounds.getWidth();
    float height = bounds.getHeight();
    float centerY = bounds.getY() + height / 2;
    float halfHeight = height / 2 - 5;

    int numSamples = static_cast<int>(waveformBuffer.size());
    float xScale = width / (numSamples - 1);

    for (int i = 0; i < numSamples; ++i)
    {
        float x = bounds.getX() + i * xScale;
        float y = centerY - waveformBuffer[i] * halfHeight;

        if (i == 0)
            path.startNewSubPath(x, y);
        else
            path.lineTo(x, y);
    }

    return path;
}

void WavetableDisplay::drawGrid(juce::Graphics& g, const juce::Rectangle<int>& bounds)
{
    g.setColour(juce::Colours::white.withAlpha(0.08f));

    // Horizontal lines
    for (int i = 0; i <= 4; ++i)
    {
        float y = bounds.getY() + i * bounds.getHeight() / 4;
        g.drawHorizontalLine(static_cast<int>(y), bounds.getX(), bounds.getRight());
    }

    // Vertical lines
    for (int i = 0; i <= 8; ++i)
    {
        float x = bounds.getX() + i * bounds.getWidth() / 8;
        g.drawVerticalLine(static_cast<int>(x), bounds.getY(), bounds.getBottom());
    }

    // Center line (stronger)
    g.setColour(juce::Colours::white.withAlpha(0.15f));
    float centerY = bounds.getY() + bounds.getHeight() / 2.0f;
    g.drawHorizontalLine(static_cast<int>(centerY), bounds.getX(), bounds.getRight());
}

void WavetableDisplay::paint(juce::Graphics& g)
{
    auto bounds = getLocalBounds();

    // Background
    g.setColour(getPanelBackground());
    g.fillRoundedRectangle(bounds.toFloat(), 5);

    // Button row at top
    auto buttonRow = bounds.removeFromTop(20);

    // Draw buttons
    editButton.setBounds(buttonRow.removeFromLeft(45));
    buttonRow.removeFromLeft(5);
    clearButton.setBounds(buttonRow.removeFromLeft(40));
    buttonRow.removeFromLeft(5);
    generateButton.setBounds(buttonRow.removeFromLeft(40));
    buttonRow.removeFromLeft(10);
    basicWaveCombo.setBounds(buttonRow.removeFromLeft(65));

    // Wavetable name on the right
    if (wavetableData)
    {
        g.setColour(getNeonPurple());
        g.setFont(juce::Font(10.0f, juce::Font::bold));
        g.drawText(wavetableData->getName(), buttonRow.removeFromRight(150),
                   juce::Justification::centredRight, false);
    }

    // Draw area
    auto drawArea = bounds.reduced(10, 5);

    // Grid
    drawGrid(g, drawArea);

    // Waveform path
    auto path = createWaveformPath(drawArea.toFloat());

    // Glow effect
    auto glowColor = (editMode == EditMode::Draw) ? getNeonPink() : getNeonCyan();
    melatonin::DropShadow glow(glowColor, 6, {0, 0});
    glow.render(g, path);

    // Main waveform line
    g.setColour(glowColor);
    g.strokePath(path, juce::PathStrokeType(2.0f));

    // Morph position indicator (view mode only)
    if (editMode == EditMode::View)
    {
        float indicatorX = drawArea.getX() + currentMorphPosition * drawArea.getWidth();
        g.setColour(getNeonPink().withAlpha(0.8f));
        g.drawVerticalLine(static_cast<int>(indicatorX), drawArea.getY(), drawArea.getBottom());

        // Morph value text
        g.setColour(juce::Colours::white);
        g.setFont(juce::Font(9.0f));
        g.drawText(juce::String(currentMorphPosition, 2), drawArea.getX(), drawArea.getBottom() - 12,
                   40, 12, juce::Justification::left, false);
    }

    // Draw mode indicator
    if (editMode == EditMode::Draw)
    {
        g.setColour(getNeonPink().withAlpha(0.7f));
        g.setFont(juce::Font(9.0f, juce::Font::bold));
        g.drawText("DRAW MODE - Draw waveform, then click GEN", drawArea.getX(), drawArea.getBottom() - 12,
                   300, 12, juce::Justification::left, false);
    }

    // Border
    auto borderColor = (editMode == EditMode::Draw) ? getNeonPink() : getNeonCyan();
    g.setColour(borderColor.withAlpha(0.5f));
    g.drawRoundedRectangle(getLocalBounds().toFloat().reduced(1), 5, 1);
}

void WavetableDisplay::resized()
{
    // Buttons are positioned in paint() based on current bounds
}
