#include <string>
#include <vector>
#include "../library/json.hpp"
#include "triggerDefs.hpp"

#pragma once

#ifndef TRIGGEREDITOR_HPP
#define TRIGGEREDITOR_HPP

struct ImVec2; // forward declaration (Dear ImGui), used by reference in method signatures
struct ImFont; // forward declaration (Dear ImGui)

// ---------------------------------------------------------------------------
// Visual (GUI) editing model
//
// A trigger file is a YAML document whose "trigger" list holds one or more
// trigger blocks. Each block has events, conditions and actions. The GUI
// editor edits those blocks through typed rows and regenerates the raw YAML
// text on every change (the YAML tab stays in sync in real time).
// ---------------------------------------------------------------------------

enum class GuiKind { Event, Condition, Action };

struct GuiEntry
{
    std::string type;  // type key, e.g. "unit-kill"
    json raw;          // original record object (preserves unknown fields)
    int group = 0;     // conditions: 0 = and, 1 = or ; actions: 0 = then, 1 = else
};

struct GuiTrigger
{
    std::vector<GuiEntry> events;
    std::vector<GuiEntry> conditions;
    std::vector<GuiEntry> actions;
};

// State of the add/edit popup (searchable type lookup + parameter editors).
struct GuiPopupState
{
    bool open = false;
    GuiKind kind = GuiKind::Event;
    bool editing = false;        // false = adding a new entry
    int triggerIndex = 0;        // block being edited
    int entryIndex = 0;          // index in its list (-1 when adding)
    int insertIndex = -1;        // adding: insert at this index (-1 = append at the end)
    char search[64] = "";        // lookup filter
    std::string selectedType;    // type key currently selected
    GuiEntry draft;              // working copy of the entry
    char unitNameBuf[256] = "";
    char textBuf[256] = "";
    int intValueBuf = 0;
    float floatValueBuf = 0.f;
    int groupBuf = 0;
    std::vector<std::string> stringsBuf;
    std::vector<int> intsBuf;
    std::vector<float> floatsBuf;
    std::vector<bool> boolsBuf;
    std::string error;
    float fixedBodyH = 0.f;      // height of everything above the params child
};

// ---------------------------------------------------------------------------
// Trigger Editor (realm-editor tool, launched by Grimsolf with --trigger-editor)
//
// ImGui tool that edits the trigger YAML files linked to a map
// (<gamePath>/data/maps/<map>.json -> <gamePath>/data/maps/<dir>/trigger/<map>.yaml,
// following the "script" field of the map's "trigger" array, e.g. meadows.yaml).
//
// Flow:
//   1. Map select  - pick one of the maps under <gamePath>/data/maps/.
//   2. Open/create - the editor resolves (or creates and links) the map's
//                    main trigger file and opens it.
//   3. Edit        - a YAML text editor with syntax highlighting, auto
//                    indentation, a live YAML validity check and structure
//                    warnings (unknown names get a "did you mean" hint), plus a
//                    sidebar that lists the files of the browsable folder
//                    selected there. The map's trigger folder comes first (the
//                    main file can reference others through the "script"
//                    entries of its own "trigger" array); the game's
//                    interaction and cutscene folders are listed as well,
//                    because their files are trigger documents too. New files
//                    can be created and non-main files deleted inside the
//                    map's trigger folder only.
//
// The whole UI is wrapped in a try/catch inside update() so an unexpected
// exception is reported through a popup instead of killing the process
// silently.
//
// update() returns true when the user asked to leave the editor (BACK button
// or the map-select stage), which the host application (TriggerApp) turns into
// closing the editor window so Grimsolf can restore its own window.
// ---------------------------------------------------------------------------
class TriggerEditor
{
    public:
        TriggerEditor(const std::string &gamePath, ImFont *fontNormal, ImFont *fontBig);
        ~TriggerEditor();

        // Renders the tool UI for one frame (Dear ImGui).
        // Returns true when the user asked to leave the editor (close the
        // realm-editor process). Never throws: exceptions from the internal
        // implementation are caught and reported through the log popup.
        bool update(float timer);

    private:
        struct LogLine
        {
            std::string text;
            unsigned int color; // packed IM_COL32
        };

        enum class Stage
        {
            MapSelect,   // pick which map to edit triggers for
            Editor       // trigger JSON editor for the selected map
        };

        std::string gamePath;       // game folder (data/maps, data/text, ...)
        ImFont *fontNormal = nullptr;
        ImFont *fontBig = nullptr;
        float imguiScale = 1.f;

        // UI scale baseline: the game's configured resolution width (read from
        // <gamePath>/data/options/options.json). The editor UI was designed
        // against it, so this keeps the old look regardless of the window size.
        float designWidth = 1920.f;

