// ============================================================================
// PanelFeedbackSystem.cpp - Feedback System für Panels
// ============================================================================

#include "PanelFeedbackSystem.h"
#include "FloatingPanelEnums.h"
#include <cmath>

//==============================================================================
// Konstruktor & Destruktor
//==============================================================================
PanelFeedbackSystem::PanelFeedbackSystem()
{
    // Timer mit 60Hz starten für Feedback-Animationen
    startTimerHz(60);
    setInterceptsMouseClicks(false, false);
}

PanelFeedbackSystem::~PanelFeedbackSystem()
{
    stopTimer();
    stopAllFeedback();
}

//==============================================================================
// Snap-Zone Management
//==============================================================================
void PanelFeedbackSystem::registerSnapZone(PanelSnapZone zone, const juce::Rectangle<int>& bounds, float cornerRadius)
{
    SnapZonePreview preview;
    preview.zone = zone;
    preview.bounds = bounds;
    preview.isActive = false;
    preview.opacity = 0.0f;
    preview.targetOpacity = 0.7f;
    preview.cornerRadius = cornerRadius;

    // Standardfarben
    preview.fillColour = snapZonePreviewColour.withAlpha(0.2f);
    preview.borderColour = snapZoneBorderColour;

    snapZones[zone] = preview;
}

void PanelFeedbackSystem::unregisterSnapZone(PanelSnapZone zone)
{
    snapZones.erase(zone);
}

void PanelFeedbackSystem::setSnapZoneBounds(PanelSnapZone zone, const juce::Rectangle<int>& bounds)
{
    auto it = snapZones.find(zone);
    if (it != snapZones.end())
        it->second.bounds = bounds;
}

juce::Rectangle<int> PanelFeedbackSystem::getSnapZoneBounds(PanelSnapZone zone) const
{
    auto it = snapZones.find(zone);
    if (it != snapZones.end())
        return it->second.bounds;
    return {};
}

void PanelFeedbackSystem::activateSnapZone(PanelSnapZone zone)
{
    auto it = snapZones.find(zone);
    if (it != snapZones.end())
    {
        it->second.isActive = true;
        it->second.targetOpacity = 0.7f;
        activeSnapZone = zone;
        triggerSnapZoneActivationCallback(zone);
    }
}

void PanelFeedbackSystem::deactivateSnapZone(PanelSnapZone zone)
{
    auto it = snapZones.find(zone);
    if (it != snapZones.end())
    {
        it->second.isActive = false;
        it->second.targetOpacity = 0.0f;
    }
}

void PanelFeedbackSystem::deactivateAllSnapZones()
{
    for (auto& pair : snapZones)
    {
        pair.second.isActive = false;
        pair.second.targetOpacity = 0.0f;
    }
    activeSnapZone = PanelSnapZone::None;
}

bool PanelFeedbackSystem::isSnapZoneActive(PanelSnapZone zone) const
{
    auto it = snapZones.find(zone);
    return it != snapZones.end() && it->second.isActive;
}

//==============================================================================
// Feedback auslösen
//==============================================================================
void PanelFeedbackSystem::showDockingSuccess(juce::Component* component, int duration)
{
    if (!component)
        return;

    showFeedback(FeedbackType::DockingSuccess, component,
                 component->getBounds(), duration, FeedbackStyle::Normal);
}

void PanelFeedbackSystem::showDockingFail(juce::Component* component, int duration)
{
    if (!component)
        return;

    showFeedback(FeedbackType::DockingFail, component,
                 component->getBounds(), duration, FeedbackStyle::Strong);

    // Zusätzlich Shake anzeigen
    showShake(component, 15);
}

void PanelFeedbackSystem::showUndocking(juce::Component* component, int duration)
{
    if (!component)
        return;

    showFeedback(FeedbackType::Undocking, component,
                 component->getBounds(), duration, FeedbackStyle::Subtle);
}

void PanelFeedbackSystem::showMinimizeFeedback(juce::Component* component)
{
    if (!component)
        return;

    showFeedback(FeedbackType::Minimize, component,
                 component->getBounds(), 150, FeedbackStyle::Subtle);
}

void PanelFeedbackSystem::showMaximizeFeedback(juce::Component* component)
{
    if (!component)
        return;

    showFeedback(FeedbackType::Maximize, component,
                 component->getBounds(), 150, FeedbackStyle::Normal);
}

void PanelFeedbackSystem::showResizeFeedback(juce::Component* component, const juce::Rectangle<int>& newBounds)
{
    if (!component)
        return;

    showFeedback(FeedbackType::Resize, component, newBounds, 100, FeedbackStyle::Subtle);
}

