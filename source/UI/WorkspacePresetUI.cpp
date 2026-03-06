// ============================================================================
// WorkspacePresetUI.cpp - Workspace Preset UI
// ============================================================================

#include "WorkspacePresetUI.h"
#include "../DockingManager.h"
#include <optional>

//==============================================================================
// PresetListboxModel Implementation
//==============================================================================
PresetListboxModel::PresetListboxModel()
{
}

PresetListboxModel::~PresetListboxModel()
{
}

void PresetListboxModel::setPresets(const std::vector<WorkspacePreset>& newPresets)
{
    presets = newPresets;
}

void PresetListboxModel::addPreset(const WorkspacePreset& preset)
{
    presets.push_back(preset);
}

void PresetListboxModel::removePreset(const juce::String& presetName)
{
    auto it = std::remove_if(presets.begin(), presets.end(),
        [&presetName](const WorkspacePreset& p) { return p.name == presetName; });
    presets.erase(it, presets.end());
}

void PresetListboxModel::updatePreset(const WorkspacePreset& preset)
{
    for (auto& p : presets)
    {
        if (p.name == preset.name)
        {
            p.description = preset.description;
            p.themeType = preset.themeType;
            p.colorScheme = preset.colorScheme;
            p.savedTime = juce::Time::getCurrentTime();
            break;
        }
    }
}

std::optional<WorkspacePreset> PresetListboxModel::getPreset(const juce::String& presetName) const
{
    for (const auto& p : presets)
    {
        if (p.name == presetName)
            return p;
    }
    return std::nullopt;
}

void PresetListboxModel::setPresetSelectedCallback(PresetSelectedCallback callback)
{
    presetSelectedCallback = callback;
}

void PresetListboxModel::setPresetDeleteCallback(PresetDeleteCallback callback)
{
    presetDeleteCallback = callback;
}

int PresetListboxModel::getNumRows()
{
    return static_cast<int>(presets.size());
}

void PresetListboxModel::paintListBoxItem(int rowNumber, juce::Graphics& g,
                                          int width, int height, bool rowIsSelected)
{
    if (rowNumber < 0 || rowNumber >= static_cast<int>(presets.size()))
        return;

    const auto& preset = presets[static_cast<size_t>(rowNumber)];
    const auto bounds = juce::Rectangle<int>(0, 0, width, height).toFloat();

    // Hintergrund
    if (rowIsSelected)
    {
        juce::ColourGradient gradient(juce::Colour(0xFF4A90E2).withAlpha(0.4f),
                                      bounds.getTopLeft(),
                                      juce::Colour(0xFF5BA3F5).withAlpha(0.2f),
                                      bounds.getBottomRight(),
                                      false);
        g.setGradientFill(gradient);
        g.fillRoundedRectangle(bounds.reduced(2.0f), 6.0f);
    }

    // Preset-Name
    const auto nameFont = juce::Font(14.0f, juce::Font::bold);
    g.setFont(nameFont);
    g.setColour(juce::Colours::white);

    const float nameY = bounds.getY() + 12.0f;
    const float nameX = bounds.getX() + 12.0f;

    g.drawText(preset.name, nameX, nameY, bounds.getWidth() - 24.0f, 20.0f,
               juce::Justification::centredLeft, true);

    // Beschreibung
    if (!preset.description.isEmpty())
    {
        g.setFont(juce::Font(12.0f));
        g.setColour(juce::Colours::lightgrey.withAlpha(0.7f));

        g.drawText(preset.description,
                  nameX,
                  nameY + 22.0f,
                  bounds.getWidth() - 24.0f,
                  16.0f,
                  juce::Justification::centredLeft, true);
    }

    // Theme-Information
    const auto themeName = juce::String("Theme: ") + (preset.themeType == ThemeManager::ThemeType::Professional ? "Professional" : "Default");
    g.setFont(juce::Font(11.0f));
    g.setColour(juce::Colours::lightgrey);

    g.drawText(themeName, nameX, nameY + 40.0f, bounds.getWidth() - 24.0f, 16.0f,
              juce::Justification::centredLeft, true);

    // Speicherdatum
    const auto timeFont = juce::Font(10.0f);
    g.setFont(timeFont);
    g.setColour(juce::Colours::grey);

    const auto timeString = preset.savedTime.toString(true, true);
    g.drawText(timeString, bounds.getX() + 12.0f, bounds.getBottom() - 18.0f,
              bounds.getWidth() - 24.0f, 14.0f, juce::Justification::centredLeft, true);

    // Trennlinie
    g.setColour(juce::Colours::white.withAlpha(0.1f));
    g.drawLine(bounds.getX(), bounds.getBottom(), bounds.getRight(), bounds.getBottom(), 1.0f);
}

