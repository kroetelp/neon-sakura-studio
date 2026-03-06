// ============================================================================
// WorkspacePresetUI.h - Workspace Preset UI
// ============================================================================

#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include "../Theme/ThemeManager.h"
#include "../Theme/WorkspaceManager.h"
#include "../Theme/ExtendedThemeSystem.h"
#include "../Theme/WorkspacePreset.h"
#include "../UI/FloatingPanelEnums.h"
#include <memory>
#include <functional>

// Forward Declarations
class DockingManager;

/**
 * PresetListboxModel - Model für die Preset-Listbox
 */
class PresetListboxModel : public juce::ListBoxModel
{
public:
    //==============================================================================
    // Callback-Typen
    //==============================================================================
    using PresetSelectedCallback = std::function<void(const WorkspacePreset&)>;
    using PresetDeleteCallback = std::function<void(const WorkspacePreset&)>;

    //==============================================================================
    // Konstruktor & Destruktor
    //==============================================================================
    PresetListboxModel();
    ~PresetListboxModel() override;

    //==============================================================================
    // Preset Management
    //==============================================================================
    /**
     * Setzt die Liste der Presets
     */
    void setPresets(const std::vector<WorkspacePreset>& presets);

    /**
     * Gibt die Liste der Presets zurück
     */
    const std::vector<WorkspacePreset>& getPresets() const { return presets; }

    /**
     * Fügt einen Preset hinzu
     */
    void addPreset(const WorkspacePreset& preset);

    /**
     * Entfernt einen Preset
     */
    void removePreset(const juce::String& presetName);

    /**
     * Aktualisiert einen Preset
     */
    void updatePreset(const WorkspacePreset& preset);

    /**
     * Gibt einen Preset anhand des Namens zurück
     */
    std::optional<WorkspacePreset> getPreset(const juce::String& presetName) const;

    //==============================================================================
    // Callbacks
    //==============================================================================
    /**
     * Setzt den Callback, wenn ein Preset ausgewählt wird
     */
    void setPresetSelectedCallback(PresetSelectedCallback callback);

    /**
     * Setzt den Callback, wenn ein Preset gelöscht wird
     */
    void setPresetDeleteCallback(PresetDeleteCallback callback);

    //==============================================================================
    // JUCE ListBoxModel Overrides
    //==============================================================================
    int getNumRows() override;
    void paintListBoxItem(int rowNumber, juce::Graphics& g,
                         int width, int height, bool rowIsSelected) override;
    void listBoxItemClicked(int row, const juce::MouseEvent&) override;
    void listBoxItemDoubleClicked(int row, const juce::MouseEvent&) override;
    void backgroundClicked(const juce::MouseEvent&) override;
    void selectedRowsChanged(int lastRowSelected) override;
    void deleteKeyPressed(int lastRowSelected) override;

private:
    //==============================================================================
    // Private Member
    //==============================================================================
    std::vector<WorkspacePreset> presets;
    PresetSelectedCallback presetSelectedCallback;
    PresetDeleteCallback presetDeleteCallback;

    //==============================================================================
    // Private Methods
    //==============================================================================
    juce::Colour getCategoryColour(const juce::String& category) const;
    juce::String getCategoryIcon(const juce::String& category) const;

    //==============================================================================
    // Leak Detector
    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PresetListboxModel)
};

/**
 * WorkspacePresetUI - UI für Workspace Presets
 *
 * Dieses Panel bietet:
 * - Liste aller gespeicherten Workspace-Presets
 * - Vorschau des Workspace-Layouts
 * - Speichern neuer Presets
 * - Laden von Presets
 * - Löschen von Presets
 * - Kategorisierung von Presets
 * - Suche/Filter-Funktionen
 * - Import/Export von Presets
 *
 * JUCE 8 Hinweis: Verwendet juce::Component und juce::Timer
 */
