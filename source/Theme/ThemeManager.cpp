#include "ThemeManager.h"
#include "ProfessionalLookAndFeel.h"

ThemeManager::ThemeManager()
{
    // Default to Professional theme with DarkOrange scheme
    setCurrentTheme(ThemeType::Professional);
}

ThemeManager::~ThemeManager()
{
}

// Singleton instance
ThemeManager& ThemeManager::getInstance()
{
    static ThemeManager instance;
    return instance;
}

void ThemeManager::setCurrentTheme(ThemeType type)
{
    currentTheme = type;

    // Update all colors based on theme and scheme
    switch (type)
    {
        case ThemeType::Professional:
            switch (currentColorScheme)
            {
                case ProfessionalTheme::ColorScheme::DarkOrange:
                    backgroundColor = ProfessionalTheme::colour(ProfessionalTheme::DarkOrange::Background);
                    backgroundLighterColor = ProfessionalTheme::colour(ProfessionalTheme::DarkOrange::BackgroundLighter);
                    accentColor = ProfessionalTheme::colour(ProfessionalTheme::DarkOrange::AccentPrimary);
                    textPrimaryColor = ProfessionalTheme::colour(ProfessionalTheme::DarkOrange::TextPrimary);
                    textSecondaryColor = ProfessionalTheme::colour(ProfessionalTheme::DarkOrange::TextSecondary);
                    panelBackgroundColor = ProfessionalTheme::colour(ProfessionalTheme::DarkOrange::PanelBackground);
                    panelHeaderColor = ProfessionalTheme::colour(ProfessionalTheme::DarkOrange::PanelHeader);
                    panelBorderColor = ProfessionalTheme::colour(ProfessionalTheme::DarkOrange::PanelBorder);
                    buttonColor = ProfessionalTheme::colour(ProfessionalTheme::DarkOrange::ButtonNormal);
                    buttonHoverColor = ProfessionalTheme::colour(ProfessionalTheme::DarkOrange::ButtonHover);
                    buttonDownColor = ProfessionalTheme::colour(ProfessionalTheme::DarkOrange::ButtonPressed);
                    sliderColor = ProfessionalTheme::colour(ProfessionalTheme::DarkOrange::SliderFill);
                    sliderTrackColor = ProfessionalTheme::colour(ProfessionalTheme::DarkOrange::SliderTrack);
                    sliderThumbColor = ProfessionalTheme::colour(ProfessionalTheme::DarkOrange::SliderThumb);
                    waveformFillColor = ProfessionalTheme::colour(ProfessionalTheme::DarkOrange::WaveformFill);
                    waveformOutlineColor = ProfessionalTheme::colour(ProfessionalTheme::DarkOrange::WaveformOutline);
                    midiNoteColor = ProfessionalTheme::colour(ProfessionalTheme::DarkOrange::MIDINote);
                    playheadColor = ProfessionalTheme::colour(ProfessionalTheme::DarkOrange::Playhead);
                    gridLineColor = ProfessionalTheme::colour(ProfessionalTheme::DarkOrange::GridLine);
                    successColor = ProfessionalTheme::colour(ProfessionalTheme::DarkOrange::Success);
                    warningColor = ProfessionalTheme::colour(ProfessionalTheme::DarkOrange::Warning);
                    errorColor = ProfessionalTheme::colour(ProfessionalTheme::DarkOrange::Error);
                    infoColor = ProfessionalTheme::colour(ProfessionalTheme::DarkOrange::Info);
                    break;

                case ProfessionalTheme::ColorScheme::DarkBlue:
                    backgroundColor = ProfessionalTheme::colour(ProfessionalTheme::DarkBlue::Background);
                    backgroundLighterColor = ProfessionalTheme::colour(ProfessionalTheme::DarkBlue::BackgroundLighter);
                    accentColor = ProfessionalTheme::colour(ProfessionalTheme::DarkBlue::AccentPrimary);
                    textPrimaryColor = ProfessionalTheme::colour(ProfessionalTheme::DarkBlue::TextPrimary);
                    textSecondaryColor = ProfessionalTheme::colour(ProfessionalTheme::DarkBlue::TextSecondary);
                    panelBackgroundColor = ProfessionalTheme::colour(ProfessionalTheme::DarkBlue::PanelBackground);
                    panelHeaderColor = ProfessionalTheme::colour(ProfessionalTheme::DarkBlue::PanelHeader);
                    panelBorderColor = ProfessionalTheme::colour(ProfessionalTheme::DarkBlue::PanelBorder);
                    buttonColor = ProfessionalTheme::colour(ProfessionalTheme::DarkBlue::ButtonNormal);
                    buttonHoverColor = ProfessionalTheme::colour(ProfessionalTheme::DarkBlue::ButtonHover);
                    buttonDownColor = ProfessionalTheme::colour(ProfessionalTheme::DarkBlue::ButtonPressed);
                    sliderColor = ProfessionalTheme::colour(ProfessionalTheme::DarkBlue::SliderFill);
                    sliderTrackColor = ProfessionalTheme::colour(ProfessionalTheme::DarkBlue::SliderTrack);
                    sliderThumbColor = ProfessionalTheme::colour(ProfessionalTheme::DarkBlue::SliderThumb);
                    waveformFillColor = ProfessionalTheme::colour(ProfessionalTheme::DarkBlue::WaveformFill);
                    waveformOutlineColor = ProfessionalTheme::colour(ProfessionalTheme::DarkBlue::WaveformOutline);
                    midiNoteColor = ProfessionalTheme::colour(ProfessionalTheme::DarkBlue::MIDINote);
                    playheadColor = ProfessionalTheme::colour(ProfessionalTheme::DarkBlue::Playhead);
                    gridLineColor = ProfessionalTheme::colour(ProfessionalTheme::DarkBlue::GridLine);
                    successColor = ProfessionalTheme::colour(ProfessionalTheme::DarkBlue::Success);
                    warningColor = ProfessionalTheme::colour(ProfessionalTheme::DarkBlue::Warning);
                    errorColor = ProfessionalTheme::colour(ProfessionalTheme::DarkBlue::Error);
                    infoColor = ProfessionalTheme::colour(ProfessionalTheme::DarkBlue::Info);
                    break;

                case ProfessionalTheme::ColorScheme::DarkGray:
                    backgroundColor = ProfessionalTheme::colour(ProfessionalTheme::DarkGray::Background);
                    backgroundLighterColor = ProfessionalTheme::colour(ProfessionalTheme::DarkGray::BackgroundLighter);
                    accentColor = ProfessionalTheme::colour(ProfessionalTheme::DarkGray::AccentPrimary);
                    textPrimaryColor = ProfessionalTheme::colour(ProfessionalTheme::DarkGray::TextPrimary);
                    textSecondaryColor = ProfessionalTheme::colour(ProfessionalTheme::DarkGray::TextSecondary);
                    panelBackgroundColor = ProfessionalTheme::colour(ProfessionalTheme::DarkGray::PanelBackground);
                    panelHeaderColor = ProfessionalTheme::colour(ProfessionalTheme::DarkGray::PanelHeader);
                    panelBorderColor = ProfessionalTheme::colour(ProfessionalTheme::DarkGray::PanelBorder);
                    buttonColor = ProfessionalTheme::colour(ProfessionalTheme::DarkGray::ButtonNormal);
                    buttonHoverColor = ProfessionalTheme::colour(ProfessionalTheme::DarkGray::ButtonHover);
                    buttonDownColor = ProfessionalTheme::colour(ProfessionalTheme::DarkGray::ButtonPressed);
                    sliderColor = ProfessionalTheme::colour(ProfessionalTheme::DarkGray::SliderFill);
                    sliderTrackColor = ProfessionalTheme::colour(ProfessionalTheme::DarkGray::SliderTrack);
                    sliderThumbColor = ProfessionalTheme::colour(ProfessionalTheme::DarkGray::SliderThumb);
                    waveformFillColor = ProfessionalTheme::colour(ProfessionalTheme::DarkGray::WaveformFill);
                    waveformOutlineColor = ProfessionalTheme::colour(ProfessionalTheme::DarkGray::WaveformOutline);
                    midiNoteColor = ProfessionalTheme::colour(ProfessionalTheme::DarkGray::MIDINote);
                    playheadColor = ProfessionalTheme::colour(ProfessionalTheme::DarkGray::Playhead);
                    gridLineColor = ProfessionalTheme::colour(ProfessionalTheme::DarkGray::GridLine);
                    successColor = ProfessionalTheme::colour(ProfessionalTheme::DarkGray::Success);
                    warningColor = ProfessionalTheme::colour(ProfessionalTheme::DarkGray::Warning);
                    errorColor = ProfessionalTheme::colour(ProfessionalTheme::DarkGray::Error);
                    infoColor = ProfessionalTheme::colour(ProfessionalTheme::DarkGray::Info);
                    break;
            }
            break;

        case ThemeType::NeonSakura:
            // Neon colors (hardcoded for now)
            backgroundColor = juce::Colour(15, 15, 25);
            backgroundLighterColor = juce::Colour(25, 25, 40);
            accentColor = juce::Colour(255, 20, 147);  // Neon Pink
            textPrimaryColor = juce::Colour(255, 255, 255);
            textSecondaryColor = juce::Colour(200, 200, 200);
            panelBackgroundColor = juce::Colour(25, 25, 40);
            panelHeaderColor = juce::Colour(35, 35, 50);
            panelBorderColor = juce::Colour(45, 45, 60);
            buttonColor = juce::Colour(40, 40, 55);
            buttonHoverColor = juce::Colour(50, 50, 65);
            buttonDownColor = juce::Colour(30, 30, 45);
            sliderColor = juce::Colour(255, 20, 147);
            sliderTrackColor = juce::Colour(40, 40, 55);
            sliderThumbColor = juce::Colour(0, 255, 255);  // Neon Cyan
            waveformFillColor = juce::Colour(0x33FF1493);
            waveformOutlineColor = juce::Colour(255, 20, 147);
            midiNoteColor = juce::Colour(0, 255, 255);
            playheadColor = juce::Colour(255, 255, 255);
            gridLineColor = juce::Colour(35, 35, 50);
            successColor = juce::Colour(0, 255, 128);
            warningColor = juce::Colour(255, 165, 0);
            errorColor = juce::Colour(255, 60, 60);
            infoColor = juce::Colour(0, 255, 255);
            break;

        case ThemeType::Custom:
            // Custom colors are set via setCustomThemeColors()
            break;
    }

    if (onThemeChanged)
        onThemeChanged(type);
}

