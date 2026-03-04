#include "ProfessionalLookAndFeel.h"

ProfessionalLookAndFeel::ProfessionalLookAndFeel()
{
    // Apply current theme colors
    getThemeManager().applyToLookAndFeel(*this);
}

// ========================================================================
// Slider Drawing
// ========================================================================

void ProfessionalLookAndFeel::drawRotarySlider(juce::Graphics& g, int x, int y, int width, int height,
                                                float sliderPos, const float rotaryStartAngle,
                                                const float rotaryEndAngle, juce::Slider& slider)
{
    auto& theme = getThemeManager();

    auto bounds = juce::Rectangle<int>(x, y, width, height).toFloat().reduced(2.0f);
    auto radius = juce::jmin(bounds.getWidth(), bounds.getHeight()) / 2.0f;
    auto toAngle = rotaryStartAngle + sliderPos * (rotaryEndAngle - rotaryStartAngle);
    auto lineWidth = std::min(3.0f, radius * 0.15f);
    auto arcRadius = radius - lineWidth / 2.0f;

    // Background arc (track)
    juce::Path backgroundArc;
    backgroundArc.addCentredArc(bounds.getCentreX(), bounds.getCentreY(), arcRadius, arcRadius,
                                 0.0f, rotaryStartAngle, rotaryEndAngle, true);
    g.setColour(theme.getSliderTrackColor());
    g.strokePath(backgroundArc, juce::PathStrokeType(lineWidth, juce::PathStrokeType::curved,
                                                      juce::PathStrokeType::rounded));

    // Value arc (fill)
    if (slider.isEnabled())
    {
        juce::Path valueArc;
        valueArc.addCentredArc(bounds.getCentreX(), bounds.getCentreY(), arcRadius, arcRadius,
                                0.0f, rotaryStartAngle, toAngle, true);
        g.setColour(theme.getAccentColor());
        g.strokePath(valueArc, juce::PathStrokeType(lineWidth, juce::PathStrokeType::curved,
                                                     juce::PathStrokeType::rounded));
    }

    // Thumb (pointer)
    auto thumbWidth = lineWidth * 2.0f;
    juce::Path thumb;
    thumb.addRectangle(-thumbWidth / 2.0f, -arcRadius - lineWidth, thumbWidth, lineWidth * 1.5f);

    g.setColour(theme.getSliderThumbColor());
    g.fillPath(thumb, juce::AffineTransform::rotation(toAngle).translated(bounds.getCentreX(), bounds.getCentreY()));

    // Center knob body
    auto knobRadius = radius * 0.5f;
    g.setColour(theme.getPanelBackgroundColor());
    g.fillEllipse(bounds.getCentreX() - knobRadius, bounds.getCentreY() - knobRadius,
                  knobRadius * 2.0f, knobRadius * 2.0f);

    // Center knob outline
    g.setColour(theme.getPanelBorderColor());
    g.drawEllipse(bounds.getCentreX() - knobRadius, bounds.getCentreY() - knobRadius,
                  knobRadius * 2.0f, knobRadius * 2.0f, 1.0f);
}

void ProfessionalLookAndFeel::drawLinearSlider(juce::Graphics& g, int x, int y, int width, int height,
                                                float sliderPos, float minSliderPos, float maxSliderPos,
                                                const juce::Slider::SliderStyle style, juce::Slider& slider)
{
    auto& theme = getThemeManager();

    if (style == juce::Slider::LinearHorizontal || style == juce::Slider::LinearBar)
    {
        // Horizontal slider
        auto trackHeight = 4.0f;
        auto trackY = y + (height - trackHeight) / 2.0f;

        // Track background
        g.setColour(theme.getSliderTrackColor());
        g.fillRoundedRectangle(juce::Rectangle<float>(static_cast<float>(x), trackY,
                                                        static_cast<float>(width), trackHeight), 2.0f);

        // Filled portion
        auto fillWidth = sliderPos - x;
        if (fillWidth > 0)
        {
            g.setColour(theme.getAccentColor().withAlpha(0.7f));
            g.fillRoundedRectangle(juce::Rectangle<float>(static_cast<float>(x), trackY,
                                                           fillWidth, trackHeight), 2.0f);
        }

        // Thumb
        auto thumbSize = 14.0f;
        auto thumbX = sliderPos - thumbSize / 2.0f;
        auto thumbY = y + (height - thumbSize) / 2.0f;

        g.setColour(theme.getSliderThumbColor());
        g.fillEllipse(thumbX, thumbY, thumbSize, thumbSize);

        // Thumb outline
        g.setColour(theme.getPanelBorderColor());
        g.drawEllipse(thumbX, thumbY, thumbSize, thumbSize, 1.0f);
    }
    else if (style == juce::Slider::LinearVertical)
    {
        // Vertical slider
        auto trackWidth = 4.0f;
        auto trackX = x + (width - trackWidth) / 2.0f;

        // Track background
        g.setColour(theme.getSliderTrackColor());
        g.fillRoundedRectangle(juce::Rectangle<float>(trackX, static_cast<float>(y),
                                                        trackWidth, static_cast<float>(height)), 2.0f);

        // Filled portion
        auto fillHeight = height - (sliderPos - y);
        if (fillHeight > 0)
        {
            g.setColour(theme.getAccentColor().withAlpha(0.7f));
            g.fillRoundedRectangle(juce::Rectangle<float>(trackX, sliderPos,
                                                           trackWidth, fillHeight), 2.0f);
        }

        // Thumb
        auto thumbSize = 14.0f;
        auto thumbX = x + (width - thumbSize) / 2.0f;
        auto thumbY = sliderPos - thumbSize / 2.0f;

        g.setColour(theme.getSliderThumbColor());
        g.fillEllipse(thumbX, thumbY, thumbSize, thumbSize);

        g.setColour(theme.getPanelBorderColor());
        g.drawEllipse(thumbX, thumbY, thumbSize, thumbSize, 1.0f);
    }
    else
    {
        // Fallback to default
        LookAndFeel_V4::drawLinearSlider(g, x, y, width, height, sliderPos,
                                          minSliderPos, maxSliderPos, style, slider);
    }
}

