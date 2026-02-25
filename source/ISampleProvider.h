#pragma once

/**
 * ISampleProvider - Interface for sample-related operations
 *
 * This interface abstracts sample management, allowing PatternGenerator
 * and other components to access sample data without coupling to MainComponent.
 */

#include <juce_core/juce_core.h>

class ISampleProvider
{
public:
    virtual ~ISampleProvider() = default;

    // Get available sample categories (e.g., "Kick", "Snare", etc.)
    virtual juce::StringArray getSampleCategories() const = 0;

    // Get the current sample directory
    virtual juce::File getSampleDirectory() const = 0;

    // Load a sample for a specific track by category
    virtual void loadSampleForCategory(int trackIndex, const juce::String& category) = 0;

    // Set the sample directory
    virtual void setSampleDirectory(const juce::File& directory) = 0;

    // Scan a directory for sample categories
    virtual void scanSampleDirectory(const juce::File& directory) = 0;
};
