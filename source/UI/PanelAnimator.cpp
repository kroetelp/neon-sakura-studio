// ============================================================================
// PanelAnimator.cpp - Animation System für Panels
// ============================================================================

#include "PanelAnimator.h"
#include <cmath>

//==============================================================================
// Konstruktor & Destruktor
//==============================================================================
PanelAnimator::PanelAnimator()
{
    // Timer mit 60Hz starten
    startTimerHz(60);
}

PanelAnimator::~PanelAnimator()
{
    stopTimer();
    stopAllAnimations();
}

//==============================================================================
// Grundlegende Animationen
//==============================================================================
int PanelAnimator::fadeIn(juce::Component* component, int duration, EasingFunction easing)
{
    if (!component || !animationsEnabled)
        return -1;

    Animation animation;
    animation.targetComponent = component;
    animation.type = AnimationType::FadeIn;
    animation.easing = easing;
    animation.duration = duration;
    animation.startTime = juce::Time::getMillisecondCounter();
    animation.startOpacity = 0.0f;
    animation.endOpacity = 1.0f;

    return animate(animation);
}

int PanelAnimator::fadeOut(juce::Component* component, int duration, bool autoHide, EasingFunction easing)
{
    if (!component || !animationsEnabled)
        return -1;

    Animation animation;
    animation.targetComponent = component;
    animation.type = AnimationType::FadeOut;
    animation.easing = easing;
    animation.duration = duration;
    animation.startTime = juce::Time::getMillisecondCounter();
    animation.startOpacity = component->getAlpha();
    animation.endOpacity = 0.0f;

    if (autoHide)
    {
        animation.onComplete = [component]()
        {
            if (component)
                component->setVisible(false);
        };
    }

    return animate(animation);
}

int PanelAnimator::slideIn(juce::Component* component, AnimationType fromDirection,
                          int duration, EasingFunction easing)
{
    if (!component || !animationsEnabled)
        return -1;

    Animation animation;
    animation.targetComponent = component;
    animation.type = fromDirection;
    animation.easing = easing;
    animation.duration = duration;
    animation.startTime = juce::Time::getMillisecondCounter();

    const auto targetBounds = component->getBounds();
    animation.startBounds = calculateStartBounds(fromDirection, component);
    animation.endBounds = targetBounds;
    animation.startOpacity = 0.0f;
    animation.endOpacity = 1.0f;

    // Komponente auf Start-Bounds setzen
    component->setBounds(animation.startBounds);
    component->setVisible(true);

    return animate(animation);
}

int PanelAnimator::slideOut(juce::Component* component, AnimationType toDirection,
                           int duration, bool autoHide, EasingFunction easing)
{
    if (!component || !animationsEnabled)
        return -1;

    Animation animation;
    animation.targetComponent = component;
    animation.type = toDirection;
    animation.easing = easing;
    animation.duration = duration;
    animation.startTime = juce::Time::getMillisecondCounter();

    const auto currentBounds = component->getBounds();
    animation.startBounds = currentBounds;
    animation.endBounds = calculateEndBounds(toDirection, component);
    animation.startOpacity = component->getAlpha();
    animation.endOpacity = 0.0f;

    if (autoHide)
    {
        animation.onComplete = [component]()
        {
            if (component)
                component->setVisible(false);
        };
    }

    return animate(animation);
}

int PanelAnimator::scaleIn(juce::Component* component, int duration, EasingFunction easing)
{
    if (!component || !animationsEnabled)
        return -1;

    Animation animation;
    animation.targetComponent = component;
    animation.type = AnimationType::ScaleIn;
    animation.easing = easing;
    animation.duration = duration;
    animation.startTime = juce::Time::getMillisecondCounter();
    animation.startScale = 0.0f;
    animation.endScale = 1.0f;
    animation.startOpacity = 0.0f;
    animation.endOpacity = 1.0f;

    const auto bounds = component->getBounds();
    const auto center = bounds.getCentre();
    const auto width = bounds.getWidth();
    const auto height = bounds.getHeight();

    animation.startBounds = juce::Rectangle<int>(center.x, center.y, 0, 0);
    animation.endBounds = bounds;

    component->setBounds(animation.startBounds);
    component->setVisible(true);

    return animate(animation);
}