void ThemeManager::setCustomThemeColors(const juce::Colour& background,
                                        const juce::Colour& backgroundLighter,
                                        const juce::Colour& accent,
                                        const juce::Colour& textPrimary,
                                        const juce::Colour& textSecondary,
                                        const juce::Colour& panelBackground,
                                        const juce::Colour& panelHeader,
                                        const juce::Colour& panelBorder,
                                        const juce::Colour& button,
                                        const juce::Colour& buttonHover,
                                        const juce::Colour& buttonDown,
                                        const juce::Colour& slider,
                                        const juce::Colour& sliderTrack,
                                        const juce::Colour& sliderThumb,
                                        const juce::Colour& waveformFill,
                                        const juce::Colour& waveformOutline,
                                        const juce::Colour& midiNote,
                                        const juce::Colour& playhead,
                                        const juce::Colour& gridLine,
                                        const juce::Colour& success,
                                        const juce::Colour& warning,
                                        const juce::Colour& error,
                                        const juce::Colour& info)
{
    backgroundColor = background;
    backgroundLighterColor = backgroundLighter;
    accentColor = accent;
    textPrimaryColor = textPrimary;
    textSecondaryColor = textSecondary;
    panelBackgroundColor = panelBackground;
    panelHeaderColor = panelHeader;
    panelBorderColor = panelBorder;
    buttonColor = button;
    buttonHoverColor = buttonHover;
    buttonDownColor = buttonDown;
    sliderColor = slider;
    sliderTrackColor = sliderTrack;
    sliderThumbColor = sliderThumb;
    waveformFillColor = waveformFill;
    waveformOutlineColor = waveformOutline;
    midiNoteColor = midiNote;
    playheadColor = playhead;
    gridLineColor = gridLine;
    successColor = success;
    warningColor = warning;
    errorColor = error;
    infoColor = info;

    currentTheme = ThemeType::Custom;

    if (onThemeChanged)
        onThemeChanged(ThemeType::Custom);
}