// ========================================================================
// Button Drawing
// ========================================================================

void ProfessionalLookAndFeel::drawButtonBackground(juce::Graphics& g, juce::Button& button,
                                                    const juce::Colour& backgroundColour,
                                                    bool shouldDrawButtonAsHighlighted,
                                                    bool shouldDrawButtonAsDown)
{
    auto& theme = getThemeManager();
    auto bounds = button.getLocalBounds().toFloat().reduced(1.0f);
    auto cornerSize = 4.0f;

    juce::Colour baseColor;

    if (button.getToggleState())
    {
        baseColor = theme.getAccentColor();
    }
    else if (shouldDrawButtonAsDown)
    {
        baseColor = theme.getButtonDownColor();
    }
    else if (shouldDrawButtonAsHighlighted)
    {
        baseColor = theme.getButtonHoverColor();
    }
    else
    {
        baseColor = theme.getButtonColor();
    }

    g.setColour(baseColor);
    g.fillRoundedRectangle(bounds, cornerSize);

    // Subtle border
    g.setColour(theme.getPanelBorderColor());
    g.drawRoundedRectangle(bounds, cornerSize, 1.0f);
}

void ProfessionalLookAndFeel::drawToggleButton(juce::Graphics& g, juce::ToggleButton& button,
                                                bool shouldDrawButtonAsHighlighted,
                                                bool shouldDrawButtonAsDown)
{
    auto& theme = getThemeManager();
    auto bounds = button.getLocalBounds().toFloat().reduced(1.0f);
    auto cornerSize = 4.0f;

    // Background
    juce::Colour bgColor;
    if (button.getToggleState())
    {
        bgColor = theme.getAccentColor();
    }
    else if (shouldDrawButtonAsDown)
    {
        bgColor = theme.getButtonDownColor();
    }
    else if (shouldDrawButtonAsHighlighted)
    {
        bgColor = theme.getButtonHoverColor();
    }
    else
    {
        bgColor = theme.getButtonColor();
    }

    g.setColour(bgColor);
    g.fillRoundedRectangle(bounds, cornerSize);

    // Border
    g.setColour(theme.getPanelBorderColor());
    g.drawRoundedRectangle(bounds, cornerSize, 1.0f);

    // Text
    g.setColour(button.getToggleState() ? theme.getBackgroundColor() : theme.getTextPrimaryColor());
    g.setFont(juce::FontOptions(14.0f, juce::Font::bold));
    g.drawText(button.getButtonText(), bounds, juce::Justification::centred);
}

void ProfessionalLookAndFeel::drawTickBox(juce::Graphics& g, juce::Component& component,
                                           float x, float y, float w, float h,
                                           bool ticked, bool isEnabled,
                                           bool shouldDrawButtonAsHighlighted,
                                           bool shouldDrawButtonAsDown)
{
    auto& theme = getThemeManager();
    auto bounds = juce::Rectangle<float>(x, y, w, h);
    auto cornerSize = 3.0f;

    // Background
    if (ticked)
    {
        g.setColour(theme.getAccentColor());
    }
    else if (shouldDrawButtonAsHighlighted)
    {
        g.setColour(theme.getButtonHoverColor());
    }
    else
    {
        g.setColour(theme.getButtonColor());
    }

    g.fillRoundedRectangle(bounds, cornerSize);

    // Border
    g.setColour(theme.getPanelBorderColor());
    g.drawRoundedRectangle(bounds, cornerSize, 1.0f);

    // Tick
    if (ticked)
    {
        juce::Path tick;
        tick.startNewSubPath(x + w * 0.25f, y + h * 0.5f);
        tick.lineTo(x + w * 0.45f, y + h * 0.7f);
        tick.lineTo(x + w * 0.75f, y + h * 0.3f);

        g.setColour(theme.getBackgroundColor());
        g.strokePath(tick, juce::PathStrokeType(2.0f, juce::PathStrokeType::curved,
                                                 juce::PathStrokeType::rounded));
    }
}

// ========================================================================
// ComboBox Drawing
// ========================================================================

