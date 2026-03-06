// ============================================================================
// PanelAnimator.h - Animation System für Panels
// ============================================================================

#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include <functional>
#include <vector>
#include <memory>
#include <unordered_map>

// Forward Declarations
class juce::Component;

/**
 * AnimationType - Verschiedene Animationstypen
 */
enum class AnimationType
{
    FadeIn,          // Fade-In Animation
    FadeOut,         // Fade-Out Animation
    SlideInLeft,     // Von links einschieben
    SlideInRight,    // Von rechts einschieben
    SlideInTop,      // Von oben einschieben
    SlideInBottom,   // Von unten einschieben
    SlideOutLeft,    // Nach links rausgleiten
    SlideOutRight,   // Nach rechts rausgleiten
    SlideOutTop,     // Nach oben rausgleiten
    SlideOutBottom,  // Nach unten rausgleiten
    ScaleIn,         // Skalieren von 0 auf 1
    ScaleOut,        // Skalieren von 1 auf 0
    ElasticIn,       // Elastischer Easing-Effekt beim Start
    ElasticOut,      // Elastischer Easing-Effekt am Ende
    Bounce,          // Bounce-Effekt
    Pulse            // Pulsierende Animation
};

/**
 * EasingFunction - Easing-Funktionen für Animationen
 */
enum class EasingFunction
{
    Linear,          // Lineare Interpolation
    EaseIn,          // Langsamer Start, schneller Ende
    EaseOut,         // Schneller Start, langsames Ende
    EaseInOut,       // Langsamer Start und Ende, schneller Mitte
    EaseInQuad,      // Quadratisch EaseIn
    EaseOutQuad,     // Quadratisch EaseOut
    EaseInOutQuad,   // Quadratisch EaseInOut
    EaseInCubic,     // Kubisch EaseIn
    EaseOutCubic,    // Kubisch EaseOut
    EaseInOutCubic,  // Kubisch EaseInOut
    EaseInElastic,   // Elastisch EaseIn
    EaseOutElastic,  // Elastisch EaseOut
    EaseInOutElastic,// Elastisch EaseInOut
    EaseInBounce,    // Bounce EaseIn
    EaseOutBounce,   // Bounce EaseOut
    EaseInOutBounce, // Bounce EaseInOut
    EaseOutBack,     // EaseOut mit Back-Effekt (überschüssig am Ende)
    EaseInBack       // EaseIn mit Back-Effekt (geht zurück)
};

/**
 * AnimationState - Aktueller Status einer Animation
 */
enum class AnimationState
{
    Idle,            // Keine Animation aktiv
    Running,         // Animation läuft
    Paused,          // Animation pausiert
    Completed,       // Animation abgeschlossen
    Cancelled        // Animation abgebrochen
};

/**
 * Animation - Eine einzelne Animation
 *
 * Enthält alle Informationen über eine laufende Animation
 */
struct Animation
{
    juce::Component* targetComponent = nullptr;  // Ziel-Komponente
    AnimationType type = AnimationType::FadeIn;  // Animationstyp
    EasingFunction easing = EasingFunction::EaseInOut; // Easing-Funktion

    int duration = 200;             // Dauer in Millisekunden
    int startTime = 0;              // Startzeit
    float progress = 0.0f;          // Fortschritt (0.0 - 1.0)
    AnimationState state = AnimationState::Idle;

    juce::Rectangle<int> startBounds;     // Start-Bounds
    juce::Rectangle<int> endBounds;       // End-Bounds
    float startOpacity = 0.0f;            // Start-Opacity
    float endOpacity = 1.0f;              // End-Opacity
    float startScale = 0.0f;              // Start-Scale
    float endScale = 1.0f;                // End-Scale

    bool autoReverse = false;     // Automatisch umkehren
    int loopCount = 0;            // Wie oft loopen (0 = kein Loop, -1 = endlos)
    int currentLoop = 0;         // Aktueller Loop-Count