void PresetListboxModel::listBoxItemClicked(int row, const juce::MouseEvent&)
{
    if (row >= 0 && row < static_cast<int>(presets.size()))
    {
        if (presetSelectedCallback)
            presetSelectedCallback(presets[static_cast<size_t>(row)]);
    }
}

void PresetListboxModel::listBoxItemDoubleClicked(int row, const juce::MouseEvent&)
{
    if (row >= 0 && row < static_cast<int>(presets.size()))
    {
        if (presetSelectedCallback)
            presetSelectedCallback(presets[static_cast<size_t>(row)]);
    }
}

void PresetListboxModel::backgroundClicked(const juce::MouseEvent&)
{
    // Nichts zu tun
}

void PresetListboxModel::selectedRowsChanged(int lastRowSelected)
{
    // Nichts zu tun - wird über listBoxItemClicked gehandelt
}

void PresetListboxModel::deleteKeyPressed(int lastRowSelected)
{
    if (lastRowSelected >= 0 && lastRowSelected < static_cast<int>(presets.size()))
    {
        const auto& preset = presets[static_cast<size_t>(lastRowSelected)];
        // Alle Presets können gelöscht werden
        if (presetDeleteCallback)
            presetDeleteCallback(preset);
    }
}

juce::Colour PresetListboxModel::getCategoryColour(const juce::String& category) const
{
    if (category == "Synthesis")
        return juce::Colour(0xFF9C27B0);
    if (category == "Mixing")
        return juce::Colour(0xFF2196F3);
    if (category == "Performance")
        return juce::Colour(0xFFFF9800);
    if (category == "Production")
        return juce::Colour(0xFF4CAF50);
    if (category == "Custom")
        return juce::Colour(0xFF607D8B);

    return juce::Colour(0xFF9E9E9E);
}

juce::String PresetListboxModel::getCategoryIcon(const juce::String& category) const
{
    if (category == "Synthesis")
        return "⚡";
    if (category == "Mixing")
        return "🎚️";
    if (category == "Performance")
        return "🎹";
    if (category == "Production")
        return "🎛️";
    if (category == "Custom")
        return "✨";

    return "";
}

//==============================================================================
// WorkspacePresetUI Implementation
//==============================================================================
WorkspacePresetUI::WorkspacePresetUI()
{
    setupUI();
    startTimerHz(30); // 30Hz für UI-Updates
}

WorkspacePresetUI::~WorkspacePresetUI()
{
    stopTimer();
}

void WorkspacePresetUI::initialize(DockingManager* dockMgr, WorkspaceManager* workspaceMgr,
                                  ThemeManager* themeMgr)
{
    dockManager = dockMgr;
    workspaceManager = workspaceMgr;
    themeManager = themeMgr;

    loadPresets();
}

void WorkspacePresetUI::setExtendedThemeSystem(ExtendedThemeSystem* extendedTheme)
{
    extendedThemeSystem = extendedTheme;
}