// Color Accessors
juce::Colour ThemeManager::getBackgroundColor() const { return backgroundColor; }
juce::Colour ThemeManager::getBackgroundLighterColor() const { return backgroundLighterColor; }
juce::Colour ThemeManager::getAccentColor() const { return accentColor; }
juce::Colour ThemeManager::getTextPrimaryColor() const { return textPrimaryColor; }
juce::Colour ThemeManager::getTextSecondaryColor() const { return textSecondaryColor; }
juce::Colour ThemeManager::getPanelBackgroundColor() const { return panelBackgroundColor; }
juce::Colour ThemeManager::getPanelHeaderColor() const { return panelHeaderColor; }
juce::Colour ThemeManager::getPanelBorderColor() const { return panelBorderColor; }
juce::Colour ThemeManager::getButtonColor() const { return buttonColor; }
juce::Colour ThemeManager::getButtonHoverColor() const { return buttonHoverColor; }
juce::Colour ThemeManager::getButtonDownColor() const { return buttonDownColor; }
juce::Colour ThemeManager::getSliderColor() const { return sliderColor; }
juce::Colour ThemeManager::getSliderTrackColor() const { return sliderTrackColor; }
juce::Colour ThemeManager::getSliderThumbColor() const { return sliderThumbColor; }
juce::Colour ThemeManager::getWaveformFillColor() const { return waveformFillColor; }
juce::Colour ThemeManager::getWaveformOutlineColor() const { return waveformOutlineColor; }
juce::Colour ThemeManager::getMidiNoteColor() const { return midiNoteColor; }
juce::Colour ThemeManager::getPlayheadColor() const { return playheadColor; }
juce::Colour ThemeManager::getGridLineColor() const { return gridLineColor; }
juce::Colour ThemeManager::getSuccessColor() const { return successColor; }
juce::Colour ThemeManager::getWarningColor() const { return warningColor; }
juce::Colour ThemeManager::getErrorColor() const { return errorColor; }
juce::Colour ThemeManager::getInfoColor() const { return infoColor; }