class WorkspacePresetUI : public juce::Component, private juce::Timer
{
public:
    //==============================================================================
    // Konstruktor & Destruktor
    //==============================================================================
    WorkspacePresetUI();
    ~WorkspacePresetUI() override;

    //==============================================================================
    // Initialisierung
    //==============================================================================
    /**
     * Initialisiert das UI mit den notwendigen Referenzen
     * @param dockManager Der DockingManager
     * @param workspaceManager Der WorkspaceManager
     * @param themeManager Der ThemeManager
     */
    void initialize(DockingManager* dockManager, WorkspaceManager* workspaceManager,
                   ThemeManager* themeManager);

    /**
     * Setzt das ExtendedThemeSystem
     */
    void setExtendedThemeSystem(ExtendedThemeSystem* extendedTheme);

    //==============================================================================
    // Preset Management
    //==============================================================================
    /**
     * Lädt alle Presets aus dem WorkspaceManager
     */
    void loadPresets();

    /**
     * Speichert das aktuelle Layout als Preset
     * @param name Der Name des Presets
     * @param description Die Beschreibung
     * @param category Die Kategorie
     */
    void saveCurrentLayout(const juce::String& name, const juce::String& description = "",
                          const juce::String& category = "");

    /**
     * Lädt einen Preset
     * @param presetName Der Name des Presets
     */
    void loadPreset(const juce::String& presetName);

    /**
     * Löscht einen Preset
     * @param presetName Der Name des Presets
     */
    void deletePreset(const juce::String& presetName);

    /**
     * Aktualisiert einen existierenden Preset
     */
    void updatePreset(const WorkspacePreset& preset);

    //==============================================================================
    // Preset-Filterung und Suche
    //==============================================================================
    /**
     * Setzt den Such-Text
     */
    void setSearchText(const juce::String& text);

    /**
     * Setzt die Kategorie zum Filtern
     */
    void setCategoryFilter(const juce::String& category);

    /**
     * Zeigt alle Presets an (Filter zurücksetzen)
     */
    void showAllPresets();

    //==============================================================================
    // Import/Export
    //==============================================================================
    /**
     * Importiert Presets aus einer Datei
     */
    void importPresets();

    /**
     * Exportiert Presets in eine Datei
     */
    void exportPresets();

    /**
     * Exportiert einen einzelnen Preset
     */
    void exportPreset(const juce::String& presetName);

    //==============================================================================
    // Vorschau
    //==============================================================================
    /**
     * Zeigt eine Vorschau des Preset-Layouts an
     */
    void showPresetPreview(const WorkspacePreset& preset);

    /**
     * Versteckt die Vorschau
     */
    void hidePresetPreview();

    //==============================================================================
    // Kategorien
    //==============================================================================
    /**
     * Gibt alle verfügbaren Kategorien zurück
     */
    std::vector<juce::String> getCategories() const;

    /**
     * Fügt eine neue Kategorie hinzu
     */
    void addCategory(const juce::String& category);

    //==============================================================================
    // Preset-Status
    //==============================================================================
    /**
     * Prüft, ob Änderungen am aktuellen Layout vorliegen
     */
    bool hasUnsavedChanges() const;

    /**
     * Markiert das aktuelle Layout als geändert
     */
    void markAsModified();

    /**
     * Markiert das aktuelle Layout als gespeichert
     */
    void markAsSaved();

    //==============================================================================
    // Callbacks
    //==============================================================================
    /**
     * Callback-Typ für Preset-Auswahl
     */
    using PresetSelectedCallback = std::function<void(const WorkspacePreset&)>;

    /**
     * Setzt den Callback, wenn ein Preset ausgewählt wird
     */
    void setPresetSelectedCallback(PresetSelectedCallback callback);

    /**
     * Callback-Typ für Preset-Speicherung
     */
    using PresetSavedCallback = std::function<void(const WorkspacePreset&)>;

    /**
     * Setzt den Callback, wenn ein Preset gespeichert wird
     */
    void setPresetSavedCallback(PresetSavedCallback callback);