    // Callbacks
    std::function<void()> onStart;
    std::function<void(float)> onUpdate;
    std::function<void()> onComplete;
    std::function<void()> onCancel;
};

/**
 * PanelAnimator - Animation System für Panels
 *
 * Diese Klasse verwaltet alle Panel-Animationen mit:
 * - Timer-basierte Updates (60Hz)
 * - Verschiedene Animationstypen (Fade, Slide, Scale, Elastic, Bounce, Pulse)
 * - Easing-Funktionen für natürliche Bewegungen
 * - Animation Queueing
 * - Callbacks für Animations-Events
 * - Performance-optimiert durch object pooling
 *
 * JUCE 8 Hinweis: Diese Klasse erbt von Timer für die Animation Loop
 */
class PanelAnimator : private juce::Timer
{
public:
    //==============================================================================
    // Konstruktor & Destruktor
    //==============================================================================
    PanelAnimator();
    ~PanelAnimator() override;

    //==============================================================================
    // Grundlegende Animationen
    //==============================================================================
    /**
     * Führt eine Fade-In Animation durch
     * @param component Ziel-Komponente
     * @param duration Dauer in Millisekunden
     * @param easing Easing-Funktion
     * @return ID der gestarteten Animation
     */
    int fadeIn(juce::Component* component, int duration = 200,
               EasingFunction easing = EasingFunction::EaseOut);

    /**
     * Führt eine Fade-Out Animation durch
     * @param component Ziel-Komponente
     * @param duration Dauer in Millisekunden
     * @param autoHide Ob die Komponente nach der Animation versteckt werden soll
     * @param easing Easing-Funktion
     * @return ID der gestarteten Animation
     */
    int fadeOut(juce::Component* component, int duration = 200,
                bool autoHide = true, EasingFunction easing = EasingFunction::EaseIn);

    /**
     * Führt eine Slide-In Animation durch
     * @param component Ziel-Komponente
     * @param fromDirection Richtung (Left, Right, Top, Bottom)
     * @param duration Dauer in Millisekunden
     * @param easing Easing-Funktion
     * @return ID der gestarteten Animation
     */
    int slideIn(juce::Component* component, AnimationType fromDirection,
                int duration = 300, EasingFunction easing = EasingFunction::EaseOutBack);

    /**
     * Führt eine Slide-Out Animation durch
     * @param component Ziel-Komponente
     * @param toDirection Richtung (Left, Right, Top, Bottom)
     * @param duration Dauer in Millisekunden
     * @param autoHide Ob die Komponente nach der Animation versteckt werden soll
     * @param easing Easing-Funktion
     * @return ID der gestarteten Animation
     */
    int slideOut(juce::Component* component, AnimationType toDirection,
                 int duration = 300, bool autoHide = true,
                 EasingFunction easing = EasingFunction::EaseInBack);

    /**
     * Führt eine Scale-In Animation durch
     * @param component Ziel-Komponente
     * @param duration Dauer in Millisekunden
     * @param easing Easing-Funktion
     * @return ID der gestarteten Animation
     */
    int scaleIn(juce::Component* component, int duration = 250,
                EasingFunction easing = EasingFunction::EaseOutElastic);

    /**
     * Führt eine Scale-Out Animation durch
     * @param component Ziel-Komponente
     * @param duration Dauer in Millisekunden
     * @param autoHide Ob die Komponente nach der Animation versteckt werden soll
     * @param easing Easing-Funktion
     * @return ID der gestarteten Animation
     */
    int scaleOut(juce::Component* component, int duration = 250,
                 bool autoHide = true, EasingFunction easing = EasingFunction::EaseInQuad);

    //==============================================================================
    // Erweiterte Animationen
    //==============================================================================
    /**
     * Führt eine Bounce-Animation durch
     * @param component Ziel-Komponente
     * @param duration Dauer in Millisekunden
     * @return ID der gestarteten Animation
     */
    int bounce(juce::Component* component, int duration = 500);

