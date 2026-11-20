#include <string>
#include <vector>
#include "../library/json.hpp"
#include "triggerDefs.hpp"

#pragma once

#ifndef TRIGGEREDITOR_HPP
#define TRIGGEREDITOR_HPP

struct ImVec2;
struct ImFont;


enum class GuiKind { Event, Condition, Action };

struct GuiEntry
{
    std::string type;
    json raw;
    int group = 0;
    bool disabled = false;
    bool isComment = false;
    std::string comment;
};

struct GuiTrigger
{
    std::string name;
    std::vector<GuiEntry> events;
    std::vector<GuiEntry> conditions;
    std::vector<GuiEntry> actions;
};

struct GuiPopupState
{
    bool open = false;
    GuiKind kind = GuiKind::Event;
    bool editing = false;
    int triggerIndex = 0;
    int entryIndex = 0;
    int insertIndex = -1;
    char search[64] = "";
    std::string selectedType;
    GuiEntry draft;
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
    float fixedBodyH = 0.f;
};

class TriggerEditor
{
    public:
        TriggerEditor(const std::string &gamePath, ImFont *fontNormal, ImFont *fontBig);
        ~TriggerEditor();

        bool update(float timer);

    private:
        struct LogLine
        {
            std::string text;
            unsigned int color;
        };

        enum class Stage
        {
            MapSelect,
            Editor
        };

        std::string gamePath;
        ImFont *fontNormal = nullptr;
        ImFont *fontBig = nullptr;
        float imguiScale = 1.f;

        float designWidth = 1920.f;

        Stage stage = Stage::MapSelect;

        std::vector<std::string> mapList;
        std::string selectedMap;
        std::string mapPath;

        std::string triggerFolderRef;
        std::string triggerFolderPath;
        std::string mainTriggerFile;

        struct BrowseSource
        {
            std::string label;
            std::string ref;
            std::string folder;
            bool editable = false;
            std::vector<std::string> files;
        };

        std::vector<BrowseSource> sources;
        int sourceIndex = 0;
        int currentFileSource = -1;

        std::string currentFile;
        std::string currentFilePath;
        std::string text;
        bool dirty = false;
        bool fileLoaded = false;

        int cursorPos = 0;
        int selectionStart = -1;
        float scrollX = 0.f;
        float scrollY = 0.f;
        bool editorFocused = false;
        float caretTimer = 0.f;
        bool caretVisible = true;
        std::vector<std::string> undoStack;
        std::vector<std::string> redoStack;

        bool yamlValid = true;
        std::string parseError;
        int parseErrorLine = 0;
        int parseErrorCol = 0;
        std::vector<std::string> syntaxWarnings;

        enum class GuiTab { Visual, Json, Misc };
        GuiTab guiTab = GuiTab::Visual;
        std::vector<GuiTrigger> guiTriggers;
        json guiRoot;
        int guiTriggerIndex = 0;
        bool guiConvertible = true;
        std::string guiConvertError;
        GuiPopupState guiPopup;
        bool guiDeleteBlockOpen = false;
        bool guiNamePopupOpen = false;
        char guiNameBuf[256] = "";
        bool guiHideMemos = false;
        bool guiHideEvents = false;
        bool guiHideConditions = false;
        bool guiHideActions = false;

        bool guiCommentPopupOpen = false;
        GuiKind guiCommentKind = GuiKind::Event;
        int guiCommentTriggerIndex = 0;
        int guiCommentEntryIndex = -1;
        int guiCommentInsertIndex = -1;
        char guiCommentBuf[512] = "";

        bool guiClipboardValid = false;
        GuiKind guiClipboardKind = GuiKind::Event;
        GuiEntry guiClipboard;

        float editorCharW = 8.f;
        float editorLineH = 20.f;
        float editorViewW = 0.f;
        float editorViewH = 0.f;
        float editorGutterW = 40.f;
        int editorVisibleLines = 10;

        float viewportW = 0.f;
        float viewportH = 0.f;

        bool createPopupOpen = false;
        char newFileName[64] = "";
        std::string createError;
        bool deletePopupOpen = false;
        std::string deleteTarget;
        bool discardPopupOpen = false;
        std::string discardAction;
        std::string discardTarget;
        bool pendingLeave = false;