void PanelFeedbackSystem::showHighlight(juce::Component* component, const juce::Rectangle<int>& bounds, int duration)
{
    if (!component)
        return;

    showFeedback(FeedbackType::Highlight, component, bounds, duration, FeedbackStyle::Normal);
}

void PanelFeedbackSystem::showGlow(juce::Component* component, const juce::Rectangle<int>& bounds, int duration)
{
    if (!component)
        return;

    showFeedback(FeedbackType::Glow, component, bounds, duration, FeedbackStyle::Normal);
}

void PanelFeedbackSystem::showPulse(juce::Component* component, int pulseCount)
{
    if (!component)
        return;

    const int duration = pulseCount * 300;

    auto animation = std::make_unique<FeedbackAnimation>();
    animation->type = FeedbackType::Pulse;
    animation->targetComponent = component;
    animation->bounds = component->getBounds();
    animation->duration = duration;
    animation->startTime = juce::Time::getMillisecondCounter();
    animation->isActive = true;
    animation->autoRemove = true;
    animation->colour = getFeedbackColour(FeedbackType::Pulse, FeedbackStyle::Normal);

    animation->onComplete = [this, component]()
    {
        triggerFeedbackEndCallback(FeedbackType::Pulse, component);
    };

    triggerFeedbackStartCallback(FeedbackType::Pulse, component);
    feedbackAnimations.push_back(std::move(animation));
}

void PanelFeedbackSystem::showShake(juce::Component* component, int intensity)
{
    if (!component)
        return;

    auto animation = std::make_unique<FeedbackAnimation>();
    animation->type = FeedbackType::Shake;
    animation->targetComponent = component;
    animation->bounds = component->getBounds();
    animation->duration = 400;
    animation->startTime = juce::Time::getMillisecondCounter();
    animation->isActive = true;
    animation->autoRemove = true;

    // Custom onUpdate für Shake-Effekt
    const auto originalBounds = component->getBounds();
    animation->onComplete = [component, originalBounds]()
    {
        if (component)
            component->setBounds(originalBounds);
    };

    triggerFeedbackStartCallback(FeedbackType::Shake, component);
    feedbackAnimations.push_back(std::move(animation));
}

void PanelFeedbackSystem::showDragStart(juce::Component* component)
{
    if (!component)
        return;

    showFeedback(FeedbackType::DragStart, component,
                 component->getBounds(), 100, FeedbackStyle::Subtle);
}

void PanelFeedbackSystem::showDragEnd(juce::Component* component)
{
    if (!component)
        return;

    showFeedback(FeedbackType::DragEnd, component,
                 component->getBounds(), 100, FeedbackStyle::Subtle);
}

//==============================================================================
// Benutzerdefiniertes Feedback
//==============================================================================
void PanelFeedbackSystem::showFeedback(FeedbackType type, juce::Component* component,
                                       const juce::Rectangle<int>& bounds, int duration,
                                       FeedbackStyle style)
{
    if (!component)
        return;

    auto animation = std::make_unique<FeedbackAnimation>();
    animation->type = type;
    animation->targetComponent = component;
    animation->bounds = bounds;
    animation->duration = duration;
    animation->startTime = juce::Time::getMillisecondCounter();
    animation->opacity = 0.0f;
    animation->targetOpacity = 1.0f;
    animation->isActive = true;
    animation->autoRemove = true;
    animation->colour = getFeedbackColour(type, style);
    animation->glowColour = getFeedbackGlowColour(type, style);

    animation->onComplete = [this, type, component]()
    {
        triggerFeedbackEndCallback(type, component);
    };

    triggerFeedbackStartCallback(type, component);
    feedbackAnimations.push_back(std::move(animation));
}

void PanelFeedbackSystem::addFeedbackAnimation(const FeedbackAnimation& animation)
{
    auto anim = std::make_unique<FeedbackAnimation>(animation);
    anim->startTime = juce::Time::getMillisecondCounter();
    anim->isActive = true;

    triggerFeedbackStartCallback(anim->type, anim->targetComponent);
    feedbackAnimations.push_back(std::move(anim));
}

//==============================================================================
// Feedback-Steuerung
//==============================================================================
void PanelFeedbackSystem::stopFeedbackForComponent(juce::Component* component)
{
    feedbackAnimations.erase(
        std::remove_if(feedbackAnimations.begin(), feedbackAnimations.end(),
            [component](const std::unique_ptr<FeedbackAnimation>& a)
            {
                return a->targetComponent == component;
            }),
        feedbackAnimations.end()
    );
}