//==============================================================================
// Preset Management
//==============================================================================
void WorkspacePresetUI::loadPresets()
{
    if (!workspaceManager)
        return;

    // TODO: Presets aus WorkspaceManager laden
    // vorerst mit Default-Presets initialisieren
    allPresets.clear();

    // Default-Presets erstellen
    WorkspacePreset defaultLayout;
    defaultLayout.name = "Default Layout";
    defaultLayout.description = "Standard Workspace Layout";
    defaultLayout.themeType = ThemeManager::ThemeType::Professional;
    defaultLayout.colorScheme = ProfessionalTheme::ColorScheme::DarkOrange;
    defaultLayout.showWavetable = true;
    defaultLayout.showTimeline = true;
    defaultLayout.showRhythmExplorer = false;
    defaultLayout.showMelodyPanel = false;
    defaultLayout.showPluginBrowser = false;
    defaultLayout.synthHeight = 350;
    defaultLayout.bottomTabsHeight = 400;
    defaultLayout.activeTab = 0;
    defaultLayout.bpm = 120.0;
    defaultLayout.masterVolume = 0.8f;
    defaultLayout.loopLength = 16;
    defaultLayout.swing = 0.0f;
    defaultLayout.reverb = 0.3f;
    defaultLayout.savedTime = juce::Time::getCurrentTime();
    allPresets.push_back(defaultLayout);

    WorkspacePreset synthLayout;
    synthLayout.name = "Synthesis Workspace";
    synthLayout.description = "Layout optimiert für Synthese und Sound-Design";
    synthLayout.themeType = ThemeManager::ThemeType::NeonSakura;
    synthLayout.colorScheme = ProfessionalTheme::ColorScheme::DarkBlue;
    synthLayout.showWavetable = true;
    synthLayout.showTimeline = false;
    synthLayout.showRhythmExplorer = true;
    synthLayout.showMelodyPanel = true;
    synthLayout.showPluginBrowser = false;
    synthLayout.synthHeight = 450;
    synthLayout.bottomTabsHeight = 350;
    synthLayout.activeTab = 1;
    synthLayout.bpm = 120.0;
    synthLayout.masterVolume = 0.8f;
    synthLayout.loopLength = 16;
    synthLayout.swing = 0.0f;
    synthLayout.reverb = 0.3f;
    synthLayout.savedTime = juce::Time::getCurrentTime();
    allPresets.push_back(synthLayout);

    WorkspacePreset mixLayout;
    mixLayout.name = "Mixing Workspace";
    mixLayout.description = "Layout optimiert für Mixing und Mastering";
    mixLayout.themeType = ThemeManager::ThemeType::Professional;
    mixLayout.colorScheme = ProfessionalTheme::ColorScheme::DarkGray;
    mixLayout.showWavetable = false;
    mixLayout.showTimeline = true;
    mixLayout.showRhythmExplorer = false;
    mixLayout.showMelodyPanel = false;
    mixLayout.showPluginBrowser = true;
    mixLayout.synthHeight = 250;
    mixLayout.bottomTabsHeight = 500;
    mixLayout.activeTab = 0;
    mixLayout.bpm = 120.0;
    mixLayout.masterVolume = 0.8f;
    mixLayout.loopLength = 16;
    mixLayout.swing = 0.0f;
    mixLayout.reverb = 0.3f;
    mixLayout.savedTime = juce::Time::getCurrentTime();
    allPresets.push_back(mixLayout);

    filterPresets();
}

void WorkspacePresetUI::saveCurrentLayout(const juce::String& name, const juce::String& description,
                                         const juce::String& category)
{
    if (name.isEmpty())
        return;

    WorkspacePreset newPreset;
    newPreset.name = name;
    newPreset.description = description;

    // Theme und andere Einstellungen speichern
    if (themeManager)
    {
        newPreset.themeType = themeManager->getCurrentTheme();
        newPreset.colorScheme = themeManager->getColorScheme();
    }

    // Panel-Sichtbarkeit speichern
    newPreset.showWavetable = true;
    newPreset.showTimeline = true;
    newPreset.showRhythmExplorer = false;
    newPreset.showMelodyPanel = false;
    newPreset.showPluginBrowser = false;

    // Layout-Daten speichern (in audioRoutingState)
    newPreset.audioRoutingState = serializeLayout();

    newPreset.savedTime = juce::Time::getCurrentTime();

    // Prüfen ob Preset bereits existiert
    bool found = false;
    for (auto& preset : allPresets)
    {
        if (preset.name == name)
        {
            // Bestehenden Preset aktualisieren
            preset.description = description;
            preset.themeType = newPreset.themeType;
            preset.colorScheme = newPreset.colorScheme;
            preset.audioRoutingState = newPreset.audioRoutingState;
            preset.savedTime = juce::Time::getCurrentTime();
            found = true;
            break;
        }
    }

    if (!found)
    {
        allPresets.push_back(newPreset);
    }

    filterPresets();

    if (presetSavedCallback)
        presetSavedCallback(newPreset);

    unsavedChanges = false;
    updateInfoLabel();
}