int PanelAnimator::scaleOut(juce::Component* component, int duration, bool autoHide, EasingFunction easing)
{
    if (!component || !animationsEnabled)
        return -1;

    Animation animation;
    animation.targetComponent = component;
    animation.type = AnimationType::ScaleOut;
    animation.easing = easing;
    animation.duration = duration;
    animation.startTime = juce::Time::getMillisecondCounter();
    animation.startScale = 1.0f;
    animation.endScale = 0.0f;
    animation.startOpacity = component->getAlpha();
    animation.endOpacity = 0.0f;

    const auto bounds = component->getBounds();
    const auto center = bounds.getCentre();
    const auto width = bounds.getWidth();
    const auto height = bounds.getHeight();

    animation.startBounds = bounds;
    animation.endBounds = juce::Rectangle<int>(center.x, center.y, 0, 0);

    if (autoHide)
    {
        animation.onComplete = [component]()
        {
            if (component)
                component->setVisible(false);
        };
    }

    return animate(animation);
}

//==============================================================================
// Erweiterte Animationen
//==============================================================================
int PanelAnimator::bounce(juce::Component* component, int duration)
{
    if (!component || !animationsEnabled)
        return -1;

    Animation animation;
    animation.targetComponent = component;
    animation.type = AnimationType::Bounce;
    animation.easing = EasingFunction::EaseOutBounce;
    animation.duration = duration;
    animation.startTime = juce::Time::getMillisecondCounter();
    animation.startScale = 0.8f;
    animation.endScale = 1.0f;

    const auto bounds = component->getBounds();
    const auto center = bounds.getCentre();
    const auto width = bounds.getWidth();
    const auto height = bounds.getHeight();

    animation.startBounds = juce::Rectangle<int>(
        center.x - static_cast<int>(width * 0.4f),
        center.y - static_cast<int>(height * 0.4f),
        static_cast<int>(width * 0.8f),
        static_cast<int>(height * 0.8f)
    );
    animation.endBounds = bounds;

    component->setBounds(animation.startBounds);

    return animate(animation);
}

int PanelAnimator::pulse(juce::Component* component, int pulseCount, int pulseDuration)
{
    if (!component || !animationsEnabled)
        return -1;

    Animation animation;
    animation.targetComponent = component;
    animation.type = AnimationType::Pulse;
    animation.easing = EasingFunction::EaseInOut;
    animation.duration = pulseDuration * pulseCount;
    animation.startTime = juce::Time::getMillisecondCounter();
    animation.loopCount = pulseCount;
    animation.currentLoop = 0;
    animation.autoReverse = true;
    animation.startScale = 1.0f;
    animation.endScale = 1.2f;

    return animate(animation);
}

int PanelAnimator::shake(juce::Component* component, int shakeIntensity, int duration)
{
    if (!component || !animationsEnabled)
        return -1;

    Animation animation;
    animation.targetComponent = component;
    animation.type = AnimationType::Pulse; // Wir nutzen Pulse für Shake mit Custom Update
    animation.easing = EasingFunction::EaseInOut;
    animation.duration = duration;
    animation.startTime = juce::Time::getMillisecondCounter();

    const auto originalBounds = component->getBounds();
    animation.onUpdate = [component, originalBounds, shakeIntensity](float progress)
    {
        if (!component)
            return;

        // Shake-Effekt: Sinus-Wellen auf X-Achse
        const float shakeOffset = std::sin(progress * juce::MathConstants<float>::pi * 4.0f) *
                                  shakeIntensity * (1.0f - progress);

        component->setBounds(
            originalBounds.getX() + static_cast<int>(shakeOffset),
            originalBounds.getY(),
            originalBounds.getWidth(),
            originalBounds.getHeight()
        );
    };

    return animate(animation);
}

//==============================================================================
// Benutzerdefinierte Animationen
//==============================================================================
int PanelAnimator::animate(const Animation& animation)
{
    if (!animationsEnabled)
        return -1;

    auto anim = std::make_unique<Animation>(animation);
    anim->state = AnimationState::Running;
    anim->progress = 0.0f;

    const int id = createAnimationId();
    animationMap[id] = anim.get();
    animations.push_back(std::move(anim));

    triggerGlobalStartCallback(*animationMap[id]);

    if (animationMap[id]->onStart)
        animationMap[id]->onStart();

    return id;
}