        // Language strings are loaded once from the game's configured language
        // file (<gamePath>/data/text/<misc-language>.json, "TRIGGER-EDITOR"
        // section); the field name is the fallback when the file is missing.
        mutable json languageFile;
        mutable bool languageLoaded = false;

        // ---- stage ----
        Stage stage = Stage::MapSelect;

        // ---- map selection ----
        std::vector<std::string> mapList;   // map refs relative to data/maps ("custom/meadows")
        std::string selectedMap;            // map ref of the current session
        std::string mapPath;                // "<gamePath>/data/maps/<ref>.json"

        // ---- trigger session (resolved once the map is chosen) ----
        std::string triggerFolderRef;       // folder ref relative to data/maps ("custom/trigger")
        std::string triggerFolderPath;      // "<gamePath>/data/maps/custom/trigger/"
        std::string mainTriggerFile;        // filename of the map-linked trigger ("meadows.yaml")

        // ---- browsable folders (sidebar) ----
        // The map's trigger folder is the first entry while a map session is
        // open. The game folders that hold trigger documents too (interactions
        // and cutscenes) follow, so the same visual editor can open them.
        struct BrowseSource
        {
            std::string label;              // language field with the button label
            std::string ref;                // folder relative to the game ("data/cutscenes")
            std::string folder;             // full folder path, with a trailing '/'
            bool editable = false;          // create/delete allowed (map trigger folder)
            std::vector<std::string> files; // ".yaml" files, relative and '/'-separated
        };

        std::vector<BrowseSource> sources;
        int sourceIndex = 0;                // folder currently listed in the sidebar
        int currentFileSource = -1;         // folder owning the open file (-1 = none)

        // ---- open file state ----
        std::string currentFile;            // path inside its folder ("meadows.yaml", "nibbos/adam.yaml")
        std::string currentFilePath;        // full path relative to the game folder
        std::string text;                   // editor buffer (UTF-8)
        bool dirty = false;                 // unsaved changes?
        bool fileLoaded = false;

        // ---- JSON text editor widget state ----
        int cursorPos = 0;                  // byte offset in text
        int selectionStart = -1;            // byte offset, -1 = no selection
        float scrollX = 0.f;                // kept in sync with the child scrollbars
        float scrollY = 0.f;
        bool editorFocused = false;
        float caretTimer = 0.f;
        bool caretVisible = true;
        std::vector<std::string> undoStack;
        std::vector<std::string> redoStack;

        // ---- live JSON validation ----
        bool yamlValid = true;
        std::string parseError;
        int parseErrorLine = 0;
        int parseErrorCol = 0;
        std::vector<std::string> syntaxWarnings; // structure / name warnings of the open file

        // ---- visual (GUI) mode ----
        // Active editor tab: GUI edits the trigger blocks, Misc edits the
        // other top-level fields (item-drop, environments-drop, ...), JSON is
        // a read-only viewer of the current file text.
        enum class GuiTab { Visual, Json, Misc };
        GuiTab guiTab = GuiTab::Visual;
        std::vector<GuiTrigger> guiTriggers; // trigger blocks of the open file
        json guiRoot;                       // whole document (keeps item-drop, etc.)
        int guiTriggerIndex = 0;            // selected block in the GUI
        bool guiConvertible = true;         // last JSON -> GUI conversion succeeded
        std::string guiConvertError;
        GuiPopupState guiPopup;
        bool guiDeleteBlockOpen = false;    // confirm removing the selected block
        bool guiHideMemos = false;          // hide the event/condition/action list panels
        bool guiHideEvents = false;         // hide the Events memo only
        bool guiHideConditions = false;     // hide the Conditions memo only
        bool guiHideActions = false;        // hide the Actions memo only

        // ---- entry clipboard (Copy / Paste in the GUI lists) ----
        // One entry at a time, scoped to its group: an event is never pasted
        // into the condition or action list (and vice versa).
        bool guiClipboardValid = false;
        GuiKind guiClipboardKind = GuiKind::Event;
        GuiEntry guiClipboard;

        // ---- per-frame editor layout (computed in renderEditor) ----
        float editorCharW = 8.f;
        float editorLineH = 20.f;
        float editorViewW = 0.f;
        float editorViewH = 0.f;
        float editorGutterW = 40.f;
        int editorVisibleLines = 10;

        // Current viewport size in pixels (set in updateImpl; used to size
        // the registration popup and the GUI group lists relative to screen).
        float viewportW = 0.f;
        float viewportH = 0.f;

        // ---- dialogs ----
        bool createPopupOpen = false;
        char newFileName[64] = "";
        std::string createError;
        bool deletePopupOpen = false;
        std::string deleteTarget;
        bool discardPopupOpen = false;      // "unsaved changes" confirmation
        std::string discardAction;          // "file:<name>" / "map" / "back"
        std::string discardTarget;
        bool pendingLeave = false;          // user asked to leave the editor