void WorkspacePresetUI::loadPreset(const juce::String& presetName)
{
    const auto preset = presetListboxModel->getPreset(presetName);
    if (preset.has_value())
    {
        // Theme anwenden
        if (themeManager)
        {
            themeManager->setCurrentTheme(preset->themeType);
            themeManager->setColorScheme(preset->colorScheme);
        }

        // AudioRoutingState laden
        if (!preset->audioRoutingState.isEmpty())
        {
            deserializeLayout(preset->audioRoutingState);
        }

        // Panel-Sichtbarkeit anwenden
        // TODO: Workspace Manager aktualisieren

        selectedPresetName = presetName;

        if (presetLoadedCallback)
            presetLoadedCallback(*preset);

        updateInfoLabel();
    }
}

void WorkspacePresetUI::deletePreset(const juce::String& presetName)
{
    const auto preset = presetListboxModel->getPreset(presetName);
    if (preset.has_value())
    {
        // Alle Presets können gelöscht werden
        showDeleteConfirmationDialog(presetName);
    }
}

void WorkspacePresetUI::updatePreset(const WorkspacePreset& preset)
{
    for (auto& p : allPresets)
    {
        if (p.name == preset.name)
        {
            // Nur updatbare Felder ändern
            p.description = preset.description;
            p.themeType = preset.themeType;
            p.colorScheme = preset.colorScheme;
            p.audioRoutingState = preset.audioRoutingState;
            p.savedTime = juce::Time::getCurrentTime();
            break;
        }
    }

    filterPresets();
}

//==============================================================================
// Preset-Filterung und Suche
//==============================================================================
void WorkspacePresetUI::setSearchText(const juce::String& text)
{
    currentSearchText = text.toLowerCase();
    filterPresets();
}

void WorkspacePresetUI::setCategoryFilter(const juce::String& category)
{
    currentCategoryFilter = category;
    filterPresets();
}

void WorkspacePresetUI::showAllPresets()
{
    currentSearchText.clear();
    currentCategoryFilter.clear();
    searchEditor->clear();
    categoryComboBox->setSelectedItemIndex(0, juce::dontSendNotification);
    filterPresets();
}

//==============================================================================
// Import/Export
//==============================================================================
void WorkspacePresetUI::importPresets()
{
    auto fileChooser = std::make_unique<juce::FileChooser>(
        "Import Workspace Presets",
        juce::File{},
        "*.xml"
    );

    fileChooser->launchAsync(juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectFiles,
        [this](const juce::FileChooser& chooser)
        {
            const auto results = chooser.getResults();
            if (results.size() > 0)
            {
                // TODO: Presets aus XML-Datei laden
                loadPresets();
            }
        }
    );
}

void WorkspacePresetUI::exportPresets()
{
    auto fileChooser = std::make_unique<juce::FileChooser>(
        "Export Workspace Presets",
        juce::File::getSpecialLocation(juce::File::userDocumentsDirectory).getChildFile("presets.xml"),
        "*.xml"
    );

    fileChooser->launchAsync(juce::FileBrowserComponent::saveMode | juce::FileBrowserComponent::canSelectFiles,
        [this](const juce::FileChooser& chooser)
        {
            const auto result = chooser.getResult();
            if (result != juce::File{})
            {
                // TODO: Presets als XML exportieren
            }
        }
    );
}

void WorkspacePresetUI::exportPreset(const juce::String& presetName)
{
    const auto preset = presetListboxModel->getPreset(presetName);
    if (!preset.has_value())
        return;

    auto fileChooser = std::make_unique<juce::FileChooser>(
        "Export Preset: " + presetName,
        juce::File::getSpecialLocation(juce::File::userDocumentsDirectory).getChildFile(presetName + ".xml"),
        "*.xml"
    );

    fileChooser->launchAsync(juce::FileBrowserComponent::saveMode | juce::FileBrowserComponent::canSelectFiles,
        [this, &preset](const juce::FileChooser& chooser)
        {
            const auto result = chooser.getResult();
            if (result != juce::File{})
            {
                // TODO: Einzelnen Preset exportieren
            }
        }
    );
}

//==============================================================================
// Vorschau
//==============================================================================
void WorkspacePresetUI::showPresetPreview(const WorkspacePreset& preset)
{
    // TODO: Vorschau implementieren
    if (presetPreviewViewport)
        presetPreviewViewport->setVisible(true);
}

void WorkspacePresetUI::hidePresetPreview()
{
    if (presetPreviewViewport)
        presetPreviewViewport->setVisible(false);
}