void PanelFeedbackSystem::stopFeedbackType(FeedbackType type)
{
    feedbackAnimations.erase(
        std::remove_if(feedbackAnimations.begin(), feedbackAnimations.end(),
            [type](const std::unique_ptr<FeedbackAnimation>& a)
            {
                return a->type == type;
            }),
        feedbackAnimations.end()
    );
}

void PanelFeedbackSystem::stopAllFeedback()
{
    feedbackAnimations.clear();
}

void PanelFeedbackSystem::clearAllSnapZones()
{
    snapZones.clear();
}

//==============================================================================
// Feedback-Status
//==============================================================================
bool PanelFeedbackSystem::hasFeedbackForComponent(juce::Component* component) const
{
    for (const auto& animation : feedbackAnimations)
    {
        if (animation->targetComponent == component && animation->isActive)
            return true;
    }
    return false;
}

bool PanelFeedbackSystem::hasFeedbackType(FeedbackType type) const
{
    for (const auto& animation : feedbackAnimations)
    {
        if (animation->type == type && animation->isActive)
            return true;
    }
    return false;
}

int PanelFeedbackSystem::getActiveFeedbackCount() const
{
    int count = 0;
    for (const auto& animation : feedbackAnimations)
    {
        if (animation->isActive)
            count++;
    }
    return count;
}

//==============================================================================
// Appearance
//==============================================================================
void PanelFeedbackSystem::setSnapZonePreviewColour(const juce::Colour& colour)
{
    snapZonePreviewColour = colour;
    // Aktive Snap-Zonen aktualisieren
    for (auto& pair : snapZones)
    {
        pair.second.fillColour = colour.withAlpha(0.2f);
    }
}

void PanelFeedbackSystem::setSnapZoneBorderColour(const juce::Colour& colour)
{
    snapZoneBorderColour = colour;
    for (auto& pair : snapZones)
    {
        pair.second.borderColour = colour;
    }
}

void PanelFeedbackSystem::setGlowColour(const juce::Colour& colour)
{
    glowColour = colour;
}

void PanelFeedbackSystem::setSnapZoneBorderThickness(float thickness)
{
    snapZoneBorderThickness = juce::jmax(1.0f, thickness);
}

//==============================================================================
// Callbacks
//==============================================================================
void PanelFeedbackSystem::setSnapZoneActivationCallback(SnapZoneCallback callback)
{
    snapZoneActivationCallback = callback;
}

void PanelFeedbackSystem::setFeedbackStartCallback(FeedbackStartCallback callback)
{
    feedbackStartCallback = callback;
}

void PanelFeedbackSystem::setFeedbackEndCallback(FeedbackEndCallback callback)
{
    feedbackEndCallback = callback;
}

//==============================================================================
// JUCE Component Overrides
//==============================================================================
void PanelFeedbackSystem::paint(juce::Graphics& g)
{
    // Snap-Zone Previews zeichnen
    for (const auto& pair : snapZones)
    {
        drawSnapZonePreview(g, pair.second);
    }

    // Feedback-Animationen zeichnen
    for (const auto& animation : feedbackAnimations)
    {
        drawFeedbackAnimation(g, *animation);
    }
}

void PanelFeedbackSystem::resized()
{
    // Nichts zu tun hier - Bounds werden extern gesetzt
}

//==============================================================================
// Private Methods
//==============================================================================
void PanelFeedbackSystem::timerCallback()
{
    // Snap-Zone Opacity aktualisieren
    const float opacityStep = 0.1f;

    for (auto& pair : snapZones)
    {
        auto& preview = pair.second;

        if (std::abs(preview.opacity - preview.targetOpacity) > 0.01f)
        {
            if (preview.opacity < preview.targetOpacity)
                preview.opacity = juce::jmin(preview.targetOpacity, preview.opacity + opacityStep);
            else
                preview.opacity = juce::jmax(preview.targetOpacity, preview.opacity - opacityStep);

            repaint(preview.bounds);
        }
    }

    // Feedback-Animationen aktualisieren
    updateFeedbackAnimations();
}

