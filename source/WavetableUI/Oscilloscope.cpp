#include "Oscilloscope.h"
#include "melatonin_blur.h"

Oscilloscope::Oscilloscope()
    : leftBuffer(bufferSize, 0.0f),
      rightBuffer(bufferSize, 0.0f)
{
    startTimerHz(30);
}

Oscilloscope::~Oscilloscope()
{
    stopTimer();
}

void Oscilloscope::pushSamples(const float* leftSamples, const float* rightSamples, int numSamples)
{
    for (int i = 0; i < numSamples; ++i)
    {
        leftBuffer[writePosition] = leftSamples[i];
        rightBuffer[writePosition] = rightSamples ? rightSamples[i] : leftSamples[i];
        writePosition = (writePosition + 1) % bufferSize;
    }
}

juce::Path Oscilloscope::createWaveformPath(const std::vector<float>& buffer,
                                             const juce::Rectangle<float>& bounds)
{
    juce::Path path;

    float width = bounds.getWidth();
    float height = bounds.getHeight();
    float centerY = bounds.getY() + height / 2;
    float halfHeight = height / 2 - 2;

    int numPoints = static_cast<int>(width);
    float samplesPerPoint = static_cast<float>(bufferSize) / numPoints;

    for (int i = 0; i < numPoints; ++i)
    {
        float bufferIndex = i * samplesPerPoint;
        int index0 = static_cast<int>(bufferIndex) % bufferSize;
        int index1 = (index0 + 1) % bufferSize;
        float alpha = bufferIndex - static_cast<int>(bufferIndex);

        float sample = buffer[index0] * (1.0f - alpha) + buffer[index1] * alpha;

        float x = bounds.getX() + i;
        float y = centerY - sample * halfHeight;

        if (i == 0)
            path.startNewSubPath(x, y);
        else
            path.lineTo(x, y);
    }

    return path;
}

void Oscilloscope::timerCallback()
{
    repaint();
}

void Oscilloscope::paint(juce::Graphics& g)
{
    auto bounds = getLocalBounds();

    // Background
    g.setColour(getPanelBackground());
    g.fillRoundedRectangle(bounds.toFloat(), 5);

    // Grid
    auto innerBounds = bounds.reduced(5);
    g.setColour(juce::Colours::white.withAlpha(0.1f));

    // Center line
    float centerY = innerBounds.getCentreY();
    g.drawHorizontalLine(static_cast<int>(centerY), innerBounds.getX(), innerBounds.getRight());

    // Title
    g.setColour(getNeonCyan());
    g.setFont(juce::Font(10.0f, juce::Font::bold));
    g.drawText("SCOPE", innerBounds.removeFromTop(15), juce::Justification::left, false);

    auto displayBounds = innerBounds.toFloat();

    // Draw left channel
    auto leftPath = createWaveformPath(leftBuffer, displayBounds);
    melatonin::DropShadow leftGlow(getNeonCyan(), 3, {0, 0});
    leftGlow.render(g, leftPath);
    g.setColour(getNeonCyan());
    g.strokePath(leftPath, juce::PathStrokeType(1.5f));

    // Draw right channel
    auto rightPath = createWaveformPath(rightBuffer, displayBounds);
    melatonin::DropShadow rightGlow(getNeonPink(), 3, {0, 0});
    rightGlow.render(g, rightPath);
    g.setColour(getNeonPink().withAlpha(0.7f));
    g.strokePath(rightPath, juce::PathStrokeType(1.0f));

    // Border
    g.setColour(getNeonCyan().withAlpha(0.3f));
    g.drawRoundedRectangle(bounds.toFloat().reduced(1), 5, 1);
}

void Oscilloscope::resized()
{
    // Nothing specific
}