//==============================================================================
// Kategorien
//==============================================================================
std::vector<juce::String> WorkspacePresetUI::getCategories() const
{
    // Kategorien basierend auf Theme-Typ zurückgeben
    std::vector<juce::String> categories = {
        "Professional", "NeonSakura", "Cyberpunk", "Minimal"
    };

    // Entferne Duplikate
    std::sort(categories.begin(), categories.end());
    categories.erase(std::unique(categories.begin(), categories.end()), categories.end());

    return categories;
}

void WorkspacePresetUI::addCategory(const juce::String& category)
{
    if (categoryComboBox)
    {
        categoryComboBox->addItem(category, categoryComboBox->getNumItems() + 1);
    }
}

//==============================================================================
// Preset-Status
//==============================================================================
bool WorkspacePresetUI::hasUnsavedChanges() const
{
    return unsavedChanges;
}

void WorkspacePresetUI::markAsModified()
{
    unsavedChanges = true;
    updateInfoLabel();
}

void WorkspacePresetUI::markAsSaved()
{
    unsavedChanges = false;
    updateInfoLabel();
}

//==============================================================================
// Callbacks
//==============================================================================
void WorkspacePresetUI::setPresetSelectedCallback(PresetSelectedCallback callback)
{
    presetSelectedCallback = callback;
}

void WorkspacePresetUI::setPresetSavedCallback(PresetSavedCallback callback)
{
    presetSavedCallback = callback;
}

void WorkspacePresetUI::setPresetLoadedCallback(PresetLoadedCallback callback)
{
    presetLoadedCallback = callback;
}

void WorkspacePresetUI::setPresetDeletedCallback(PresetDeletedCallback callback)
{
    presetDeletedCallback = callback;
}

//==============================================================================
// JUCE Component Overrides
//==============================================================================
void WorkspacePresetUI::paint(juce::Graphics& g)
{
    const auto bounds = getLocalBounds().toFloat();

    // Hintergrund
    if (extendedThemeSystem)
    {
        const auto bgColour = extendedThemeSystem->getColour(ThemeColourRole::Background);
        g.fillAll(bgColour);
    }
    else
    {
        g.fillAll(juce::Colour(0xFF1E1E1E));
    }

    // Header Hintergrund
    juce::ColourGradient headerGradient(juce::Colour(0xFF2A2A2A),
                                       bounds.getTopLeft(),
                                       juce::Colour(0xFF222222),
                                       bounds.getTopLeft().withY(bounds.getHeight() * 0.15f),
                                       false);
    g.setGradientFill(headerGradient);
    g.fillRect(bounds.getX(), bounds.getY(), bounds.getWidth(), bounds.getHeight() * 0.15f);

    // Header Trennlinie
    g.setColour(juce::Colours::white.withAlpha(0.1f));
    g.drawLine(bounds.getX(), bounds.getHeight() * 0.15f,
              bounds.getRight(), bounds.getHeight() * 0.15f, 1.0f);
}

