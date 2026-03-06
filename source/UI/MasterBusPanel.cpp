#include "MasterBusPanel.h"
#include "../AudioRouting/AudioRoutingGraph.h"
#include "../Theme/ThemeManager.h"

//==============================================================================
MasterBusPanel::MasterBusPanel(AudioRoutingGraph& graph)
    : DockablePanel(PanelType::MasterBus, "Master Bus")
    , audioGraph(graph)
{
    setOpaque(true);

    // Timer wird in initializeContent() gestartet
}

MasterBusPanel::~MasterBusPanel()
{
    stopTimer();
}

//==============================================================================
void MasterBusPanel::initializeContent()
{
    // Initialisiere Level-History
    for (int i = 0; i < historySize; ++i)
    {
        leftLevelHistory.push_back(-60.0f);
        rightLevelHistory.push_back(-60.0f);
    }

    // Starte Meter-Updates-Timer (60fps)
    startTimerHz(60);
}

//==============================================================================
void MasterBusPanel::setLoudnessMode(LoudnessMode mode)
{
    if (currentLoudnessMode != mode)
    {
        currentLoudnessMode = mode;
        resetLoudness();
        repaint();
    }
}

void MasterBusPanel::setMeterIntegrationTime(float seconds)
{
    integrationTime = juce::jlimit(0.1f, 2.0f, seconds);
}

//==============================================================================
void MasterBusPanel::setMasterVolume(float volume)
{
    masterVolume = juce::jlimit(0.0f, 1.0f, volume);
    repaint();
}

void MasterBusPanel::setMasterPan(float pan)
{
    masterPan = juce::jlimit(-1.0f, 1.0f, pan);
    repaint();
}

void MasterBusPanel::setMasterMute(bool muted)
{
    masterMuted = muted;
    repaint();
}

void MasterBusPanel::toggleMasterMute()
{
    setMasterMute(!masterMuted);
}

//==============================================================================
void MasterBusPanel::setExportFormat(const juce::String& format)
{
    exportFormat = format;
}

void MasterBusPanel::setSampleRate(int rate)
{
    sampleRate = rate;
}

void MasterBusPanel::setBitDepth(int bits)
{
    bitDepth = bits;
}

void MasterBusPanel::setMasterEffectsEnabled(bool enabled)
{
    masterEffectsEnabled = enabled;
    repaint();
}

//==============================================================================
void MasterBusPanel::setLevelUpdateCallback(std::function<void(float, float)> callback)
{
    levelUpdateCallback = callback;
}

void MasterBusPanel::setLoudnessUpdateCallback(std::function<void(float)> callback)
{
    loudnessUpdateCallback = callback;
}

//==============================================================================
void MasterBusPanel::paint(juce::Graphics& g)
{
    auto& theme = ThemeManager::getInstance();
    auto bounds = getLocalBounds().toFloat();

    // Hintergrund
    g.fillAll(theme.getPanelBackgroundColor());

    // Content-Bereich (ohne Panel-Header)
    auto contentArea = getContentBounds().toFloat();

    // Export Section Breite definieren
    const float exportSectionWidth = 140.0f;

    // Metering-Section (Links)
    auto meterWidth = 60.0f;
    auto meterBounds = juce::Rectangle<float>(
        contentArea.getX() + 10,
        contentArea.getY() + 10,
        meterWidth,
        contentArea.getHeight() - 20
    );

    // Stereo Meter
    drawStereoMeter(g, meterBounds, leftLevel, rightLevel);

    // Controls-Section (Mitte)
    auto controlsX = meterBounds.getRight() + 10;
    auto controlsWidth = contentArea.getWidth() - meterWidth - exportSectionWidth - 40;
    auto controlsBounds = juce::Rectangle<float>(
        controlsX,
        contentArea.getY() + 10,
        controlsWidth,
        contentArea.getHeight() - 20
    );

    drawMasterControls(g, controlsBounds);

    // Export-Section (Rechts)
    auto exportBounds = juce::Rectangle<float>(
        contentArea.getRight() - exportSectionWidth - 10,
        contentArea.getY() + 10,
        exportSectionWidth,
        contentArea.getHeight() - 20
    );

    drawExportControls(g, exportBounds);

    // Master Effects (unter Controls)
    if (masterEffectsEnabled)
    {
        auto effectsBounds = juce::Rectangle<float>(
            controlsX,
            controlsBounds.getBottom() + 10,
            controlsWidth,
            80
        );

        drawMasterEffects(g, effectsBounds);
    }
}