int PanelAnimator::animateBounds(juce::Component* component, const juce::Rectangle<int>& startBounds,
                                const juce::Rectangle<int>& endBounds, int duration, EasingFunction easing)
{
    if (!component || !animationsEnabled)
        return -1;

    Animation animation;
    animation.targetComponent = component;
    animation.type = AnimationType::FadeIn; // Bounds-Animation nutzen Fade-In als Type
    animation.easing = easing;
    animation.duration = duration;
    animation.startTime = juce::Time::getMillisecondCounter();
    animation.startBounds = startBounds;
    animation.endBounds = endBounds;

    return animate(animation);
}

int PanelAnimator::animateOpacity(juce::Component* component, float startOpacity, float endOpacity,
                                 int duration, EasingFunction easing)
{
    if (!component || !animationsEnabled)
        return -1;

    Animation animation;
    animation.targetComponent = component;
    animation.type = AnimationType::FadeIn;
    animation.easing = easing;
    animation.duration = duration;
    animation.startTime = juce::Time::getMillisecondCounter();
    animation.startOpacity = startOpacity;
    animation.endOpacity = endOpacity;

    return animate(animation);
}

//==============================================================================
// Animations-Steuerung
//==============================================================================
void PanelAnimator::stopAnimation(int animationId, bool complete)
{
    auto it = animationMap.find(animationId);
    if (it == animationMap.end())
        return;

    Animation* animation = it->second;

    if (complete && animation->onComplete)
        animation->onComplete();

    animation->state = AnimationState::Cancelled;
    triggerGlobalCancelCallback(*animation);

    if (animation->onCancel)
        animation->onCancel();

    removeAnimation(animationId);
}

void PanelAnimator::pauseAnimation(int animationId)
{
    auto it = animationMap.find(animationId);
    if (it == animationMap.end())
        return;

    Animation* animation = it->second;
    animation->state = AnimationState::Paused;
}

void PanelAnimator::resumeAnimation(int animationId)
{
    auto it = animationMap.find(animationId);
    if (it == animationMap.end())
        return;

    Animation* animation = it->second;
    animation->state = AnimationState::Running;
    animation->startTime = juce::Time::getMillisecondCounter() -
                           static_cast<int>(animation->progress * animation->duration);
}

void PanelAnimator::stopAllAnimationsForComponent(juce::Component* component)
{
    std::vector<int> idsToRemove;

    for (const auto& pair : animationMap)
    {
        if (pair.second->targetComponent == component)
            idsToRemove.push_back(pair.first);
    }

    for (int id : idsToRemove)
        stopAnimation(id);
}

void PanelAnimator::stopAllAnimations()
{
    std::vector<int> ids;
    for (const auto& pair : animationMap)
        ids.push_back(pair.first);

    for (int id : ids)
        stopAnimation(id);
}

//==============================================================================
// Animations-Status
//==============================================================================
bool PanelAnimator::isAnimationRunning(int animationId) const
{
    auto it = animationMap.find(animationId);
    return it != animationMap.end() && it->second->state == AnimationState::Running;
}

bool PanelAnimator::isComponentAnimating(juce::Component* component) const
{
    for (const auto& pair : animationMap)
    {
        if (pair.second->targetComponent == component && pair.second->state == AnimationState::Running)
            return true;
    }
    return false;
}

int PanelAnimator::getActiveAnimationCount() const
{
    int count = 0;
    for (const auto& pair : animationMap)
    {
        if (pair.second->state == AnimationState::Running)
            count++;
    }
    return count;
}

//==============================================================================
// Queue Management
//==============================================================================
void PanelAnimator::queueAnimation(const Animation& animation, int priority)
{
    auto anim = std::make_unique<Animation>(animation);
    animationQueue.push_back(std::move(anim));
}

void PanelAnimator::processNextQueuedAnimation()
{
    if (animationQueue.empty() || hasActiveAnimations())
        return;

    auto animation = std::move(animationQueue.front());
    animationQueue.erase(animationQueue.begin());
    animate(*animation);
}

void PanelAnimator::clearQueue()
{
    animationQueue.clear();
}

int PanelAnimator::getQueueSize() const
{
    return static_cast<int>(animationQueue.size());
}

//==============================================================================
// Callbacks
//==============================================================================
void PanelAnimator::setAnimationStartCallback(std::function<void(const Animation&)> callback)
{
    globalStartCallback = callback;
}