void WorkspacePresetUI::resized()
{
    const auto bounds = getLocalBounds();
    const int headerHeight = bounds.getHeight() / 8;
    const int toolbarHeight = bounds.getHeight() / 10;
    const int infoHeight = 30;

    // Header
    if (titleLabel)
        titleLabel->setBounds(16, 16, bounds.getWidth() - 32, headerHeight - 32);

    // Toolbar
    const int toolbarY = headerHeight;
    const int buttonWidth = 80;
    const int buttonMargin = 8;
    int buttonX = 16;

    if (newPresetButton)
    {
        newPresetButton->setBounds(buttonX, toolbarY + 8, buttonWidth, toolbarHeight - 16);
        buttonX += buttonWidth + buttonMargin;
    }

    if (saveButton)
    {
        saveButton->setBounds(buttonX, toolbarY + 8, buttonWidth, toolbarHeight - 16);
        buttonX += buttonWidth + buttonMargin;
    }

    if (loadButton)
    {
        loadButton->setBounds(buttonX, toolbarY + 8, buttonWidth, toolbarHeight - 16);
        buttonX += buttonWidth + buttonMargin;
    }

    if (deleteButton)
    {
        deleteButton->setBounds(buttonX, toolbarY + 8, buttonWidth, toolbarHeight - 16);
        buttonX += buttonWidth + buttonMargin;
    }

    if (importButton)
    {
        importButton->setBounds(bounds.getWidth() - (buttonWidth + 16) * 3, toolbarY + 8, buttonWidth, toolbarHeight - 16);
    }

    if (exportButton)
    {
        exportButton->setBounds(bounds.getWidth() - (buttonWidth + 16) * 2, toolbarY + 8, buttonWidth, toolbarHeight - 16);
    }

    if (refreshButton)
    {
        refreshButton->setBounds(bounds.getWidth() - buttonWidth - 16, toolbarY + 8, buttonWidth, toolbarHeight - 16);
    }

    // Suchfeld und Kategorie
    const int searchWidth = 200;
    const int categoryWidth = 150;
    const int filterY = toolbarY + toolbarHeight + 8;

    if (searchEditor)
        searchEditor->setBounds(16, filterY, searchWidth, 30);

    if (categoryComboBox)
        categoryComboBox->setBounds(16 + searchWidth + 16, filterY, categoryWidth, 30);

    // Preset-Listbox
    const int listY = filterY + 40;
    const int listHeight = bounds.getHeight() - listY - infoHeight - 16;

    if (presetListBox)
        presetListBox->setBounds(16, listY, bounds.getWidth() - 32, listHeight);

    // Info-Label
    if (infoLabel)
        infoLabel->setBounds(16, bounds.getHeight() - infoHeight - 8, bounds.getWidth() - 32, infoHeight);

    // Unsaved Changes Label
    if (unsavedChangesLabel)
        unsavedChangesLabel->setBounds(bounds.getWidth() - 200, bounds.getHeight() - infoHeight - 8, 180, infoHeight);
}

void WorkspacePresetUI::visibilityChanged()
{
    if (isVisible())
        loadPresets();
}

//==============================================================================
// Private Methods
//==============================================================================
void WorkspacePresetUI::timerCallback()
{
    // Timer für periodische UI-Updates
}

void WorkspacePresetUI::setupUI()
{
    // Title Label
    titleLabel = std::make_unique<juce::Label>("titleLabel", "Workspace Presets");
    titleLabel->setFont(juce::Font(20.0f, juce::Font::bold));
    titleLabel->setColour(juce::Label::textColourId, juce::Colours::white);
    addAndMakeVisible(*titleLabel);

    // Buttons
    newPresetButton = std::make_unique<juce::TextButton>("New");
    newPresetButton->onClick = [this]() { onNewPresetButtonClicked(); };
    addAndMakeVisible(*newPresetButton);

    saveButton = std::make_unique<juce::TextButton>("Save");
    saveButton->onClick = [this]() { onSaveButtonClicked(); };
    addAndMakeVisible(*saveButton);

    loadButton = std::make_unique<juce::TextButton>("Load");
    loadButton->onClick = [this]() { onLoadButtonClicked(); };
    addAndMakeVisible(*loadButton);

    deleteButton = std::make_unique<juce::TextButton>("Delete");
    deleteButton->onClick = [this]() { onDeleteButtonClicked(); };
    addAndMakeVisible(*deleteButton);

    importButton = std::make_unique<juce::TextButton>("Import");
    importButton->onClick = [this]() { onImportButtonClicked(); };
    addAndMakeVisible(*importButton);

    exportButton = std::make_unique<juce::TextButton>("Export");
    exportButton->onClick = [this]() { onExportButtonClicked(); };
    addAndMakeVisible(*exportButton);

    refreshButton = std::make_unique<juce::TextButton>("Refresh");
    refreshButton->onClick = [this]() { onRefreshButtonClicked(); };
    addAndMakeVisible(*refreshButton);

    // Suchfeld
    searchEditor = std::make_unique<juce::TextEditor>("searchEditor");
    searchEditor->setText("Search presets...", juce::dontSendNotification);
    searchEditor->setColour(juce::TextEditor::backgroundColourId, juce::Colour(0xFF2A2A2A));
    searchEditor->setColour(juce::TextEditor::textColourId, juce::Colours::white);
    searchEditor->setColour(juce::TextEditor::outlineColourId, juce::Colours::transparentBlack);
    addAndMakeVisible(*searchEditor);

    // Kategorie-ComboBox
    categoryComboBox = std::make_unique<juce::ComboBox>("categoryComboBox");
    categoryComboBox->addItem("All Categories", 1);
    categoryComboBox->addItem("Synthesis", 2);
    categoryComboBox->addItem("Mixing", 3);
    categoryComboBox->addItem("Performance", 4);
    categoryComboBox->addItem("Production", 5);
    categoryComboBox->addItem("Custom", 6);
    categoryComboBox->setSelectedItemIndex(0);
    categoryComboBox->onChange = [this]() { onCategoryChanged(); };
    addAndMakeVisible(*categoryComboBox);

    // Preset Listbox Model und Listbox
    presetListboxModel = std::make_unique<PresetListboxModel>();
    presetListboxModel->setPresetSelectedCallback([this](const WorkspacePreset& preset)
    {
        onPresetSelected(preset);
    });
    presetListboxModel->setPresetDeleteCallback([this](const WorkspacePreset& preset)
    {
        onPresetDeleteRequested(preset);
    });

    presetListBox = std::make_unique<juce::ListBox>("presetListBox", presetListboxModel.get());
    presetListBox->setColour(juce::ListBox::backgroundColourId, juce::Colours::transparentBlack);
    presetListBox->setRowHeight(80);
    addAndMakeVisible(*presetListBox);

    // Info-Label
    infoLabel = std::make_unique<juce::Label>("infoLabel", "");
    infoLabel->setFont(juce::Font(12.0f));
    infoLabel->setColour(juce::Label::textColourId, juce::Colours::lightgrey);
    addAndMakeVisible(*infoLabel);

    // Unsaved Changes Label
    unsavedChangesLabel = std::make_unique<juce::Label>("unsavedChangesLabel", "");
    unsavedChangesLabel->setFont(juce::Font(12.0f, juce::Font::bold));
    unsavedChangesLabel->setColour(juce::Label::textColourId, juce::Colour(0xFFFFA500));
    unsavedChangesLabel->setJustificationType(juce::Justification::centredRight);
    unsavedChangesLabel->setVisible(false);
    addAndMakeVisible(*unsavedChangesLabel);
}