    /**
     * Callback-Typ für Preset-Ladung
     */
    using PresetLoadedCallback = std::function<void(const WorkspacePreset&)>;

    /**
     * Setzt den Callback, wenn ein Preset geladen wird
     */
    void setPresetLoadedCallback(PresetLoadedCallback callback);

    /**
     * Callback-Typ für Preset-Löschung
     */
    using PresetDeletedCallback = std::function<void(const juce::String&)>;

    /**
     * Setzt den Callback, wenn ein Preset gelöscht wird
     */
    void setPresetDeletedCallback(PresetDeletedCallback callback);

    //==============================================================================
    // JUCE Component Overrides
    //==============================================================================
    void paint(juce::Graphics& g) override;
    void resized() override;
    void visibilityChanged() override;

private:
    //==============================================================================
    // UI Components
    //==============================================================================
    std::unique_ptr<PresetListboxModel> presetListboxModel;
    std::unique_ptr<juce::ListBox> presetListBox;
    std::unique_ptr<juce::TextEditor> searchEditor;
    std::unique_ptr<juce::ComboBox> categoryComboBox;
    std::unique_ptr<juce::TextButton> saveButton;
    std::unique_ptr<juce::TextButton> loadButton;
    std::unique_ptr<juce::TextButton> deleteButton;
    std::unique_ptr<juce::TextButton> importButton;
    std::unique_ptr<juce::TextButton> exportButton;
    std::unique_ptr<juce::TextButton> refreshButton;
    std::unique_ptr<juce::TextButton> newPresetButton;
    std::unique_ptr<juce::Viewport> presetPreviewViewport;
    std::unique_ptr<juce::Component> presetPreviewComponent;
    std::unique_ptr<juce::Label> titleLabel;
    std::unique_ptr<juce::Label> infoLabel;
    std::unique_ptr<juce::Label> unsavedChangesLabel;

    //==============================================================================
    // Manager References
    //==============================================================================
    DockingManager* dockManager = nullptr;
    WorkspaceManager* workspaceManager = nullptr;
    ThemeManager* themeManager = nullptr;
    ExtendedThemeSystem* extendedThemeSystem = nullptr;

    //==============================================================================
    // Preset Data
    //==============================================================================
    std::vector<WorkspacePreset> allPresets;
    std::vector<WorkspacePreset> filteredPresets;
    juce::String currentSearchText;
    juce::String currentCategoryFilter;
    juce::String selectedPresetName;
    bool unsavedChanges = false;

    //==============================================================================
    // Callbacks
    //==============================================================================
    PresetSelectedCallback presetSelectedCallback;
    PresetSavedCallback presetSavedCallback;
    PresetLoadedCallback presetLoadedCallback;
    PresetDeletedCallback presetDeletedCallback;

    //==============================================================================
    // Private Methods
    //==============================================================================
    void timerCallback() override;

    void setupUI();
    void updatePresetList();
    void filterPresets();
    void updateButtons();
    void updateInfoLabel();

    void onPresetSelected(const WorkspacePreset& preset);
    void onPresetDoubleClicked(const WorkspacePreset& preset);
    void onPresetDeleteRequested(const WorkspacePreset& preset);

    void onSaveButtonClicked();
    void onLoadButtonClicked();
    void onDeleteButtonClicked();
    void onImportButtonClicked();
    void onExportButtonClicked();
    void onNewPresetButtonClicked();
    void onRefreshButtonClicked();
    void onSearchTextChanged();
    void onCategoryChanged();

    void showSavePresetDialog();
    void showDeleteConfirmationDialog(const juce::String& presetName);

    juce::String serializeLayout() const;
    bool deserializeLayout(const juce::String& layoutData);

    void drawPresetPreview(juce::Graphics& g, const WorkspacePreset& preset);

    //==============================================================================
    // Leak Detector
    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(WorkspacePresetUI)
};