void PanelAnimator::setAnimationUpdateCallback(std::function<void(const Animation&, float)> callback)
{
    globalUpdateCallback = callback;
}

void PanelAnimator::setAnimationCompleteCallback(std::function<void(const Animation&)> callback)
{
    globalCompleteCallback = callback;
}

void PanelAnimator::setAnimationCancelCallback(std::function<void(const Animation&)> callback)
{
    globalCancelCallback = callback;
}

//==============================================================================
// Performance & Optimization
//==============================================================================
void PanelAnimator::setFrameRate(int fps)
{
    frameRate = juce::jlimit(1, 144, fps);
    if (isTimerRunning())
        startTimerHz(frameRate);
}

//==============================================================================
// Preset Animationen
//==============================================================================
void PanelAnimator::panelFadeIn(juce::Component* component)
{
    fadeIn(component, 150, EasingFunction::EaseOutQuad);
}

void PanelAnimator::panelFadeOut(juce::Component* component)
{
    fadeOut(component, 150, true, EasingFunction::EaseInQuad);
}

void PanelAnimator::dockAnimation(juce::Component* component)
{
    slideIn(component, AnimationType::SlideInRight, 250, EasingFunction::EaseOutBack);
}

void PanelAnimator::undockAnimation(juce::Component* component)
{
    slideOut(component, AnimationType::SlideOutRight, 250, true, EasingFunction::EaseInBack);
}

void PanelAnimator::openPanelAnimation(juce::Component* component)
{
    scaleIn(component, 200, EasingFunction::EaseOutElastic);
}

void PanelAnimator::closePanelAnimation(juce::Component* component)
{
    scaleOut(component, 200, true, EasingFunction::EaseInQuad);
}

//==============================================================================
// Private Methods
//==============================================================================
void PanelAnimator::timerCallback()
{
    const int currentTime = juce::Time::getMillisecondCounter();
    std::vector<int> completedAnimations;

    for (const auto& pair : animationMap)
    {
        Animation* animation = pair.second;

        if (animation->state != AnimationState::Running)
            continue;

        updateAnimation(animation, currentTime);

        if (animation->state == AnimationState::Completed)
            completedAnimations.push_back(pair.first);
    }

    // Abgeschlossene Animationen aufräumen
    for (int id : completedAnimations)
    {
        auto it = animationMap.find(id);
        if (it != animationMap.end() && it->second->onComplete)
            it->second->onComplete();

        triggerGlobalCompleteCallback(*it->second);
        removeAnimation(id);
    }

    // Nächste Queue-Animation starten
    if (!hasActiveAnimations())
        processNextQueuedAnimation();
}

int PanelAnimator::createAnimationId()
{
    return nextAnimationId++;
}

void PanelAnimator::addAnimation(std::unique_ptr<Animation> animation)
{
    const int id = createAnimationId();
    animationMap[id] = animation.get();
    animations.push_back(std::move(animation));
}

void PanelAnimator::removeAnimation(int animationId)
{
    auto it = animationMap.find(animationId);
    if (it == animationMap.end())
        return;

    Animation* animPtr = it->second;
    animationMap.erase(it);

    // Aus dem Vector entfernen
    animations.erase(
        std::remove_if(animations.begin(), animations.end(),
            [animPtr](const std::unique_ptr<Animation>& a) { return a.get() == animPtr; }),
        animations.end()
    );
}

void PanelAnimator::updateAnimation(Animation* animation, int currentTime)
{
    if (!animation || !animation->targetComponent)
        return;

    const int elapsedTime = currentTime - animation->startTime;
    float t = static_cast<float>(elapsedTime) / static_cast<float>(animation->duration);
    t = juce::jlimit(0.0f, 1.0f, t);

    animation->progress = t;

    // Easing anwenden
    const float easedProgress = applyEasing(t, animation->easing);

    // Animation anwenden
    applyAnimation(animation);

    // Custom Update Callback
    if (animation->onUpdate)
        animation->onUpdate(easedProgress);

    // Global Update Callback
    triggerGlobalUpdateCallback(*animation, easedProgress);

    // Prüfen ob Animation abgeschlossen
    if (t >= 1.0f)
    {
        if (animation->autoReverse && (animation->loopCount < 0 || animation->currentLoop < animation->loopCount))
        {
            // Animation umkehren
            std::swap(animation->startBounds, animation->endBounds);
            std::swap(animation->startOpacity, animation->endOpacity);
            std::swap(animation->startScale, animation->endScale);
            animation->startTime = currentTime;
            animation->currentLoop++;
        }
        else if (animation->loopCount < 0 || animation->currentLoop < animation->loopCount)
        {
            // Animation loopen
            animation->startTime = currentTime;
            animation->currentLoop++;
        }
        else
        {
            animation->state = AnimationState::Completed;
        }
    }
}