void ProfessionalLookAndFeel::drawComboBox(juce::Graphics& g, int width, int height, bool isButtonDown,
                                           int buttonX, int buttonY, int buttonW, int buttonH,
                                           juce::ComboBox& box)
{
    auto& theme = getThemeManager();
    auto bounds = box.getLocalBounds().toFloat().reduced(1.0f);
    auto cornerSize = 4.0f;

    // Background
    g.setColour(isButtonDown ? theme.getButtonDownColor() : theme.getButtonColor());
    g.fillRoundedRectangle(bounds, cornerSize);

    // Border
    g.setColour(theme.getPanelBorderColor());
    g.drawRoundedRectangle(bounds, cornerSize, 1.0f);

    // Arrow
    auto arrowX = bounds.getRight() - 18.0f;
    auto arrowY = bounds.getCentreY();

    juce::Path arrow;
    arrow.startNewSubPath(arrowX - 5.0f, arrowY - 3.0f);
    arrow.lineTo(arrowX, arrowY + 3.0f);
    arrow.lineTo(arrowX + 5.0f, arrowY - 3.0f);

    g.setColour(theme.getTextSecondaryColor());
    g.strokePath(arrow, juce::PathStrokeType(2.0f));
}

void ProfessionalLookAndFeel::positionComboBoxText(juce::ComboBox& box, juce::Label& label)
{
    label.setBounds(1, 1, box.getWidth() - 25, box.getHeight() - 2);
    label.setFont(getComboBoxFont(box));
}

// ========================================================================
// Label Drawing
// ========================================================================

void ProfessionalLookAndFeel::drawLabel(juce::Graphics& g, juce::Label& label)
{
    auto& theme = getThemeManager();

    g.fillAll(label.findColour(juce::Label::backgroundColourId));

    if (!label.isBeingEdited())
    {
        auto alpha = label.isEnabled() ? 1.0f : 0.5f;
        g.setColour(label.findColour(juce::Label::textColourId).withMultipliedAlpha(alpha));

        g.setFont(label.getFont());
        g.drawText(label.getText(), label.getLocalBounds(),
                   label.getJustificationType(),
                   std::min(1.0f, label.getMinimumHorizontalScale()));
    }
    else if (label.isEnabled())
    {
        g.setColour(label.findColour(juce::Label::outlineColourId));
        g.drawRect(label.getLocalBounds());
    }
}

// ========================================================================
// Text Editor Drawing
// ========================================================================

void ProfessionalLookAndFeel::fillTextEditorBackground(juce::Graphics& g, int width, int height,
                                                        juce::TextEditor& textEditor)
{
    auto& theme = getThemeManager();

    if (textEditor.isEnabled())
    {
        g.setColour(theme.getBackgroundColor());
        g.fillRoundedRectangle(textEditor.getLocalBounds().toFloat(), 4.0f);
    }
}

void ProfessionalLookAndFeel::drawTextEditorOutline(juce::Graphics& g, int width, int height,
                                                     juce::TextEditor& textEditor)
{
    auto& theme = getThemeManager();

    if (textEditor.isEnabled())
    {
        g.setColour(textEditor.hasKeyboardFocus(true) ?
                    theme.getAccentColor() : theme.getPanelBorderColor());
        g.drawRoundedRectangle(textEditor.getLocalBounds().toFloat().reduced(0.5f), 4.0f, 1.0f);
    }
}

// ========================================================================
// Scrollbar Drawing
// ========================================================================

void ProfessionalLookAndFeel::drawScrollbar(juce::Graphics& g, juce::ScrollBar& scrollbar,
                                             int x, int y, int width, int height,
                                             bool isScrollbarVertical, int thumbPosition,
                                             int thumbSize, bool isMouseOver, bool isMouseDown)
{
    auto& theme = getThemeManager();

    // Background
    g.setColour(theme.getBackgroundColor().withAlpha(0.5f));
    g.fillRect(x, y, width, height);

    // Thumb
    auto thumbColor = isMouseDown ? theme.getAccentColor() :
                       isMouseOver ? theme.getButtonHoverColor() :
                       theme.getButtonColor();

    g.setColour(thumbColor);

    if (isScrollbarVertical)
    {
        g.fillRoundedRectangle(x + 2, thumbPosition, width - 4, thumbSize, 3.0f);
    }
    else
    {
        g.fillRoundedRectangle(thumbPosition, y + 2, thumbSize, height - 4, 3.0f);
    }
}

// ========================================================================
// Document Window Drawing
// ========================================================================

void ProfessionalLookAndFeel::drawDocumentWindowTitleBar(juce::DocumentWindow& window,
                                                          juce::Graphics& g, int w, int h,
                                                          int titleSpaceX, int titleSpaceW,
                                                          const juce::Image* icon,
                                                          bool drawTitleTextOnLeft)
{
    auto& theme = getThemeManager();

    g.setColour(theme.getPanelHeaderColor());
    g.fillAll();

    g.setColour(theme.getTextPrimaryColor());
    g.setFont(juce::FontOptions(14.0f, juce::Font::bold));
    g.drawText(window.getName(), titleSpaceX, 0, titleSpaceW, h, juce::Justification::centredLeft);
}