void PanelFeedbackSystem::updateFeedbackAnimations()
{
    const int currentTime = juce::Time::getMillisecondCounter();
    std::vector<size_t> toRemove;

    for (size_t i = 0; i < feedbackAnimations.size(); ++i)
    {
        auto& animation = feedbackAnimations[i];
        if (!animation->isActive)
            continue;

        const int elapsedTime = currentTime - animation->startTime;
        animation->progress = juce::jlimit(0.0f, 1.0f,
                                           static_cast<float>(elapsedTime) / static_cast<float>(animation->duration));

        // Shake-Animation speziell behandeln
        if (animation->type == FeedbackType::Shake && animation->targetComponent)
        {
            const auto originalBounds = animation->targetComponent->getBounds();
            const float shakeOffset = std::sin(animation->progress * juce::MathConstants<float>::pi * 6.0f) *
                                      (1.0f - animation->progress) * 10.0f;

            animation->targetComponent->setBounds(
                originalBounds.getX() + static_cast<int>(shakeOffset),
                originalBounds.getY(),
                originalBounds.getWidth(),
                originalBounds.getHeight()
            );
        }
        else
        {
            // Fade-In und Fade-Out basierend auf Progress
            if (animation->progress < 0.5f)
                animation->opacity = animation->progress * 2.0f;
            else
                animation->opacity = (1.0f - animation->progress) * 2.0f;

            // Pulse-Effekt für Pulse-Feedback
            if (animation->type == FeedbackType::Pulse)
            {
                const float pulseProgress = animation->progress * 6.283185f * 3.0f; // 2π * 3 pulses
                const float pulseValue = (std::sin(pulseProgress) + 1.0f) * 0.5f;
                animation->currentScale = 1.0f + pulseValue * 0.1f;
            }
        }

        // Bounds repainten
        if (animation->targetComponent)
            repaint(animation->targetComponent->getBounds());
        else
            repaint(animation->bounds);

        // Animation abgeschlossen?
        if (animation->progress >= 1.0f)
        {
            if (animation->onComplete)
                animation->onComplete();

            if (animation->autoRemove)
                toRemove.push_back(i);
            else
                animation->isActive = false;
        }
    }

    // Abgeschlossene Animationen entfernen
    for (auto it = toRemove.rbegin(); it != toRemove.rend(); ++it)
    {
        feedbackAnimations.erase(feedbackAnimations.begin() + static_cast<std::ptrdiff_t>(*it));
    }
}

void PanelFeedbackSystem::applyFeedbackAnimation(FeedbackAnimation* animation)
{
    if (!animation)
        return;

    // Animation wird in paint() gezeichnet, hier nichts zu tun
}

juce::Colour PanelFeedbackSystem::getFeedbackColour(FeedbackType type, FeedbackStyle style) const
{
    switch (type)
    {
        case FeedbackType::DockingSuccess:
            return juce::Colour(0xFF4CAF50); // Grün

        case FeedbackType::DockingFail:
            return juce::Colour(0xFFF44336); // Rot

        case FeedbackType::Undocking:
            return juce::Colour(0xFF2196F3); // Blau

        case FeedbackType::Minimize:
        case FeedbackType::Maximize:
            return juce::Colour(0xFFFFA726); // Orange

        case FeedbackType::Resize:
            return juce::Colour(0xFF9E9E9E); // Grau

        case FeedbackType::Highlight:
            return juce::Colour(0xFF4A90E2).withAlpha(style == FeedbackStyle::Strong ? 0.4f : 0.2f);

        case FeedbackType::Glow:
            return glowColour.withAlpha(0.6f);

        case FeedbackType::Pulse:
            return juce::Colour(0xFF5BA3F5).withAlpha(0.3f);

        case FeedbackType::Shake:
            return juce::Colour(0xFFF44336).withAlpha(0.4f);

        case FeedbackType::DragStart:
            return juce::Colour(0xFF2196F3).withAlpha(0.3f);

        case FeedbackType::DragEnd:
            return juce::Colour(0xFF4CAF50).withAlpha(0.3f);

        case FeedbackType::ZoneActive:
            return juce::Colour(0xFF4A90E2).withAlpha(0.5f);

        default:
            return juce::Colours::white.withAlpha(0.2f);
    }
}

juce::Colour PanelFeedbackSystem::getFeedbackGlowColour(FeedbackType type, FeedbackStyle style) const
{
    switch (type)
    {
        case FeedbackType::DockingSuccess:
            return juce::Colour(0xFF66BB6A);

        case FeedbackType::DockingFail:
            return juce::Colour(0xFFEF5350);

        case FeedbackType::Glow:
            return glowColour;

        default:
            return getFeedbackColour(type, style).brighter(0.2f);
    }
}

void PanelFeedbackSystem::triggerSnapZoneActivationCallback(PanelSnapZone zone)
{
    if (snapZoneActivationCallback)
        snapZoneActivationCallback(zone);
}

void PanelFeedbackSystem::triggerFeedbackStartCallback(FeedbackType type, juce::Component* component)
{
    if (feedbackStartCallback)
        feedbackStartCallback(type, component);
}