        std::vector<LogLine> logLines;
        bool logDirty = false;
        bool errorPopupOpen = false;
        std::vector<std::string> errorPopupLines;
        void showError(const std::string &text);

        void addLog(const std::string &text, unsigned int color);
        bool updateImpl();
        void renderErrorPopup();

        void refreshMapList();
        void selectMap(const std::string &ref);
        bool openMapSession(const std::string &ref);
        std::string ensureTriggerLink(json &mapJson, const std::string &mapRef);
        std::string resolveMapScript(const json &mapJson) const;
        void resetSession();

        void refreshSources();
        void refreshSourceFiles(int index);
        void switchSource(int index);
        void openFileInSource(int index, const std::string &filename);
        void openSourceSession(int index, const std::string &filename);
        bool sourceIsEditable(int index) const;

        void openTriggerFile(const std::string &filename);
        void createTriggerFile(const std::string &name);
        void deleteTriggerFile(const std::string &filename);
        bool saveCurrentFile();

        void renderMapSelect();
        void renderSidebar();
        void renderToolbar();
        void renderLog();
        void renderYamlBody();

        void renderGuiEditor();
        void renderGuiGroup(GuiKind kind, std::vector<GuiEntry> &entries);
        bool textToGui();
        void guiToText();
        void guiAnnotateFromText();
        std::string guiDescription(GuiKind kind, const std::string &type) const;
        std::string guiEntryValues(GuiKind kind, const GuiEntry &entry) const;
        const std::vector<TriggerTypeDef> &guiDefs(GuiKind kind) const;
        void openGuiPopup(GuiKind kind, int triggerIndex, int entryIndex, int insertIndex = -1);
        void renderGuiPopup();
        void commitGuiPopup();
        void guiOpenCommentPopup(GuiKind kind, int triggerIndex, int entryIndex, int insertIndex = -1);
        void renderGuiCommentPopup();
        void renderGuiNamePopup();
        void commitGuiCommentPopup();

        void renderGuiMiscEditor();
        bool guiMiscEditValue(json &value);
        bool guiMiscEditArray(json &value);
        bool guiMiscEditObject(json &value);
        void guiMiscAddArrayElement(json &value);
        void guiSyncText();
        void guiMoveEntry(std::vector<GuiEntry> &entries, int index, int direction);
        void guiDropEntry(std::vector<GuiEntry> &entries, int from, int to, bool insertBefore);
        void guiDeleteEntry(std::vector<GuiEntry> &entries, int index);
        void guiDuplicateEntry(std::vector<GuiEntry> &entries, int index);
        void guiCopyEntry(const GuiEntry &entry, GuiKind kind);
        void guiPasteEntry(std::vector<GuiEntry> &entries, GuiKind kind, int insertIndex = -1);
        std::vector<GuiEntry> &guiEntryList(GuiKind kind, GuiTrigger &trigger);

        void renderEditor();
        void renderEditorStatus();
        void validateYaml();
        void formatYaml();
        void checkTriggerSyntax(const json &root);
        void logSyntaxWarnings();
        void handleEditorKeys();
        void handleEditorMouse(ImVec2 base);
        void renderEditorText(ImVec2 base);
        void drawCaret(ImVec2 base);
        void ensureCaretVisible();

        void requestAction(const std::string &action, const std::string &target);
        void openTriggerFileGui();
        bool performAction(const std::string &action, const std::string &target);
        std::string defaultTriggerTemplate() const;

        int countLines() const;
        int lineStartOffset(int line) const;
        int lineEndOffset(int line) const;
        int lineOfOffset(int offset) const;
        int lineIndentWidth(int line) const;
        int utf8Columns(int offset) const;
        int utf8Advance(int offset) const;
        int utf8Back(int offset) const;
        int visualToByte(int line, int column) const;
        void clampCursor();
        void snapshotUndo();
        std::string getSelectedText() const;

        json loadJsonFile(const std::string &path) const;
        bool saveTextFile(const std::string &path, const std::string &content) const;
        std::string readTextFile(const std::string &path) const;
};

#endif