void WorkspacePresetUI::updatePresetList()
{
    if (presetListboxModel)
        presetListboxModel->setPresets(filteredPresets);

    if (presetListBox)
        presetListBox->updateContent();

    updateButtons();
    updateInfoLabel();
}

void WorkspacePresetUI::filterPresets()
{
    filteredPresets.clear();

    for (const auto& preset : allPresets)
    {
        bool matchesSearch = true;

        // Suchfilter (Name und Description)
        if (!currentSearchText.isEmpty())
        {
            const auto nameLower = preset.name.toLowerCase();
            const auto descLower = preset.description.toLowerCase();
            matchesSearch = nameLower.contains(currentSearchText) ||
                           descLower.contains(currentSearchText);
        }

        if (matchesSearch)
            filteredPresets.push_back(preset);
    }

    updatePresetList();
}

void WorkspacePresetUI::updateButtons()
{
    const bool hasSelection = !selectedPresetName.isEmpty();

    if (loadButton)
        loadButton->setEnabled(hasSelection);

    if (deleteButton)
    {
        const auto preset = presetListboxModel->getPreset(selectedPresetName);
        const bool canDelete = preset.has_value();
        deleteButton->setEnabled(canDelete);
    }

    if (saveButton)
        saveButton->setEnabled(true); // Kann immer speichern
}

void WorkspacePresetUI::updateInfoLabel()
{
    juce::String infoText;

    if (!selectedPresetName.isEmpty())
    {
        const auto preset = presetListboxModel->getPreset(selectedPresetName);
        if (preset.has_value())
        {
            infoText = preset->name;
            if (!preset->description.isEmpty())
                infoText += " - " + preset->description;
        }
    }
    else
    {
        infoText = "Select a preset to view details";
    }

    if (infoLabel)
        infoLabel->setText(infoText, juce::dontSendNotification);

    if (unsavedChangesLabel)
        unsavedChangesLabel->setVisible(unsavedChanges);
}

void WorkspacePresetUI::onPresetSelected(const WorkspacePreset& preset)
{
    selectedPresetName = preset.name;

    if (presetSelectedCallback)
        presetSelectedCallback(preset);

    updateButtons();
    updateInfoLabel();
}

void WorkspacePresetUI::onPresetDoubleClicked(const WorkspacePreset& preset)
{
    // Preset direkt laden
    loadPreset(preset.name);
}