void MasterBusPanel::resized()
{
    // Layout wird komplett in paint() über graphics berechnet
}

void MasterBusPanel::timerCallback()
{
    // Decay Peaks
    float decay = 0.05f;  // 5% pro Frame bei 60fps
    peakHoldTime += 1.0f / 60.0f;

    if (peakHoldTime > 0.5f)  // 0.5 Sekunden ohne neuer Peak
    {
        leftPeak *= (1.0f - decay);
        rightPeak *= (1.0f - decay);
    }

    // Recalculate History Average für Loudness
    if (leftLevelHistory.size() > 0)
    {
        float avgLeft = 0.0f;
        for (auto level : leftLevelHistory)
            avgLeft += level;
        avgLeft /= leftLevelHistory.size();

        float avgRight = 0.0f;
        for (auto level : rightLevelHistory)
            avgRight += level;
        avgRight /= rightLevelHistory.size();

        // Aktualisiere Loudness basierend auf Durchschnitt
        float avgLevel = (avgLeft + avgRight) / 2.0f;
        loudnessLUFS = loudnessLUFS * 0.9f + avgLevel * 0.1f;
    }

    repaint();
}

//==============================================================================
void MasterBusPanel::updateLevels(float newLeft, float newRight)
{
    // Exponential Smoothing
    float alpha = 0.1f;  // Smoothing-Faktor

    leftLevel = leftLevel * (1.0f - alpha) + newLeft * alpha;
    rightLevel = rightLevel * (1.0f - alpha) + newRight * alpha;

    // Clip und Peak Detection
    leftLevel = juce::jlimit(minDB, maxDB, leftLevel);
    rightLevel = juce::jlimit(minDB, maxDB, rightLevel);

    if (leftLevel > leftPeak)
    {
        leftPeak = leftLevel;
        peakHoldTime = 0.0f;
    }

    if (rightLevel > rightPeak)
    {
        rightPeak = rightLevel;
        peakHoldTime = 0.0f;
    }

    // Update History
    leftLevelHistory.push_back(leftLevel);
    rightLevelHistory.push_back(rightLevel);

    if (leftLevelHistory.size() > historySize)
        leftLevelHistory.pop_front();

    if (rightLevelHistory.size() > historySize)
        rightLevelHistory.pop_front();
}