    /**
     * Führt eine Pulse-Animation durch
     * @param component Ziel-Komponente
     * @param pulseCount Anzahl der Pulses
     * @param pulseDuration Dauer eines Pulses in Millisekunden
     * @return ID der gestarteten Animation
     */
    int pulse(juce::Component* component, int pulseCount = 3, int pulseDuration = 300);

    /**
     * Führt eine Shake-Animation durch
     * @param component Ziel-Komponente
     * @param shakeIntensity Intensität in Pixel
     * @param duration Dauer in Millisekunden
     * @return ID der gestarteten Animation
     */
    int shake(juce::Component* component, int shakeIntensity = 10, int duration = 400);

    //==============================================================================
    // Benutzerdefinierte Animationen
    //==============================================================================
    /**
     * Startet eine benutzerdefinierte Animation
     * @param animation Die Animations-Konfiguration
     * @return ID der gestarteten Animation
     */
    int animate(const Animation& animation);

    /**
     * Animiert Bounds einer Komponente
     * @param component Ziel-Komponente
     * @param startBounds Start-Bounds
     * @param endBounds End-Bounds
     * @param duration Dauer in Millisekunden
     * @param easing Easing-Funktion
     * @return ID der gestarteten Animation
     */
    int animateBounds(juce::Component* component, const juce::Rectangle<int>& startBounds,
                     const juce::Rectangle<int>& endBounds, int duration = 200,
                     EasingFunction easing = EasingFunction::EaseInOut);

    /**
     * Animiert Opacity einer Komponente
     * @param component Ziel-Komponente
     * @param startOpacity Start-Opacity
     * @param endOpacity End-Opacity
     * @param duration Dauer in Millisekunden
     * @param easing Easing-Funktion
     * @return ID der gestarteten Animation
     */
    int animateOpacity(juce::Component* component, float startOpacity, float endOpacity,
                      int duration = 200, EasingFunction easing = EasingFunction::EaseInOut);

    //==============================================================================
    // Animations-Steuerung
    //==============================================================================
    /**
     * Stoppt eine Animation
     * @param animationId ID der Animation
     * @param complete Ob die Animation als abgeschlossen markiert werden soll
     */
    void stopAnimation(int animationId, bool complete = false);

    /**
     * Pausiert eine Animation
     * @param animationId ID der Animation
     */
    void pauseAnimation(int animationId);

    /**
     * Setzt eine pausierte Animation fort
     * @param animationId ID der Animation
     */
    void resumeAnimation(int animationId);

    /**
     * Stoppt alle Animationen für eine Komponente
     * @param component Die Ziel-Komponente
     */
    void stopAllAnimationsForComponent(juce::Component* component);

    /**
     * Stoppt alle Animationen
     */
    void stopAllAnimations();

    //==============================================================================
    // Animations-Status
    //==============================================================================
    /**
     * Prüft, ob eine Animation läuft
     * @param animationId ID der Animation
     */
    bool isAnimationRunning(int animationId) const;

    /**
     * Prüft, ob eine Komponente animiert wird
     * @param component Die Ziel-Komponente
     */
    bool isComponentAnimating(juce::Component* component) const;

    /**
     * Gibt die Anzahl der laufenden Animationen zurück
     */
    int getActiveAnimationCount() const;

    /**
     * Prüft, ob Animationen laufen
     */
    bool hasActiveAnimations() const { return getActiveAnimationCount() > 0; }

    //==============================================================================
    // Queue Management
    //==============================================================================
    /**
     * Fügt eine Animation zur Queue hinzu
     * @param animation Die Animations-Konfiguration
     * @param priority Priorität (höhere Zahlen werden zuerst ausgeführt)
     */
    void queueAnimation(const Animation& animation, int priority = 0);

    /**
     * Startet die nächste Animation aus der Queue
     */
    void processNextQueuedAnimation();

    /**
     * Löscht alle Animationen aus der Queue
     */
    void clearQueue();