void PanelAnimator::applyAnimation(Animation* animation)
{
    if (!animation || !animation->targetComponent)
        return;

    const float t = applyEasing(animation->progress, animation->easing);

    // Bounds animieren
    if (animation->startBounds != animation->endBounds)
    {
        const int x = juce::roundToInt(
            animation->startBounds.getX() +
            (animation->endBounds.getX() - animation->startBounds.getX()) * t
        );
        const int y = juce::roundToInt(
            animation->startBounds.getY() +
            (animation->endBounds.getY() - animation->startBounds.getY()) * t
        );
        const int width = juce::roundToInt(
            animation->startBounds.getWidth() +
            (animation->endBounds.getWidth() - animation->startBounds.getWidth()) * t
        );
        const int height = juce::roundToInt(
            animation->startBounds.getHeight() +
            (animation->endBounds.getHeight() - animation->startBounds.getHeight()) * t
        );

        animation->targetComponent->setBounds(x, y, width, height);
    }

    // Opacity animieren
    if (std::abs(animation->startOpacity - animation->endOpacity) > 0.01f)
    {
        const float opacity = animation->startOpacity +
                              (animation->endOpacity - animation->startOpacity) * t;
        animation->targetComponent->setAlpha(juce::jlimit(0.0f, 1.0f, opacity));
    }

    // Scale animieren (durch Bounds)
    if (std::abs(animation->startScale - animation->endScale) > 0.01f &&
        animation->startBounds != animation->endBounds)
    {
        const auto originalBounds = animation->endBounds;
        const auto center = originalBounds.getCentre();
        const float currentScale = animation->startScale +
                                   (animation->endScale - animation->startScale) * t;

        const int width = juce::roundToInt(originalBounds.getWidth() * currentScale);
        const int height = juce::roundToInt(originalBounds.getHeight() * currentScale);

        animation->targetComponent->setBounds(
            center.x - width / 2,
            center.y - height / 2,
            width,
            height
        );
    }
}