void MasterBusPanel::drawStereoMeter(juce::Graphics& g, juce::Rectangle<float> bounds, float left, float right)
{
    auto& theme = ThemeManager::getInstance();

    // Hintergrund
    g.setColour(theme.getPanelBackgroundColor());
    g.fillRect(bounds);

    // Meter-Hintergrund-Track
    auto meterTrackWidth = bounds.getWidth() / 2.0f - 8.0f;
    auto leftMeterX = bounds.getX() + 4.0f;
    auto rightMeterX = leftMeterX + meterTrackWidth + 16.0f;
    auto meterY = bounds.getY() + 10.0f;
    auto meterHeight = bounds.getHeight() - 25.0f;

    // Left Meter Track
    g.setColour(juce::Colours::black.withAlpha(0.3f));
    g.fillRect(juce::Rectangle<float>(leftMeterX, meterY, meterTrackWidth, meterHeight));

    // Right Meter Track
    g.fillRect(juce::Rectangle<float>(rightMeterX, meterY, meterTrackWidth, meterHeight));

    // Meter-Fill Gradient (grün bei unten, rot bei oben)
    auto gradient = juce::ColourGradient::vertical(theme.getWarningColor(), theme.getSuccessColor(), bounds);
    g.setGradientFill(gradient);

    // Left Level
    auto leftY = dbToY(left, bounds);
    if (leftY < meterY)
        leftY = meterY;
    if (leftY > meterY + meterHeight)
        leftY = meterY + meterHeight;

    auto leftFillHeight = meterHeight - (leftY - meterY);
    if (leftFillHeight > 0)
    {
        g.fillRect(juce::Rectangle<float>(leftMeterX, leftY, meterTrackWidth, leftFillHeight));
    }

    // Left Peak
    auto leftPeakY = dbToY(leftPeak, bounds);
    if (leftPeakY < meterY)
        leftPeakY = meterY;
    if (leftPeakY > meterY + meterHeight)
        leftPeakY = meterY + meterHeight;

    // Peak Hold Indicator
    if (leftPeak - leftLevel < 3.0f)
    {
        auto peakY = leftPeakY - 2.0f;
        if (peakY >= meterY)
        {
            g.setColour(theme.getAccentColor().withAlpha(0.7f));
            g.fillRect(juce::Rectangle<float>(leftMeterX, peakY, meterTrackWidth, 4.0f));
        }
    }

    // Clip Indicator (0dB)
    if (leftLevel >= -0.1f)
    {
        auto clipY = dbToY(-0.1f, bounds);
        if (clipY >= meterY && clipY <= meterY + meterHeight)
        {
            g.setColour(theme.getErrorColor());
            g.fillRect(juce::Rectangle<float>(leftMeterX - 2, clipY - 2, meterTrackWidth + 4, 4));
        }
    }

    // L Label
    g.setColour(theme.getTextSecondaryColor());
    g.setFont(10.0f);
    g.drawText("L", juce::Rectangle<float>(leftMeterX, bounds.getBottom() - 12, meterTrackWidth, 12),
              juce::Justification::centred, false);

    // Right Level
    auto rightY = dbToY(right, bounds);
    if (rightY < meterY)
        rightY = meterY;
    if (rightY > meterY + meterHeight)
        rightY = meterY + meterHeight;

    auto rightFillHeight = meterHeight - (rightY - meterY);
    if (rightFillHeight > 0)
    {
        g.fillRect(juce::Rectangle<float>(rightMeterX, rightY, meterTrackWidth, rightFillHeight));
    }

    // Right Peak
    auto rightPeakY = dbToY(rightPeak, bounds);
    if (rightPeakY < meterY)
        rightPeakY = meterY;
    if (rightPeakY > meterY + meterHeight)
        rightPeakY = meterY + meterHeight;

    // Peak Hold Indicator
    if (rightPeak - rightLevel < 3.0f)
    {
        auto peakY = rightPeakY - 2.0f;
        if (peakY >= meterY)
        {
            g.setColour(theme.getAccentColor().withAlpha(0.7f));
            g.fillRect(juce::Rectangle<float>(rightMeterX, peakY, meterTrackWidth, 4.0f));
        }
    }

    // Clip Indicator
    if (rightLevel >= -0.1f)
    {
        auto clipY = dbToY(-0.1f, bounds);
        if (clipY >= meterY && clipY <= meterY + meterHeight)
        {
            g.setColour(theme.getErrorColor());
            g.fillRect(juce::Rectangle<float>(rightMeterX - 2, clipY - 2, meterTrackWidth + 4, 4));
        }
    }

    // R Label
    g.setColour(theme.getTextSecondaryColor());
    g.setFont(10.0f);
    g.drawText("R", juce::Rectangle<float>(rightMeterX, bounds.getBottom() - 12, meterTrackWidth, 12),
              juce::Justification::centred, false);
}