    /**
     * Gibt die Größe der Queue zurück
     */
    int getQueueSize() const;

    //==============================================================================
    // Callbacks
    //==============================================================================
    /**
     * Setzt einen Callback, der aufgerufen wird, wenn eine Animation startet
     */
    void setAnimationStartCallback(std::function<void(const Animation&)> callback);

    /**
     * Setzt einen Callback, der während der Animation aufgerufen wird
     */
    void setAnimationUpdateCallback(std::function<void(const Animation&, float)> callback);

    /**
     * Setzt einen Callback, der aufgerufen wird, wenn eine Animation abgeschlossen ist
     */
    void setAnimationCompleteCallback(std::function<void(const Animation&)> callback);

    /**
     * Setzt einen Callback, der aufgerufen wird, wenn eine Animation abgebrochen wird
     */
    void setAnimationCancelCallback(std::function<void(const Animation&)> callback);

    //==============================================================================
    // Performance & Optimization
    //==============================================================================
    /**
     * Setzt die maximale FPS für Animationen
     * @param fps Frames pro Sekunde (Standard: 60)
     */
    void setFrameRate(int fps);

    /**
     * Gibt die aktuelle FPS zurück
     */
    int getFrameRate() const { return frameRate; }

    /**
     * Aktiviert/Deaktiviert Animationen global
     * @param enabled Ob Animationen aktiviert sein sollen
     */
    void setAnimationsEnabled(bool enabled) { animationsEnabled = enabled; }

    /**
     * Prüft, ob Animationen aktiviert sind
     */
    bool areAnimationsEnabled() const { return animationsEnabled; }

    //==============================================================================
    // Preset Animationen
    //==============================================================================
    /**
     * Standard Fade-In Animation für Panels
     */
    void panelFadeIn(juce::Component* component);

    /**
     * Standard Fade-Out Animation für Panels
     */
    void panelFadeOut(juce::Component* component);

    /**
     * Standard Dock Animation (Slide-In von rechts)
     */
    void dockAnimation(juce::Component* component);

    /**
     * Standard Undock Animation (Slide-Out nach rechts)
     */
    void undockAnimation(juce::Component* component);

    /**
     * Standard Panel-Öffnungs-Animation
     */
    void openPanelAnimation(juce::Component* component);

    /**
     * Standard Panel-Schließ-Animation
     */
    void closePanelAnimation(juce::Component* component);

private:
    //==============================================================================
    // Private Member
    //==============================================================================
    // Animation Speicher
    std::vector<std::unique_ptr<Animation>> animations;
    std::vector<std::unique_ptr<Animation>> animationQueue;

    // IDs
    int nextAnimationId = 1;
    std::unordered_map<int, Animation*> animationMap;

    // Einstellungen
    int frameRate = 60;
    bool animationsEnabled = true;

    // Globale Callbacks
    std::function<void(const Animation&)> globalStartCallback;
    std::function<void(const Animation&, float)> globalUpdateCallback;
    std::function<void(const Animation&)> globalCompleteCallback;
    std::function<void(const Animation&)> globalCancelCallback;

    //==============================================================================
    // Private Methods
    //==============================================================================
    void timerCallback() override;

    int createAnimationId();
    void addAnimation(std::unique_ptr<Animation> animation);
    void removeAnimation(int animationId);
    void updateAnimation(Animation* animation, int currentTime);
    void applyAnimation(Animation* animation);
    float applyEasing(float t, EasingFunction easing);

    juce::Rectangle<int> calculateStartBounds(AnimationType type, juce::Component* component);
    juce::Rectangle<int> calculateEndBounds(AnimationType type, juce::Component* component);

    void triggerGlobalStartCallback(const Animation& animation);
    void triggerGlobalUpdateCallback(const Animation& animation, float progress);
    void triggerGlobalCompleteCallback(const Animation& animation);
    void triggerGlobalCancelCallback(const Animation& animation);

    //==============================================================================
    // Leak Detector
    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PanelAnimator)
};
