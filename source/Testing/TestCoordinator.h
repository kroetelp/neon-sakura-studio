// ============================================================================
// TestCoordinator.h - Testing System
// ============================================================================

#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include <memory>
#include <functional>

/**
 * TestCoordinator - Testing System
 *
 * TODO: Vollständige Implementierung
 */
class TestCoordinator
{
public:
    TestCoordinator();
    ~TestCoordinator();

    static TestCoordinator& getInstance();

private:
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(TestCoordinator)
};