void MasterBusPanel::drawLoudnessMeter(juce::Graphics& g, juce::Rectangle<float> bounds, float loudness)
{
    auto& theme = ThemeManager::getInstance();

    // Hintergrund
    g.setColour(theme.getPanelBackgroundColor());
    g.fillRect(bounds);

    // Meter-Gradient
    auto gradient = juce::ColourGradient::vertical(theme.getWarningColor(), theme.getSuccessColor(), bounds);
    g.setGradientFill(gradient);

    auto loudnessY = dbToY(loudness, bounds);
    auto meterArea = bounds.withTrimmedTop(10).withTrimmedBottom(25);
    auto fillHeight = meterArea.getHeight() - (loudnessY - meterArea.getY());

    if (fillHeight > 0 && fillHeight <= meterArea.getHeight())
    {
        g.fillRect(juce::Rectangle<float>(meterArea.getX(), loudnessY, meterArea.getWidth(), fillHeight));
    }

    // LUFS Target Line
    auto targetY = dbToY(targetLUFS, bounds);
    if (targetY >= meterArea.getY() && targetY <= meterArea.getBottom())
    {
        g.setColour(theme.getAccentColor().withAlpha(0.4f));
        g.drawLine(juce::Line<float>(meterArea.getX() + 10, targetY, meterArea.getRight() - 10, targetY), 2.0f);
    }

    // Level-Text
    g.setColour(theme.getTextPrimaryColor());
    g.setFont(14.0f);
    g.setFont(juce::Font(14.0f, juce::Font::bold));
    g.drawText(juce::String(loudness, 1) + " LUFS",
              meterArea, juce::Justification::centred, false);

    // Ziel-Wert
    g.setColour(theme.getTextSecondaryColor());
    g.setFont(10.0f);
    g.drawText("Target: -14 LUFS (EBU R128)",
              juce::Rectangle<float>(bounds.getX() + 10, bounds.getBottom() - 18, bounds.getWidth() - 20, 12),
              juce::Justification::centred, false);
}

void MasterBusPanel::drawMasterControls(juce::Graphics& g, juce::Rectangle<float> bounds)
{
    auto& theme = ThemeManager::getInstance();

    // Hintergrund
    g.setColour(theme.getPanelBackgroundColor());
    g.fillRect(bounds);

    // Titel
    g.setColour(theme.getTextPrimaryColor());
    g.setFont(juce::Font(14.0f, juce::Font::bold));
    g.drawText("Master Controls",
              juce::Rectangle<float>(bounds.getX() + 10, bounds.getY() + 10, bounds.getWidth() - 20, 20),
              juce::Justification::left, false);

    // Volume Slider
    auto sliderY = bounds.getY() + 40;
    auto sliderWidth = bounds.getWidth() - 20;
    auto sliderHeight = 20;

    auto sliderBounds = juce::Rectangle<float>(bounds.getX() + 10, sliderY, sliderWidth, sliderHeight);

    // Slider-Track
    g.setColour(theme.getSliderTrackColor());
    g.fillRoundedRectangle(sliderBounds, 4.0f);

    // Slider-Fill
    auto fillWidth = sliderWidth * juce::jlimit(0.0f, 1.0f, masterVolume);
    auto fillBounds = sliderBounds.withTrimmedLeft(2.0f).withWidth(fillWidth - 4.0f);
    fillBounds = fillBounds.reduced(2.0f, 2.0f);

    g.setColour(theme.getSliderColor());
    g.fillRoundedRectangle(fillBounds, 3.0f);

    // Slider-Thumb
    auto thumbX = fillBounds.getRight() - 5.0f;
    auto thumbWidth = 15.0f;
    g.setColour(theme.getSliderThumbColor());
    g.fillRoundedRectangle(juce::Rectangle<float>(thumbX, sliderY + 2, thumbWidth, sliderHeight - 4), 3.0f);

    // Volume-Label
    g.setColour(theme.getTextSecondaryColor());
    g.setFont(10.0f);
    g.drawText("Volume: " + juce::String(masterVolume * 100, 0) + "%",
              juce::Rectangle<float>(bounds.getX() + 10, sliderY + 25, sliderWidth, 15),
              juce::Justification::left, false);

    // Pan Slider
    auto panY = sliderY + 50;
    auto panSliderBounds = juce::Rectangle<float>(bounds.getX() + 10, panY, sliderWidth, sliderHeight);

    g.setColour(theme.getSliderTrackColor());
    g.fillRoundedRectangle(panSliderBounds, 4.0f);

    // Pan-Center-Indicator
    auto centerPoint = panSliderBounds.getCentre().toFloat();
    g.setColour(theme.getAccentColor());
    auto centerRect = juce::Rectangle<float>(centerPoint.x - 4.0f, panY, 8.0f, sliderHeight);
    g.fillRoundedRectangle(centerRect, 2.0f);

    // Pan-Thumb
    auto panNorm = (masterPan + 1.0f) / 2.0f;  // -1..1 -> 0..1
    auto panThumbX = panSliderBounds.getX() + panSliderBounds.getWidth() * panNorm - 5.0f;
    g.setColour(theme.getSliderThumbColor());
    g.fillRoundedRectangle(juce::Rectangle<float>(panThumbX, panY + 2, 10.0f, sliderHeight - 4), 2.0f);

    // Pan-Text
    g.setColour(theme.getTextSecondaryColor());
    g.setFont(10.0f);
    juce::String panStr;
    if (masterPan > 0.1f)
        panStr = "R " + juce::String(masterPan * 50, 0);
    else if (masterPan < -0.1f)
        panStr = "L " + juce::String(-masterPan * 50, 0);
    else
        panStr = "Center";

    g.drawText("Pan: " + panStr,
              juce::Rectangle<float>(bounds.getX() + 10, panY + 25, sliderWidth, 15),
              juce::Justification::left, false);

    // Mute Button
    auto muteY = panY + 50;
    auto muteWidth = 80.0f;
    auto muteHeight = 30.0f;
    auto muteBounds = juce::Rectangle<float>(bounds.getX() + 10, muteY, muteWidth, muteHeight);

    auto muteColor = masterMuted ? theme.getWarningColor() : theme.getButtonColor();
    g.setColour(muteColor);
    g.fillRoundedRectangle(muteBounds, 4.0f);

    // Mute Text
    g.setColour(masterMuted ? juce::Colours::black : theme.getTextPrimaryColor());
    g.setFont(12.0f);
    g.setFont(juce::Font(12.0f, juce::Font::bold));
    g.drawText(masterMuted ? "MUTED" : "MUTE",
              muteBounds, juce::Justification::centred, false);
}

