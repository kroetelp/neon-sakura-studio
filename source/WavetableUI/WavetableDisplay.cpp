#include "WavetableDisplay.h"
#include "melatonin_blur.h"

WavetableDisplay::WavetableDisplay()
{
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

void WavetableDisplay::refresh()
{
    // --- FIX: Garantiert den absolut neuesten Morph-Wert beim Preset-Laden abgreifen ---
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
    mouseDrag(e);
}

void WavetableDisplay::mouseDrag(const juce::MouseEvent& e)
{
    float newMorph = static_cast<float>(e.x) / getWidth();
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

    path.startNewSubPath(bounds.getX(), centerY);

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

void WavetableDisplay::paint(juce::Graphics& g)
{
    auto bounds = getLocalBounds();

    g.setColour(getPanelBackground());
    g.fillRoundedRectangle(bounds.toFloat(), 5);

    g.setColour(juce::Colours::white.withAlpha(0.1f));
    auto innerBounds = bounds.reduced(10);
    for (int i = 0; i <= 4; ++i)
    {
        float y = innerBounds.getY() + i * innerBounds.getHeight() / 4;
        g.drawHorizontalLine(static_cast<int>(y), innerBounds.getX(), innerBounds.getRight());
    }
    for (int i = 0; i <= 8; ++i)
    {
        float x = innerBounds.getX() + i * innerBounds.getWidth() / 8;
        g.drawVerticalLine(static_cast<int>(x), innerBounds.getY(), innerBounds.getBottom());
    }

    auto path = createWaveformPath(innerBounds.toFloat());

    melatonin::DropShadow glow(getNeonCyan(), 6, {0, 0});
    glow.render(g, path);

    g.setColour(getNeonCyan());
    g.strokePath(path, juce::PathStrokeType(2.0f));

    float indicatorX = innerBounds.getX() + currentMorphPosition * innerBounds.getWidth();
    g.setColour(getNeonPink());
    g.drawVerticalLine(static_cast<int>(indicatorX), innerBounds.getY(), innerBounds.getBottom());

    g.setColour(juce::Colours::white);
    g.setFont(juce::Font(10.0f));
    g.drawText(juce::String(currentMorphPosition, 2), innerBounds.getX(), innerBounds.getBottom() - 15,
               50, 15, juce::Justification::left, false);

    if (wavetableData)
    {
        g.setColour(getNeonPurple());
        g.setFont(juce::Font(11.0f, juce::Font::bold));
        g.drawText(wavetableData->getName(), bounds.removeFromTop(20),
                   juce::Justification::centred, false);
    }

    g.setColour(getNeonCyan().withAlpha(0.5f));
    g.drawRoundedRectangle(getLocalBounds().toFloat().reduced(1), 5, 1);
}

void WavetableDisplay::resized()
{
    updateWaveform();
}