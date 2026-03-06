#include "TestCoordinator.h"
#include <juce_core/juce_core.h>
#include <algorithm>
#include <thread>
#include <atomic>

TestCoordinator::TestCoordinator()
{
}

TestCoordinator::~TestCoordinator()
{
}

TestCoordinator& TestCoordinator::getInstance()
{
    static TestCoordinator instance;
    return instance;
}