void MasterBusPanel::drawExportControls(juce::Graphics& g, juce::Rectangle<float> bounds)
{
    auto& theme = ThemeManager::getInstance();

    // Hintergrund
    g.setColour(theme.getPanelBackgroundColor());
    g.fillRect(bounds);

    // Titel
    g.setColour(theme.getTextPrimaryColor());
    g.setFont(14.0f);
    g.setFont(juce::Font(14.0f, juce::Font::bold));
    g.drawText("Export / Render",
              juce::Rectangle<float>(bounds.getX() + 10, bounds.getY() + 10, bounds.getWidth() - 20, 20),
              juce::Justification::left, false);

    // Format Dropdown (visuelle Darstellung)
    auto formatY = bounds.getY() + 40;
    auto formatBounds = juce::Rectangle<float>(bounds.getX() + 10, formatY, bounds.getWidth() - 20, 30);

    g.setColour(theme.getButtonColor());
    g.fillRoundedRectangle(formatBounds, 4.0f);
    g.setColour(theme.getTextPrimaryColor());
    g.setFont(11.0f);
    g.drawText(exportFormat,
              formatBounds, juce::Justification::centred, false);

    // Sample Rate Display
    auto srY = formatY + 40;
    g.setColour(theme.getTextSecondaryColor());
    g.setFont(10.0f);
    g.drawText("SR: " + juce::String(sampleRate) + " Hz",
              juce::Rectangle<float>(bounds.getX() + 10, srY, bounds.getWidth() - 20, 15),
              juce::Justification::left, false);

    // Bit Depth Display
    auto bitY = srY + 20;
    g.drawText("Bits: " + juce::String(bitDepth),
              juce::Rectangle<float>(bounds.getX() + 10, bitY, bounds.getWidth() - 20, 15),
              juce::Justification::left, false);

    // Export Button
    auto exportY = bitY + 30;
    auto exportBounds = juce::Rectangle<float>(bounds.getX() + 10, exportY, bounds.getWidth() - 20, 40);

    g.setColour(theme.getAccentColor());
    g.fillRoundedRectangle(exportBounds, 4.0f);
    g.setColour(juce::Colours::white);
    g.setFont(12.0f);
    g.setFont(juce::Font(12.0f, juce::Font::bold));

    if (isExporting)
    {
        g.drawText("Exporting...", exportBounds, juce::Justification::centred, false);

        // Progress Bar
        auto progressBounds = juce::Rectangle<float>(
            exportBounds.getX() + 10,
            exportBounds.getCentreY() + 8,
            exportBounds.getWidth() - 20,
            4
        );
        g.setColour(juce::Colours::white.withAlpha(0.3f));
        g.fillRoundedRectangle(progressBounds, 2.0f);

        auto progressFillWidth = progressBounds.getWidth() * renderProgress;
        auto progressFillBounds = progressBounds.withWidth(progressFillWidth);
        g.setColour(theme.getSuccessColor());
        g.fillRoundedRectangle(progressFillBounds, 2.0f);
    }
    else
    {
        g.drawText("Export", exportBounds, juce::Justification::centred, false);
    }

    // Render Button
    auto renderY = exportY + 50;
    auto renderBounds = juce::Rectangle<float>(bounds.getX() + 10, renderY, bounds.getWidth() - 20, 40);

    g.setColour(theme.getInfoColor());
    g.fillRoundedRectangle(renderBounds, 4.0f);
    g.setColour(theme.getTextPrimaryColor());
    g.setFont(12.0f);
    g.setFont(juce::Font(12.0f, juce::Font::bold));
    g.drawText("Render", renderBounds, juce::Justification::centred, false);
}