void WorkspacePresetUI::onPresetDeleteRequested(const WorkspacePreset& preset)
{
    deletePreset(preset.name);
}

void WorkspacePresetUI::onSaveButtonClicked()
{
    showSavePresetDialog();
}

void WorkspacePresetUI::onLoadButtonClicked()
{
    if (!selectedPresetName.isEmpty())
        loadPreset(selectedPresetName);
}

void WorkspacePresetUI::onDeleteButtonClicked()
{
    if (!selectedPresetName.isEmpty())
        deletePreset(selectedPresetName);
}

void WorkspacePresetUI::onImportButtonClicked()
{
    importPresets();
}

void WorkspacePresetUI::onExportButtonClicked()
{
    exportPresets();
}

void WorkspacePresetUI::onNewPresetButtonClicked()
{
    showSavePresetDialog();
}

void WorkspacePresetUI::onRefreshButtonClicked()
{
    loadPresets();
}

void WorkspacePresetUI::onSearchTextChanged()
{
    if (searchEditor)
        setSearchText(searchEditor->getText());
}

void WorkspacePresetUI::onCategoryChanged()
{
    if (categoryComboBox)
    {
        const int selectedIndex = categoryComboBox->getSelectedItemIndex();
        if (selectedIndex > 0)
        {
            const auto category = categoryComboBox->getItemText(selectedIndex);
            setCategoryFilter(category);
        }
        else
        {
            showAllPresets();
        }
    }
}

void WorkspacePresetUI::showSavePresetDialog()
{
    auto* dialog = new juce::AlertWindow("Save Workspace Preset", "Enter a name:", juce::AlertWindow::NoIcon);
    dialog->addTextEditor("presetName", "", "Preset Name:");
    dialog->addTextEditor("presetDescription", "", "Description:");
    dialog->addButton("Save", 1, juce::KeyPress(juce::KeyPress::returnKey));
    dialog->addButton("Cancel", 0, juce::KeyPress(juce::KeyPress::escapeKey));
    dialog->enterModalState(true, juce::ModalCallbackFunction::create([this, dialog](int result) {
        if (result == 1) {
            const auto name = dialog->getTextEditorContents("presetName");
            const auto desc = dialog->getTextEditorContents("presetDescription");
            if (!name.isEmpty()) saveCurrentLayout(name, desc, "Custom");
        }
    }));
}

void WorkspacePresetUI::showDeleteConfirmationDialog(const juce::String& presetName)
{
    const juce::String message = "Are you sure you want to delete the preset \"" + presetName + "\"?";

    auto* dialog = new juce::AlertWindow("Delete Preset", message, juce::AlertWindow::WarningIcon);
    dialog->addButton("Yes", 1);
    dialog->addButton("No", 0);
    dialog->enterModalState(true, juce::ModalCallbackFunction::create([this, presetName](int result) {
        if (result == 1) {
            presetListboxModel->removePreset(presetName);

            for (auto it = allPresets.begin(); it != allPresets.end(); ++it)
            {
                if (it->name == presetName)
                {
                    allPresets.erase(it);
                    break;
                }
            }

            filterPresets();

            if (presetDeletedCallback)
                presetDeletedCallback(presetName);

            if (selectedPresetName == presetName)
                selectedPresetName.clear();
        }
    }));
}

juce::String WorkspacePresetUI::serializeLayout() const
{
    // TODO: Layout serialisieren
    juce::ValueTree layoutTree("WorkspaceLayout");

    if (workspaceManager)
    {
        // Layout-Daten aus WorkspaceManager holen
        // ...
    }

    return layoutTree.toXmlString();
}

bool WorkspacePresetUI::deserializeLayout(const juce::String& layoutData)
{
    // TODO: Layout deserialisieren
    if (workspaceManager && !layoutData.isEmpty())
    {
        auto xml = juce::parseXML(layoutData);
        if (xml != nullptr)
        {
            juce::ValueTree layoutTree = juce::ValueTree::fromXml(*xml);

            // Layout an WorkspaceManager übergeben
            // ...
            return true;
        }
    }
    return false;
}

void WorkspacePresetUI::drawPresetPreview(juce::Graphics& g, const WorkspacePreset& preset)
{
    // TODO: Vorschau zeichnen
}