        // ---- log / error popup ----
        std::vector<LogLine> logLines;
        bool logDirty = false;
        bool errorPopupOpen = false;
        std::vector<std::string> errorPopupLines;
        void showError(const std::string &text);

        // ---- internals ----
        std::string getLanguage(const std::string &field) const;
        void addLog(const std::string &text, unsigned int color);
        bool updateImpl();
        void renderErrorPopup();

        // map stage
        void refreshMapList();
        void selectMap(const std::string &ref);
        bool openMapSession(const std::string &ref);
        std::string ensureTriggerLink(json &mapJson, const std::string &mapRef);
        std::string resolveMapScript(const json &mapJson) const;
        void resetSession();

        // browsable folders
        void refreshSources();
        void refreshSourceFiles(int index);
        void switchSource(int index);
        void openFileInSource(int index, const std::string &filename);
        void openSourceSession(int index, const std::string &filename); // no map session
        bool sourceIsEditable(int index) const;

        // trigger file stage
        void openTriggerFile(const std::string &filename);
        void createTriggerFile(const std::string &name);
        void deleteTriggerFile(const std::string &filename);
        bool saveCurrentFile();

        // UI panels
        void renderMapSelect();
        void renderSidebar();
        void renderToolbar();
        void renderLog();
        void renderYamlBody();   // raw JSON editor content (inside the JSON tab)

        // visual (GUI) editor
        void renderGuiEditor();
        void renderGuiGroup(GuiKind kind, std::vector<GuiEntry> &entries);
        bool textToGui();                  // parse this->text into guiTriggers
        void guiToText();                  // regenerate this->text from guiTriggers
        std::string guiDescription(GuiKind kind, const std::string &type) const;
        std::string guiEntryValues(GuiKind kind, const GuiEntry &entry) const; // compact values summary
        const std::vector<TriggerTypeDef> &guiDefs(GuiKind kind) const;
        void openGuiPopup(GuiKind kind, int triggerIndex, int entryIndex, int insertIndex = -1);
        void renderGuiPopup();
        void commitGuiPopup();

        // Misc tab: generic editor for the non-trigger top-level fields of the
        // file (item-drop, item-drop-list, environments-drop, environments-
        // drop-hit, player-characters, config, exit, procedural-*, quest-*, ...).
        void renderGuiMiscEditor();
        bool guiMiscEditValue(json &value);   // edits one value; true = changed
        bool guiMiscEditArray(json &value);
        bool guiMiscEditObject(json &value);
        void guiMiscAddArrayElement(json &value);
        void guiSyncText();                // guiToText + validate + dirty + log
        void guiMoveEntry(std::vector<GuiEntry> &entries, int index, int direction);
        void guiDropEntry(std::vector<GuiEntry> &entries, int from, int to, bool insertBefore);
        void guiDeleteEntry(std::vector<GuiEntry> &entries, int index);
        void guiDuplicateEntry(std::vector<GuiEntry> &entries, int index);
        void guiCopyEntry(const GuiEntry &entry, GuiKind kind);
        void guiPasteEntry(std::vector<GuiEntry> &entries, GuiKind kind, int insertIndex = -1);
        std::vector<GuiEntry> &guiEntryList(GuiKind kind, GuiTrigger &trigger);

        // JSON editor widget
        void renderEditor();
        void renderEditorStatus();
        void validateYaml();
        void formatYaml();
        void checkTriggerSyntax(const json &root);  // fills syntaxWarnings
        void logSyntaxWarnings();
        void handleEditorKeys();
        void handleEditorMouse(ImVec2 base);
        void renderEditorText(ImVec2 base);
        void drawCaret(ImVec2 base);
        void ensureCaretVisible();

        // session / dirty-change handling
        void requestAction(const std::string &action, const std::string &target);
        void openTriggerFileGui();         // initialize GUI state after opening a file
        bool performAction(const std::string &action, const std::string &target);
        std::string defaultTriggerTemplate() const;

        // text helpers (byte-offset based)
        int countLines() const;
        int lineStartOffset(int line) const;
        int lineEndOffset(int line) const;   // start of the next line
        int lineOfOffset(int offset) const;
        int lineIndentWidth(int line) const;
        int utf8Columns(int offset) const;   // code points on the line before offset
        int utf8Advance(int offset) const;   // next code point boundary
        int utf8Back(int offset) const;      // previous code point boundary
        int visualToByte(int line, int column) const; // column (code points) -> byte offset
        void clampCursor();
        void snapshotUndo();
        std::string getSelectedText() const;

        // filesystem helpers
        json loadJsonFile(const std::string &path) const;
        bool saveTextFile(const std::string &path, const std::string &content) const;
        std::string readTextFile(const std::string &path) const;
};

#endif // TRIGGEREDITOR_HPP