void PanelFeedbackSystem::triggerFeedbackEndCallback(FeedbackType type, juce::Component* component)
{
    if (feedbackEndCallback)
        feedbackEndCallback(type, component);
}

void PanelFeedbackSystem::drawSnapZonePreview(juce::Graphics& g, const SnapZonePreview& preview)
{
    if (preview.opacity <= 0.01f || !preview.bounds.isEmpty())
        return;

    const auto bounds = preview.bounds.toFloat();

    // Gradient für den Snap-Zone Preview
    juce::ColourGradient gradient(preview.fillColour.withAlpha(preview.opacity * 0.3f),
                                  bounds.getCentre(),
                                  preview.fillColour.withAlpha(0.0f),
                                  bounds.getBottomRight(),
                                  true);
    g.setGradientFill(gradient);

    // Gefülltes Rechteck mit Corner Radius
    g.fillRoundedRectangle(bounds, preview.cornerRadius);

    // Border zeichnen
    g.setColour(preview.borderColour.withAlpha(preview.opacity * 0.6f));
    g.drawRoundedRectangle(bounds.reduced(1.0f), preview.cornerRadius, snapZoneBorderThickness);
}

void PanelFeedbackSystem::drawFeedbackAnimation(juce::Graphics& g, const FeedbackAnimation& animation)
{
    if (animation.opacity <= 0.01f || !animation.isActive)
        return;

    const auto bounds = animation.bounds.toFloat();

    // Glow-Effekt für bestimmte Feedback-Typen
    if (animation.type == FeedbackType::Glow || animation.type == FeedbackType::Highlight)
    {
        const float glowSize = animation.bounds.getWidth() * 0.02f;
        const auto glowBounds = bounds.expanded(glowSize);

        juce::ColourGradient glowGradient(animation.glowColour.withAlpha(animation.opacity * 0.5f),
                                          glowBounds.getCentre(),
                                          juce::Colours::transparentBlack,
                                          glowBounds.getBottomRight(),
                                          true);
        g.setGradientFill(glowGradient);
        g.fillRoundedRectangle(glowBounds, 12.0f);
    }

    // Border zeichnen
    g.setColour(animation.colour.withAlpha(animation.opacity));

    switch (animation.type)
    {
        case FeedbackType::DockingSuccess:
        case FeedbackType::DockingFail:
            // Dicker Border für Feedback
            g.drawRoundedRectangle(bounds, 8.0f, 3.0f);
            break;

        case FeedbackType::Resize:
            // Corner-Indikatoren für Resize
            drawResizeIndicators(g, bounds);
            break;

        default:
            g.drawRoundedRectangle(bounds, 8.0f, 2.0f);
            break;
    }

    // Glow-Effekt für Glow und Pulse
    if (animation.type == FeedbackType::Glow || animation.type == FeedbackType::Pulse)
    {
        g.setColour(animation.glowColour.withAlpha(animation.opacity * 0.4f));
        g.fillRoundedRectangle(bounds, 8.0f);
    }
}

void PanelFeedbackSystem::drawResizeIndicators(juce::Graphics& g, const juce::Rectangle<float>& bounds)
{
    const float indicatorSize = 8.0f;
    const float indicatorThickness = 3.0f;

    // Vier Ecken
    const auto drawCorner = [&](float x, float y, bool top, bool left)
    {
        const auto cornerBounds = juce::Rectangle<float>(x, y, indicatorSize, indicatorSize);

        if (top)
        {
            // Obere Linie
            g.drawLine(juce::Line<float>(cornerBounds.getBottomLeft(), cornerBounds.getTopRight()), indicatorThickness);
        }
        else
        {
            // Untere Linie
            g.drawLine(juce::Line<float>(cornerBounds.getTopLeft(), cornerBounds.getBottomRight()), indicatorThickness);
        }

        if (left)
        {
            // Linke Linie
            g.drawLine(juce::Line<float>(cornerBounds.getTopRight(), cornerBounds.getBottomLeft()), indicatorThickness);
        }
        else
        {
            // Rechte Linie
            g.drawLine(juce::Line<float>(cornerBounds.getTopLeft(), cornerBounds.getBottomRight()), indicatorThickness);
        }
    };

    drawCorner(bounds.getX() - indicatorSize / 2, bounds.getY() - indicatorSize / 2, true, true);
    drawCorner(bounds.getRight() - indicatorSize / 2, bounds.getY() - indicatorSize / 2, true, false);
    drawCorner(bounds.getX() - indicatorSize / 2, bounds.getBottom() - indicatorSize / 2, false, true);
    drawCorner(bounds.getRight() - indicatorSize / 2, bounds.getBottom() - indicatorSize / 2, false, false);
}