// Color Scheme
ProfessionalTheme::ColorScheme ThemeManager::getColorScheme() const { return currentColorScheme; }

void ThemeManager::setColorScheme(ProfessionalTheme::ColorScheme scheme)
{
    currentColorScheme = scheme;
    setCurrentTheme(currentTheme);  // Refresh colors
}

void ThemeManager::cycleColorScheme()
{
    currentColorScheme = getNextColorScheme();
    setCurrentTheme(currentTheme);  // Refresh colors
}

ProfessionalTheme::ColorScheme ThemeManager::getNextColorScheme() const
{
    switch (currentColorScheme)
    {
        case ProfessionalTheme::ColorScheme::DarkOrange:
            return ProfessionalTheme::ColorScheme::DarkBlue;
        case ProfessionalTheme::ColorScheme::DarkBlue:
            return ProfessionalTheme::ColorScheme::DarkGray;
        case ProfessionalTheme::ColorScheme::DarkGray:
        default:
            return ProfessionalTheme::ColorScheme::DarkOrange;
    }
}

// Apply Theme to LookAndFeel
void ThemeManager::applyToLookAndFeel(juce::LookAndFeel& lf)
{
    // Apply colors to LookAndFeel_V4 using setColour
    lf.setColour(juce::ResizableWindow::backgroundColourId, backgroundColor);
    lf.setColour(juce::DocumentWindow::backgroundColourId, backgroundColor);
    lf.setColour(juce::TextButton::buttonColourId, buttonColor);
    lf.setColour(juce::TextButton::buttonOnColourId, accentColor);
    lf.setColour(juce::TextButton::textColourOnId, textPrimaryColor);
    lf.setColour(juce::TextButton::textColourOffId, textPrimaryColor);
    lf.setColour(juce::ComboBox::backgroundColourId, buttonColor);
    lf.setColour(juce::ComboBox::textColourId, textPrimaryColor);
    lf.setColour(juce::ComboBox::outlineColourId, panelBorderColor);
    lf.setColour(juce::Slider::backgroundColourId, backgroundColor);
    lf.setColour(juce::Slider::trackColourId, sliderTrackColor);
    lf.setColour(juce::Slider::thumbColourId, sliderThumbColor);
    lf.setColour(juce::Label::textColourId, textPrimaryColor);
}

void ThemeManager::applyToComponent(juce::Component& component)
{
    component.setColour(juce::DocumentWindow::backgroundColourId, backgroundColor);
}