void MasterBusPanel::drawMasterEffects(juce::Graphics& g, juce::Rectangle<float> bounds)
{
    auto& theme = ThemeManager::getInstance();

    // Hintergrund
    g.setColour(theme.getPanelBackgroundColor());
    g.fillRect(bounds);

    // Titel
    g.setColour(theme.getTextPrimaryColor());
    g.setFont(14.0f);
    g.setFont(juce::Font(14.0f, juce::Font::bold));
    g.drawText("Master Effects",
              juce::Rectangle<float>(bounds.getX() + 10, bounds.getY() + 10, bounds.getWidth() - 20, 20),
              juce::Justification::left, false);

    // Info-Text
    g.setColour(theme.getTextSecondaryColor());
    g.setFont(10.0f);
    g.drawText("Master Effects Enabled",
              bounds, juce::Justification::centred, false);
}

//==============================================================================
float MasterBusPanel::dbToY(float db, juce::Rectangle<float> meterArea) const
{
    auto range = maxDB - minDB;
    auto normalized = juce::jlimit(0.0f, 1.0f, (db - minDB) / range);

    auto meterY = meterArea.getY() + 10;
    auto meterHeight = meterArea.getHeight() - 20;

    return meterY + meterHeight * (1.0f - normalized);
}

juce::String MasterBusPanel::dbToString(float db) const
{
    if (db < -60.0f) return "-60 dB";
    if (db < -40.0f) return "-40 dB";
    if (db < -20.0f) return "-20 dB";
    if (db < -10.0f) return "-10 dB";
    if (db < -6.0f) return "-6 dB";
    if (db < -3.0f) return "-3 dB";
    if (db < 0.0f) return "-0 dB";
    if (db < 3.0f) return "3 dB";
    if (db < 6.0f) return "6 dB";
    if (db < 10.0f) return "10 dB";
    if (db < 20.0f) return "20 dB";
    return "-0 dB";
}

void MasterBusPanel::resetLevels()
{
    leftLevel = -60.0f;
    rightLevel = -60.0f;
    leftPeak = -60.0f;
    rightPeak = -60.0f;
    peakHoldTime = 0.0f;

    for (auto& level : leftLevelHistory)
        level = -60.0f;

    for (auto& level : rightLevelHistory)
        level = -60.0f;

    repaint();
}

void MasterBusPanel::resetLoudness()
{
    loudnessLUFS = -60.0f;
    truePeak = -60.0f;
    falsePeak = -60.0f;
}