float PanelAnimator::applyEasing(float t, EasingFunction easing)
{
    switch (easing)
    {
        case EasingFunction::Linear:
            return t;

        case EasingFunction::EaseIn:
            return t * t;

        case EasingFunction::EaseOut:
            return t * (2.0f - t);

        case EasingFunction::EaseInOut:
            return t < 0.5f ? 2.0f * t * t : -1.0f + (4.0f - 2.0f * t) * t;

        case EasingFunction::EaseInQuad:
            return t * t;

        case EasingFunction::EaseOutQuad:
            return t * (2.0f - t);

        case EasingFunction::EaseInOutQuad:
            return t < 0.5f ? 2.0f * t * t : -1.0f + (4.0f - 2.0f * t) * t;

        case EasingFunction::EaseInCubic:
            return t * t * t;

        case EasingFunction::EaseOutCubic:
            return (--t) * t * t + 1.0f;

        case EasingFunction::EaseInOutCubic:
            return t < 0.5f ? 4.0f * t * t * t : (t - 1.0f) * (2.0f * t - 2.0f) * (2.0f * t - 2.0f) + 1.0f;

        case EasingFunction::EaseInElastic:
        {
            const float c4 = (2.0f * juce::MathConstants<float>::pi) / 3.0f;
            return t == 0.0f ? 0.0f :
                   t == 1.0f ? 1.0f :
                   -std::pow(2.0f, 10.0f * t - 10.0f) * std::sin((t * 10.0f - 10.75f) * c4);
        }

        case EasingFunction::EaseOutElastic:
        {
            const float c4 = (2.0f * juce::MathConstants<float>::pi) / 3.0f;
            return t == 0.0f ? 0.0f :
                   t == 1.0f ? 1.0f :
                   std::pow(2.0f, -10.0f * t) * std::sin((t * 10.0f - 0.75f) * c4) + 1.0f;
        }

        case EasingFunction::EaseInOutElastic:
        {
            const float c5 = (2.0f * juce::MathConstants<float>::pi) / 4.5f;
            return t == 0.0f ? 0.0f :
                   t == 1.0f ? 1.0f :
                   t < 0.5f ?
                   -(std::pow(2.0f, 20.0f * t - 10.0f) * std::sin((20.0f * t - 11.125f) * c5)) / 2.0f :
                   (std::pow(2.0f, -20.0f * t + 10.0f) * std::sin((20.0f * t - 11.125f) * c5)) / 2.0f + 1.0f;
        }

        case EasingFunction::EaseOutBounce:
        {
            const float n1 = 7.5625f;
            const float d1 = 2.75f;

            if (t < 1.0f / d1)
            {
                return n1 * t * t;
            }
            else if (t < 2.0f / d1)
            {
                return n1 * (t -= 1.5f / d1) * t + 0.75f;
            }
            else if (t < 2.5f / d1)
            {
                return n1 * (t -= 2.25f / d1) * t + 0.9375f;
            }
            else
            {
                return n1 * (t -= 2.625f / d1) * t + 0.984375f;
            }
        }

        case EasingFunction::EaseInBounce:
            return 1.0f - applyEasing(1.0f - t, EasingFunction::EaseOutBounce);

        case EasingFunction::EaseInOutBounce:
            return t < 0.5f ?
                   (1.0f - applyEasing(1.0f - 2.0f * t, EasingFunction::EaseOutBounce)) / 2.0f :
                   (1.0f + applyEasing(2.0f * t - 1.0f, EasingFunction::EaseOutBounce)) / 2.0f;

        case EasingFunction::EaseOutBack:
        {
            const float c1 = 1.70158f;
            const float c3 = c1 + 1.0f;
            return 1.0f + c3 * std::pow(t - 1.0f, 3.0f) + c1 * std::pow(t - 1.0f, 2.0f);
        }

        case EasingFunction::EaseInBack:
        {
            const float c1 = 1.70158f;
            const float c3 = c1 + 1.0f;
            return c3 * t * t * t - c1 * t * t;
        }

        default:
            return t;
    }
}

juce::Rectangle<int> PanelAnimator::calculateStartBounds(AnimationType type, juce::Component* component)
{
    if (!component)
        return {};

    const auto bounds = component->getBounds();

    switch (type)
    {
        case AnimationType::SlideInLeft:
            return bounds.withX(bounds.getX() - bounds.getWidth());

        case AnimationType::SlideInRight:
            return bounds.withX(bounds.getRight());

        case AnimationType::SlideInTop:
            return bounds.withY(bounds.getY() - bounds.getHeight());

        case AnimationType::SlideInBottom:
            return bounds.withY(bounds.getBottom());

        default:
            return bounds;
    }
}

juce::Rectangle<int> PanelAnimator::calculateEndBounds(AnimationType type, juce::Component* component)
{
    if (!component)
        return {};

    const auto bounds = component->getBounds();

    switch (type)
    {
        case AnimationType::SlideOutLeft:
            return bounds.withX(bounds.getX() - bounds.getWidth());

        case AnimationType::SlideOutRight:
            return bounds.withX(bounds.getRight());

        case AnimationType::SlideOutTop:
            return bounds.withY(bounds.getY() - bounds.getHeight());

        case AnimationType::SlideOutBottom:
            return bounds.withY(bounds.getBottom());

        default:
            return bounds;
    }
}

void PanelAnimator::triggerGlobalStartCallback(const Animation& animation)
{
    if (globalStartCallback)
        globalStartCallback(animation);
}

void PanelAnimator::triggerGlobalUpdateCallback(const Animation& animation, float progress)
{
    if (globalUpdateCallback)
        globalUpdateCallback(animation, progress);
}

void PanelAnimator::triggerGlobalCompleteCallback(const Animation& animation)
{
    if (globalCompleteCallback)
        globalCompleteCallback(animation);
}

void PanelAnimator::triggerGlobalCancelCallback(const Animation& animation)
{
    if (globalCancelCallback)
        globalCancelCallback(animation);
}
