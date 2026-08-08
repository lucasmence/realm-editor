#include "triggerEditor.hpp"
#include "../external/imgui/imgui.h"
#include "../external/imgui/imgui-SFML.h"

#include <fstream>
#include <sstream>
#include <functional>
#include <cstring>
#include <cstdlib>
#include <cctype>
#include <cmath>
#include <algorithm>
#include <boost/filesystem.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/lexical_cast.hpp>

// Number formatting helper (replaces Grimsolf's toStr).
template <typename T> static std::string toStr(const T &value)
{
    return boost::lexical_cast<std::string>(value);
}

// ---------------------------------------------------------------------------
// JSON syntax colors (monokai-ish)
// ---------------------------------------------------------------------------
static const ImU32 COLOR_KEY         = IM_COL32(166, 226,  46, 255); // green
static const ImU32 COLOR_STRING      = IM_COL32(230, 219, 116, 255); // yellow
static const ImU32 COLOR_NUMBER      = IM_COL32(174, 129, 255, 255); // purple
static const ImU32 COLOR_BOOLEAN     = IM_COL32(102, 217, 239, 255); // cyan
static const ImU32 COLOR_PUNCT       = IM_COL32(215, 215, 215, 255); // light gray
static const ImU32 COLOR_BROKEN      = IM_COL32(249,  38, 114, 255); // red (invalid construct)
static const ImU32 COLOR_LINE_NUMBER = IM_COL32(110, 110, 122, 255);
static const ImU32 COLOR_SELECTION   = IM_COL32( 38,  79, 120, 110);
static const ImU32 COLOR_CARET       = IM_COL32(235, 235, 235, 255);

// Indentation unit used by auto-indent and the Format button (matches the
// 4-space dump used everywhere in the project's JSON files).
static const int INDENT = 4;

// Undo/redo history limit (per open file).
static const int HISTORY_LIMIT = 200;

// The Trigger Editor renders dense content (JSON text and trigger block
// lists), so its font is ~40% smaller than the regular menu font.
static const float GUI_FONT_SCALE = 0.6f;

// The Add/Edit registration popup covers ~60% of the screen (with a floor so
// it stays usable on small viewports); the type lookup list takes ~38% of the
// popup height, the rest goes to the parameter editors.
static const float GUI_POPUP_SIZE_FRACTION = 0.6f;
static const float GUI_POPUP_MIN_WIDTH = 640.f;
static const float GUI_POPUP_MIN_HEIGHT = 480.f;
static const float GUI_TYPE_LIST_HEIGHT_FRACTION = 0.38f;

// Minimum height of each group list in the visual editor; it also grows with
// the viewport (24% of the screen height).
static const float GUI_GROUP_MIN_HEIGHT = 170.f;
static const float GUI_GROUP_VIEWPORT_FRACTION = 0.24f;

// ---------------------------------------------------------------------------
// JSON tokenizer (used by the syntax highlighter)
// ---------------------------------------------------------------------------
enum class JsonTokenType
{
    Key,
    String,
    Number,
    Boolean,
    Null,
    Punctuation,
    Other
};

struct JsonToken
{
    int start;
    int end;
    JsonTokenType type;
};

static bool isIdentChar(char c)
{
    return std::isalnum((unsigned char)c) != 0 || c == '_' || c == '-' || c == '.';
}

static std::vector<JsonToken> tokenizeJson(const std::string &text)
{
    std::vector<JsonToken> tokens;
    const int n = (int)text.size();
    int i = 0;

    while (i < n)
    {
        char c = text[i];

        if (c == '"')
        {
            int start = i;
            ++i;
            bool closed = false;
            while (i < n)
            {
                if (text[i] == '\\')
                {
                    i += 2;
                    continue;
                }
                if (text[i] == '"')
                {
                    ++i;
                    closed = true;
                    break;
                }
                ++i;
            }

            JsonTokenType type = closed ? JsonTokenType::String : JsonTokenType::Other;

            // A string followed by ':' (skipping whitespace) is an object key.
            if (closed)
            {
                int j = i;
                while (j < n && (text[j] == ' ' || text[j] == '\t'))
                    ++j;
                if (j < n && text[j] == ':')
                    type = JsonTokenType::Key;
            }

            tokens.push_back(JsonToken{ start, i, type });
        }
        else if (c == '-' || (c >= '0' && c <= '9'))
        {
            int start = i;
            ++i;
            while (i < n)
            {
                char d = text[i];
                if ((d >= '0' && d <= '9') || d == '.' || d == 'e' || d == 'E' || d == '+' || d == '-')
                    ++i;
                else
                    break;
            }
            tokens.push_back(JsonToken{ start, i, JsonTokenType::Number });
        }
        else if (text.compare(i, 4, "true") == 0 && (i + 4 >= n || !isIdentChar(text[i + 4])))
        {
            tokens.push_back(JsonToken{ i, i + 4, JsonTokenType::Boolean });
            i += 4;
        }
        else if (text.compare(i, 5, "false") == 0 && (i + 5 >= n || !isIdentChar(text[i + 5])))
        {
            tokens.push_back(JsonToken{ i, i + 5, JsonTokenType::Boolean });
            i += 5;
        }
        else if (text.compare(i, 4, "null") == 0 && (i + 4 >= n || !isIdentChar(text[i + 4])))
        {
            tokens.push_back(JsonToken{ i, i + 4, JsonTokenType::Null });
            i += 4;
        }
        else if (c == '{' || c == '}' || c == '[' || c == ']' || c == ':' || c == ',')
        {
            tokens.push_back(JsonToken{ i, i + 1, JsonTokenType::Punctuation });
            ++i;
        }
        else
        {
            ++i; // whitespace and stray characters produce no token
        }
    }

    return tokens;
}

static ImU32 tokenColor(JsonTokenType type)
{
    switch (type)
    {
        case JsonTokenType::Key:         return COLOR_KEY;
        case JsonTokenType::String:      return COLOR_STRING;
        case JsonTokenType::Number:      return COLOR_NUMBER;
        case JsonTokenType::Boolean:     return COLOR_BOOLEAN;
        case JsonTokenType::Null:        return COLOR_BOOLEAN;
        case JsonTokenType::Punctuation: return COLOR_PUNCT;
        default:                         return COLOR_BROKEN;
    }
}

// ---------------------------------------------------------------------------
// UTF-8 helpers (the editor buffer is UTF-8; cursor positions are byte
// offsets, display columns are counted in code points)
// ---------------------------------------------------------------------------
// Word boundaries for Ctrl+Left/Right navigation.
static int wordLeft(const std::string &text, int offset)
{
    int i = offset;
    while (i > 0 && (text[i - 1] == ' ' || text[i - 1] == '\t'))
        --i;
    while (i > 0 && isIdentChar(text[i - 1]))
        --i;
    return i;
}

static int wordRight(const std::string &text, int offset)
{
    int n = (int)text.size();
    int i = offset;
    while (i < n && (text[i] == ' ' || text[i] == '\t'))
        ++i;
    while (i < n && isIdentChar(text[i]))
        ++i;
    return i;
}

// ---------------------------------------------------------------------------
// TriggerEditor
// ---------------------------------------------------------------------------

TriggerEditor::TriggerEditor(const std::string &gamePath, ImFont *fontNormal, ImFont *fontBig)
{
    this->gamePath = gamePath;
    this->fontNormal = fontNormal;
    this->fontBig = fontBig;

    // UI scale baseline: the game's configured resolution width (Grimsolf used
    // to force 1920x1080 and scale the editor by the configured resolution, so
    // a 1280x720 config ran at 1.5x). Read it here so the UI keeps the old
    // look regardless of the realm-editor window size; 1920 is the fallback.
    json options = this->loadJsonFile(gamePath + "/data/options/options.json");
    if (options.is_object())
        this->designWidth = (float)options.value("video-resolution-width", 1920);
    if (this->designWidth <= 0.f)
        this->designWidth = 1920.f;

    this->refreshMapList();
}

TriggerEditor::~TriggerEditor()
{
    this->logLines.clear();
    this->mapList.clear();
    this->triggerFileList.clear();
    this->undoStack.clear();
    this->redoStack.clear();
}

std::string TriggerEditor::getLanguage(const std::string &field) const
{
    if (!this->languageLoaded)
    {
        // Load the game's configured language file once (the realm-editor has
        // no Language class of its own). The field name is the fallback when
        // the file or the "TRIGGER-EDITOR" section cannot be read.
        this->languageLoaded = true;
        json options = this->loadJsonFile(this->gamePath + "/data/options/options.json");
        std::string language = "English";
        if (options.is_object())
            language = options.value("misc-language", "English");
        this->languageFile = this->loadJsonFile(this->gamePath + "/data/text/" + language + ".json");
    }

    if (!this->languageFile.is_object())
        return field;
    return this->languageFile["TRIGGER-EDITOR"].value(field, field);
}

void TriggerEditor::addLog(const std::string &text, unsigned int color)
{
    this->logLines.push_back(LogLine{ text, color });
    if (this->logLines.size() > 500)
        this->logLines.erase(this->logLines.begin());
    this->logDirty = true;
}

// Logs an error and opens the modal error popup. Used instead of letting an
// exception escape (the main loop catches everything and closes the game
// silently, which looks like a crash with no message).
void TriggerEditor::showError(const std::string &text)
{
    this->addLog(text, 0xFF6060FF);
    this->errorPopupLines.push_back(text);
    if (this->errorPopupLines.size() > 20)
        this->errorPopupLines.erase(this->errorPopupLines.begin());
    this->errorPopupOpen = true;
}

// ---------------------------------------------------------------------------
// Filesystem helpers
// ---------------------------------------------------------------------------

json TriggerEditor::loadJsonFile(const std::string &path) const
{
    // Read directly with nlohmann so the editor never pollutes the game's
    // global JSON load-error list (which would open a console dialog).
    if (!boost::filesystem::exists(path))
        return json();

    std::ifstream stream(path);
    if (!stream)
        return json();

    try
    {
        json data;
        stream >> data;
        return data;
    }
    catch (...)
    {
        return json();
    }
}

std::string TriggerEditor::readTextFile(const std::string &path) const
{
    std::ifstream stream(path, std::ios::binary);
    if (!stream)
        return "";

    std::ostringstream buffer;
    buffer << stream.rdbuf();
    return buffer.str();
}

bool TriggerEditor::saveTextFile(const std::string &path, const std::string &content) const
{
    boost::filesystem::path parent = boost::filesystem::path(path).parent_path();
    if (!parent.empty())
        boost::filesystem::create_directories(parent);

    std::ofstream stream(path, std::ios::trunc);
    if (!stream)
        return false;

    stream << content;
    return (bool)stream;
}

// ---------------------------------------------------------------------------
// Map stage
// ---------------------------------------------------------------------------

void TriggerEditor::refreshMapList()
{
    this->mapList.clear();

    try
    {
        boost::filesystem::path base = boost::filesystem::path(this->gamePath) / "data" / "maps";
        if (!boost::filesystem::is_directory(base))
            return;

        for (boost::filesystem::recursive_directory_iterator it(base), end; it != end; ++it)
        {
            if (!boost::filesystem::is_regular_file(it->status()))
                continue;
            if (it->path().extension() != ".json")
                continue;

            std::string rel = it->path().lexically_relative(base).string();
            boost::replace_all(rel, "\\", "/");

            // Trigger script files live in "trigger/" subfolders: they are the
            // content being edited, not maps to pick.
            if (rel.find("/trigger/") != std::string::npos || rel.rfind("trigger/", 0) == 0)
                continue;

            if (rel.size() > 5)
                rel = rel.substr(0, rel.size() - 5); // strip ".json"

            this->mapList.push_back(rel);
        }
    }
    catch (...)
    {
        // A failed scan must never close the game: keep the list as is.
    }

    std::sort(this->mapList.begin(), this->mapList.end());
}

void TriggerEditor::selectMap(const std::string &ref)
{
    this->selectedMap = ref;
    this->mapPath = this->gamePath + "/data/maps/" + ref + ".json";
}

// Reads the map's "trigger" array and returns the first non-empty "script"
// reference found ("" when the map is not linked to a trigger file).
std::string TriggerEditor::resolveMapScript(const json &mapJson) const
{
    if (!mapJson.is_object() || !mapJson.contains("trigger") || !mapJson["trigger"].is_array())
        return "";

    for (const auto &entry : mapJson["trigger"])
        if (entry.is_object() && entry.contains("script") && entry["script"].is_string())
        {
            std::string script = entry["script"].get<std::string>();
            if (!script.empty())
                return script;
        }

    return "";
}

// Links the map to its main trigger file (data/maps/<dir>/trigger/<name>.json,
// where <dir>/<name> is the map ref) by writing the map's "trigger" array and
// saving the map. Returns the script ref written ("" on failure).
std::string TriggerEditor::ensureTriggerLink(json &mapJson, const std::string &mapRef)
{
    size_t slash = mapRef.find_last_of('/');
    std::string dir = (slash == std::string::npos) ? "" : mapRef.substr(0, slash);
    std::string name = (slash == std::string::npos) ? mapRef : mapRef.substr(slash + 1);

    std::string scriptRef = (dir.empty() ? "" : dir + "/") + "trigger/" + name;

    mapJson["trigger"] = json::array();
    mapJson["trigger"].push_back(json{ { "script", scriptRef } });

    if (!this->saveTextFile(this->mapPath, mapJson.dump(4) + "\n"))
        return "";

    return scriptRef;
}

bool TriggerEditor::openMapSession(const std::string &ref)
{
    this->selectedMap = ref;
    this->mapPath = this->gamePath + "/data/maps/" + ref + ".json";

    json mapJson = this->loadJsonFile(this->mapPath);
    if (!mapJson.is_object())
    {
        this->showError("[erro] " + this->getLanguage("FAILED-LOAD") + ": " + this->mapPath);
        return false;
    }

    // 1) Is the map already linked to a trigger script?
    std::string scriptRef = this->resolveMapScript(mapJson);
    bool created = false;

    // 2) No link: create one (and save the map).
    if (scriptRef.empty())
    {
        scriptRef = this->ensureTriggerLink(mapJson, ref);
        if (scriptRef.empty())
        {
            this->showError("[erro] " + this->getLanguage("FAILED-MAP-LINK") + ": " + this->mapPath);
            return false;
        }
        created = true;
    }

    // 3) Resolve the trigger folder and the main trigger file.
    size_t slash = scriptRef.find_last_of('/');
    this->triggerFolderRef = (slash == std::string::npos) ? "" : scriptRef.substr(0, slash);
    std::string mainStem = (slash == std::string::npos) ? scriptRef : scriptRef.substr(slash + 1);
    this->mainTriggerFile = mainStem + ".json";
    this->triggerFolderPath = this->gamePath + "/data/maps/"
                              + (this->triggerFolderRef.empty() ? "" : this->triggerFolderRef + "/");

    // 4) Make sure the main trigger file exists (create it with the template).
    std::string mainPath = this->triggerFolderPath + this->mainTriggerFile;
    if (!boost::filesystem::exists(mainPath))
    {
        if (!this->saveTextFile(mainPath, this->defaultTriggerTemplate()))
        {
            this->showError("[erro] " + this->getLanguage("FAILED-CREATE") + ": " + mainPath);
            return false;
        }
        created = true;
    }

    if (created)
        this->addLog("[info] " + this->getLanguage("MAP-LINKED") + ": " + scriptRef, 0x88CCFFFF);

    this->refreshTriggerFileList();
    this->openTriggerFile(this->mainTriggerFile);
    this->stage = Stage::Editor;
    this->addLog("[info] " + this->getLanguage("MAP") + ": " + ref, 0x88CCFFFF);
    return true;
}

void TriggerEditor::resetSession()
{
    this->stage = Stage::MapSelect;
    this->selectedMap.clear();
    this->mapPath.clear();
    this->triggerFolderRef.clear();
    this->triggerFolderPath.clear();
    this->mainTriggerFile.clear();
    this->triggerFileList.clear();
    this->currentFile.clear();
    this->currentFilePath.clear();
    this->text.clear();
    this->dirty = false;
    this->fileLoaded = false;
    this->cursorPos = 0;
    this->selectionStart = -1;
    this->undoStack.clear();
    this->redoStack.clear();
    this->parseError.clear();
    this->jsonValid = true;
    this->guiTab = GuiTab::Visual;
    this->guiTriggers.clear();
    this->guiRoot = json();
    this->guiTriggerIndex = 0;
    this->guiConvertible = true;
    this->guiConvertError.clear();
    this->guiPopup = GuiPopupState();
    this->guiDeleteBlockOpen = false;
}

// ---------------------------------------------------------------------------
// Trigger file stage
// ---------------------------------------------------------------------------

void TriggerEditor::refreshTriggerFileList()
{
    this->triggerFileList.clear();

    try
    {
        boost::filesystem::path dir = this->triggerFolderPath;
        if (!boost::filesystem::is_directory(dir))
            return;

        for (auto &entry : boost::filesystem::directory_iterator(dir))
            if (boost::filesystem::is_regular_file(entry.status()) && entry.path().extension() == ".json")
                this->triggerFileList.push_back(entry.path().filename().string());
    }
    catch (...)
    {
        // Keep the current list on a failed scan.
    }

    std::sort(this->triggerFileList.begin(), this->triggerFileList.end());
}

void TriggerEditor::openTriggerFile(const std::string &filename)
{
    std::string path = this->triggerFolderPath + filename;
    if (!boost::filesystem::exists(path))
    {
        this->showError("[erro] " + this->getLanguage("FAILED-LOAD") + ": " + path);
        return;
    }

    this->text = this->readTextFile(path);
    this->currentFile = filename;
    this->currentFilePath = path;
    this->fileLoaded = true;
    this->dirty = false;
    this->cursorPos = 0;
    this->selectionStart = -1;
    this->undoStack.clear();
    this->redoStack.clear();
    this->validateJson();
    this->openTriggerFileGui();
    this->addLog("[info] " + this->getLanguage("OPENED") + ": " + path, 0x88CCFFFF);
}

// Builds the GUI model from the just-opened file and selects the active tab.
// The visual editor is the default mode: when the file cannot be converted
// (invalid JSON, wrong structure, ...) the editor falls back to the JSON tab.
void TriggerEditor::openTriggerFileGui()
{
    this->guiTriggers.clear();
    this->guiTriggerIndex = 0;
    this->guiRoot = json::object();
    this->guiDeleteBlockOpen = false;
    this->guiPopup.open = false;
    this->guiPopup.error.clear();

    if (this->jsonToGui())
    {
        this->guiTab = GuiTab::Visual;
        this->guiConvertible = true;
    }
    else
    {
        // The JSON tab is a read-only viewer: it always shows the raw text.
        this->guiTab = GuiTab::Json;
        this->guiConvertible = false;
        this->addLog("[erro] " + this->guiConvertError, 0xFF6060FF);
    }
}

void TriggerEditor::createTriggerFile(const std::string &name)
{
    std::string trimmed = name;
    boost::trim(trimmed);

    // Accept "name" or "name.json", validate the stem.
    if (boost::iends_with(trimmed, ".json"))
        trimmed = trimmed.substr(0, trimmed.size() - 5);
    boost::trim(trimmed);

    bool valid = !trimmed.empty();
    for (char c : trimmed)
        if (!(std::isalnum((unsigned char)c) != 0 || c == '-' || c == '_'))
        {
            valid = false;
            break;
        }

    if (!valid)
    {
        this->createError = this->getLanguage("INVALID-NAME");
        return;
    }

    std::string filename = trimmed + ".json";
    std::string path = this->triggerFolderPath + filename;

    if (boost::filesystem::exists(path))
    {
        this->createError = this->getLanguage("NAME-EXISTS");
        return;
    }

    if (!this->saveTextFile(path, this->defaultTriggerTemplate()))
    {
        this->showError("[erro] " + this->getLanguage("FAILED-CREATE") + ": " + path);
        return;
    }

    this->refreshTriggerFileList();
    this->addLog("[ok] " + this->getLanguage("CREATED") + ": " + path, 0x55FF88FF);
    this->openTriggerFile(filename);
}

void TriggerEditor::deleteTriggerFile(const std::string &filename)
{
    // The map-linked trigger file is the entry point of the map's script
    // structure: it is never allowed to be deleted.
    if (filename == this->mainTriggerFile)
    {
        this->showError("[erro] " + this->getLanguage("DELETE-MAIN-FORBIDDEN"));
        return;
    }

    std::string path = this->triggerFolderPath + filename;
    if (!boost::filesystem::exists(path))
        return;

    if (!boost::filesystem::remove(path))
    {
        this->showError("[erro] " + this->getLanguage("FAILED-DELETE") + ": " + path);
        return;
    }

    if (filename == this->currentFile)
    {
        this->currentFile.clear();
        this->currentFilePath.clear();
        this->text.clear();
        this->fileLoaded = false;
        this->dirty = false;
        this->cursorPos = 0;
        this->selectionStart = -1;
        this->undoStack.clear();
        this->redoStack.clear();
    }

    this->refreshTriggerFileList();
    this->addLog("[ok] " + this->getLanguage("DELETED") + ": " + path, 0x55FF88FF);
}

bool TriggerEditor::saveCurrentFile()
{
    if (!this->fileLoaded)
        return false;

    std::string content = this->text;
    if (content.empty() || content.back() != '\n')
        content += "\n";

    if (!this->saveTextFile(this->currentFilePath, content))
    {
        this->showError("[erro] " + this->getLanguage("FAILED-SAVE") + ": " + this->currentFilePath);
        return false;
    }

    this->dirty = false;
    this->addLog("[ok] " + this->getLanguage("SAVED") + ": " + this->currentFilePath, 0x55FF88FF);
    return true;
}

std::string TriggerEditor::defaultTriggerTemplate() const
{
    return R"({
    "trigger":
    [
        {
            "events":
            [
            ],
            "conditions":
            [
            ],
            "then":
            [
            ]
        }
    ],
    "item-drop":
    [
    ],
    "item-drop-list":
    [
    ],
    "environments-drop":
    [
    ]
}
)";
}

// ---------------------------------------------------------------------------
// Dirty-change handling
// ---------------------------------------------------------------------------

// Requests an action that replaces/leaves the current buffer. When there are
// unsaved changes a confirmation popup is shown first; performAction() is
// called with the stored action after the user confirms.
void TriggerEditor::requestAction(const std::string &action, const std::string &target)
{
    if (this->dirty && this->fileLoaded && !this->discardPopupOpen)
    {
        this->discardAction = action;
        this->discardTarget = target;
        this->discardPopupOpen = true;
        return;
    }

    if (this->performAction(action, target))
        this->pendingLeave = true;
}

// Returns true when the action leaves the editor (back to the Studio menu).
bool TriggerEditor::performAction(const std::string &action, const std::string &target)
{
    if (action == "back")
    {
        this->resetSession();
        return true;
    }
    if (action == "map")
    {
        this->resetSession();
        return false;
    }
    if (action == "create")
    {
        this->createTriggerFile(target);
        return false;
    }
    if (action == "delete")
    {
        this->deleteTriggerFile(target);
        return false;
    }
    if (action == "file")
    {
        this->openTriggerFile(target);
        return false;
    }
    return false;
}

// ---------------------------------------------------------------------------
// JSON editor widget
// ---------------------------------------------------------------------------

void TriggerEditor::validateJson()
{
    this->parseError.clear();
    this->parseErrorLine = 0;
    this->parseErrorCol = 0;

    if (this->text.empty())
    {
        this->jsonValid = true;
        return;
    }

    try
    {
        json parsed = json::parse(this->text);
        (void)parsed;
        this->jsonValid = true;
    }
    catch (const json::parse_error &e)
    {
        this->jsonValid = false;
        this->parseError = e.what();

        std::string msg = e.what();
        size_t pos = msg.find("at line ");
        if (pos != std::string::npos)
        {
            pos += 8;
            this->parseErrorLine = std::atoi(msg.c_str() + pos);
            size_t comma = msg.find(", column ", pos);
            if (comma != std::string::npos)
                this->parseErrorCol = std::atoi(msg.c_str() + comma + 9);
        }
    }
    catch (...)
    {
        this->jsonValid = false;
        this->parseError = "parse error";
    }
}

void TriggerEditor::formatJson()
{
    this->validateJson();
    if (!this->jsonValid)
    {
        this->showError("[erro] " + this->getLanguage("INVALID") + ": " + this->parseError);
        return;
    }

    try
    {
        json parsed = json::parse(this->text);
        this->snapshotUndo();
        this->text = parsed.dump(4);
        this->cursorPos = 0;
        this->selectionStart = -1;
        this->dirty = true;
        this->validateJson();
        this->addLog("[info] " + this->getLanguage("FORMATTED"), 0x88CCFFFF);
    }
    catch (...)
    {
        this->showError("[erro] " + this->getLanguage("INVALID"));
    }
}

// --- text helpers (byte-offset based) ---

int TriggerEditor::countLines() const
{
    int lines = 1;
    for (char c : this->text)
        if (c == '\n')
            ++lines;
    return lines;
}

int TriggerEditor::lineStartOffset(int line) const
{
    if (line <= 0)
        return 0;

    int current = 0;
    for (int i = 0; i < (int)this->text.size(); ++i)
        if (this->text[i] == '\n')
        {
            ++current;
            if (current == line)
                return i + 1;
        }

    return (int)this->text.size();
}

int TriggerEditor::lineEndOffset(int line) const
{
    int ls = this->lineStartOffset(line);
    int i = ls;
    while (i < (int)this->text.size() && this->text[i] != '\n')
        ++i;
    return i;
}

int TriggerEditor::lineOfOffset(int offset) const
{
    int line = 0;
    int limit = std::min(offset, (int)this->text.size());
    for (int i = 0; i < limit; ++i)
        if (this->text[i] == '\n')
            ++line;
    return line;
}

int TriggerEditor::lineIndentWidth(int line) const
{
    int ls = this->lineStartOffset(line);
    int le = this->lineEndOffset(line);
    int width = 0;
    for (int i = ls; i < le; ++i)
    {
        if (this->text[i] == ' ')
            ++width;
        else if (this->text[i] == '\t')
            width += INDENT;
        else
            break;
    }
    return width;
}

// Number of code points between the start of the line holding `offset` and
// `offset` (display column, tabs count as one column).
int TriggerEditor::utf8Columns(int offset) const
{
    int line = this->lineOfOffset(offset);
    int ls = this->lineStartOffset(line);
    int columns = 0;
    for (int i = ls; i < offset; ++i)
        if ((this->text[i] & 0xC0) != 0x80)
            ++columns;
    return columns;
}

int TriggerEditor::utf8Advance(int offset) const
{
    if (offset >= (int)this->text.size())
        return offset;

    unsigned char c = (unsigned char)this->text[offset];
    int length = 1;
    if (c >= 0xF0)
        length = 4;
    else if (c >= 0xE0)
        length = 3;
    else if (c >= 0xC0)
        length = 2;

    return std::min(offset + length, (int)this->text.size());
}

int TriggerEditor::utf8Back(int offset) const
{
    if (offset <= 0)
        return 0;

    int i = offset - 1;
    while (i > 0 && ((unsigned char)this->text[i] & 0xC0) == 0x80)
        --i;
    return i;
}

int TriggerEditor::visualToByte(int line, int column) const
{
    int ls = this->lineStartOffset(line);
    int le = this->lineEndOffset(line);

    int offset = ls;
    int col = 0;
    while (offset < le && col < column)
    {
        offset = this->utf8Advance(offset);
        ++col;
    }
    return offset;
}

void TriggerEditor::clampCursor()
{
    this->cursorPos = std::max(0, std::min(this->cursorPos, (int)this->text.size()));
}

void TriggerEditor::snapshotUndo()
{
    this->undoStack.push_back(this->text);
    if (this->undoStack.size() > (size_t)HISTORY_LIMIT)
        this->undoStack.erase(this->undoStack.begin());
    this->redoStack.clear();
}

std::string TriggerEditor::getSelectedText() const
{
    if (this->selectionStart < 0)
        return "";

    int from = std::min(this->cursorPos, this->selectionStart);
    int to = std::max(this->cursorPos, this->selectionStart);
    return this->text.substr(from, to - from);
}

void TriggerEditor::handleEditorKeys()
{
    ImGuiIO &io = ImGui::GetIO();

    // Not focused: leave the typed-character queue alone when one of the
    // modal dialogs is open (their InputText consumes it); otherwise drain it
    // so characters typed while a plain button had focus do not replay later.
    if (!this->editorFocused)
    {
        if (!this->createPopupOpen && !this->deletePopupOpen
            && !this->discardPopupOpen && !this->errorPopupOpen)
            io.InputQueueCharacters.resize(0);
        return;
    }

    // The JSON tab is a read-only viewer (the GUI and Misc tabs are the
    // editing surfaces and keep the JSON text in sync): only navigation,
    // selection and copy are allowed - no hotkey mutates the buffer, so keys
    // pressed while reading never interfere with editing.
    io.InputQueueCharacters.resize(0);

    bool ctrl = io.KeyCtrl;
    bool shift = io.KeyShift;

    // --- selection helpers ---
    const auto moveCursor = [&](int newPos)
    {
        if (shift)
        {
            if (this->selectionStart < 0)
                this->selectionStart = this->cursorPos;
        }
        else
        {
            this->selectionStart = -1;
        }
        this->cursorPos = newPos;
        this->clampCursor();
        this->caretTimer = 0.f;
        this->caretVisible = true;
    };

    if (ctrl && ImGui::IsKeyPressed(ImGuiKey_A))
    {
        this->selectionStart = 0;
        this->cursorPos = (int)this->text.size();
        this->caretTimer = 0.f;
    }
    else if (ctrl && ImGui::IsKeyPressed(ImGuiKey_C))
    {
        ImGui::SetClipboardText(this->getSelectedText().c_str());
    }
    else if (ImGui::IsKeyPressed(ImGuiKey_LeftArrow, true))
    {
        int pos = ctrl ? wordLeft(this->text, this->cursorPos) : this->utf8Back(this->cursorPos);
        moveCursor(pos);
    }
    else if (ImGui::IsKeyPressed(ImGuiKey_RightArrow, true))
    {
        int pos = ctrl ? wordRight(this->text, this->cursorPos) : this->utf8Advance(this->cursorPos);
        moveCursor(pos);
    }
    else if (ImGui::IsKeyPressed(ImGuiKey_UpArrow, true))
    {
        int line = this->lineOfOffset(this->cursorPos);
        int column = this->utf8Columns(this->cursorPos);
        int newLine = std::max(0, line - 1);
        moveCursor(this->visualToByte(newLine, column));
    }
    else if (ImGui::IsKeyPressed(ImGuiKey_DownArrow, true))
    {
        int line = this->lineOfOffset(this->cursorPos);
        int column = this->utf8Columns(this->cursorPos);
        int newLine = std::min(this->countLines() - 1, line + 1);
        moveCursor(this->visualToByte(newLine, column));
    }
    else if (ImGui::IsKeyPressed(ImGuiKey_Home, true))
    {
        int line = this->lineOfOffset(this->cursorPos);
        int target = ctrl ? 0 : this->lineStartOffset(line);
        moveCursor(target);
    }
    else if (ImGui::IsKeyPressed(ImGuiKey_End, true))
    {
        int line = this->lineOfOffset(this->cursorPos);
        int target = ctrl ? (int)this->text.size() : this->lineEndOffset(line);
        moveCursor(target);
    }
    else if (ImGui::IsKeyPressed(ImGuiKey_PageUp, true))
    {
        int line = this->lineOfOffset(this->cursorPos);
        int column = this->utf8Columns(this->cursorPos);
        int newLine = std::max(0, line - std::max(1, this->editorVisibleLines - 1));
        moveCursor(this->visualToByte(newLine, column));
    }
    else if (ImGui::IsKeyPressed(ImGuiKey_PageDown, true))
    {
        int line = this->lineOfOffset(this->cursorPos);
        int column = this->utf8Columns(this->cursorPos);
        int newLine = std::min(this->countLines() - 1, line + std::max(1, this->editorVisibleLines - 1));
        moveCursor(this->visualToByte(newLine, column));
    }
    else if (ImGui::IsKeyPressed(ImGuiKey_Escape))
    {
        this->selectionStart = -1;
    }
}

void TriggerEditor::handleEditorMouse(ImVec2 base)
{
    ImGuiIO &io = ImGui::GetIO();

    // Place the caret where the mouse was clicked. Clamp to the last line.
    const auto placeFromMouse = [&]()
    {
        float fx = io.MousePos.x - base.x + this->scrollX - this->editorGutterW;
        float fy = io.MousePos.y - base.y + this->scrollY;

        int column = fx > 0.f ? (int)std::floor(fx / this->editorCharW) : 0;
        int line = fy > 0.f ? (int)std::floor(fy / this->editorLineH) : 0;

        int lines = this->countLines();
        if (line >= lines)
            line = std::max(0, lines - 1);
        if (line < 0)
            line = 0;

        this->cursorPos = this->visualToByte(line, column);
        this->caretTimer = 0.f;
        this->caretVisible = true;
    };

    if (ImGui::IsItemClicked(ImGuiMouseButton_Left))
    {
        this->selectionStart = -1;
        placeFromMouse();
    }
    else if (ImGui::IsItemActive() && ImGui::IsMouseDragging(ImGuiMouseButton_Left))
    {
        if (this->selectionStart < 0)
            this->selectionStart = this->cursorPos;
        placeFromMouse();
    }

    // Wheel over the editor: vertical scroll (Shift = horizontal).
    if (ImGui::IsItemHovered() && io.MouseWheel != 0.f)
    {
        float step = 3.f * this->editorLineH;
        if (io.KeyShift)
            ImGui::SetScrollX(ImGui::GetScrollX() + io.MouseWheel * 3.f * this->editorCharW);
        else
            ImGui::SetScrollY(ImGui::GetScrollY() - io.MouseWheel * step);
    }
    if (ImGui::IsItemHovered() && io.MouseWheelH != 0.f)
        ImGui::SetScrollX(ImGui::GetScrollX() + io.MouseWheelH * 3.f * this->editorCharW);
}

void TriggerEditor::renderEditorText(ImVec2 base)
{
    ImDrawList *drawList = ImGui::GetWindowDrawList();
    int lines = this->countLines();
    int firstLine = (int)std::floor(this->scrollY / this->editorLineH);
    if (firstLine < 0)
        firstLine = 0;
    int lastLine = std::min(lines - 1, firstLine + this->editorVisibleLines);

    // Line numbers + tokens.
    std::vector<JsonToken> tokens = tokenizeJson(this->text);

    for (int line = firstLine; line <= lastLine; ++line)
    {
        int ls = this->lineStartOffset(line);
        int le = this->lineEndOffset(line);
        float y = base.y - this->scrollY + (float)line * this->editorLineH;

        char number[16];
        std::snprintf(number, sizeof(number), "%d", line + 1);
        drawList->AddText(ImGui::GetFont(), ImGui::GetFontSize(),
                          ImVec2(base.x + 4.f, y), COLOR_LINE_NUMBER, number);

        // Tokens that intersect this line.
        for (const auto &token : tokens)
        {
            if (token.end <= ls)
                continue;
            if (token.start >= le)
                break;

            int s = std::max(token.start, ls);
            int e = std::min(token.end, le);
            if (s >= e)
                continue;

            float x = base.x + this->editorGutterW - this->scrollX
                      + (float)this->utf8Columns(s) * this->editorCharW;
            drawList->AddText(ImGui::GetFont(), ImGui::GetFontSize(), ImVec2(x, y),
                              tokenColor(token.type), this->text.c_str() + s, this->text.c_str() + e);
        }
    }

    // Selection highlight (clipped to the editor child automatically).
    if (this->selectionStart >= 0)
    {
        int selA = std::min(this->cursorPos, this->selectionStart);
        int selB = std::max(this->cursorPos, this->selectionStart);

        for (int line = firstLine; line <= lastLine; ++line)
        {
            int ls = this->lineStartOffset(line);
            int le = this->lineEndOffset(line);
            if (selB <= ls || selA >= le)
                continue;

            int s = std::max(selA, ls);
            int e = std::min(selB, le);

            float x1 = base.x + this->editorGutterW - this->scrollX
                       + (float)this->utf8Columns(s) * this->editorCharW;
            float x2 = base.x + this->editorGutterW - this->scrollX
                       + (float)this->utf8Columns(e) * this->editorCharW;
            float y = base.y - this->scrollY + (float)line * this->editorLineH;

            drawList->AddRectFilled(ImVec2(x1, y), ImVec2(x2, y + this->editorLineH), COLOR_SELECTION);
        }
    }
}

void TriggerEditor::drawCaret(ImVec2 base)
{
    if (!this->editorFocused || !this->caretVisible)
        return;

    int line = this->lineOfOffset(this->cursorPos);
    float x = base.x + this->editorGutterW - this->scrollX
              + (float)this->utf8Columns(this->cursorPos) * this->editorCharW;
    float y = base.y - this->scrollY + (float)line * this->editorLineH;

    ImDrawList *drawList = ImGui::GetWindowDrawList();
    drawList->AddRectFilled(ImVec2(x, y),
                            ImVec2(x + std::max(1.f, this->editorCharW * 0.16f), y + this->editorLineH),
                            COLOR_CARET);
}

void TriggerEditor::ensureCaretVisible()
{
    int line = this->lineOfOffset(this->cursorPos);
    float caretX = this->editorGutterW + (float)this->utf8Columns(this->cursorPos) * this->editorCharW;
    float caretY = (float)line * this->editorLineH;

    float left = this->scrollX;
    float right = this->scrollX + std::max(1.f, this->editorViewW - this->editorGutterW);
    if (caretX < left)
        ImGui::SetScrollX(caretX - this->editorCharW);
    else if (caretX > right)
        ImGui::SetScrollX(caretX - (this->editorViewW - this->editorGutterW) + this->editorCharW);

    float top = this->scrollY;
    float bottom = this->scrollY + this->editorViewH;
    if (caretY < top)
        ImGui::SetScrollY(caretY);
    else if (caretY + this->editorLineH > bottom)
        ImGui::SetScrollY(caretY + this->editorLineH - this->editorViewH);
}

void TriggerEditor::renderEditorStatus()
{
    
    float scale = this->imguiScale;

    // Left: JSON validity.
    if (this->jsonValid)
    {
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.45f, 0.85f, 0.45f, 1.0f));
        ImGui::TextUnformatted(this->getLanguage("VALID").c_str());
        ImGui::PopStyleColor();
    }
    else
    {
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 0.45f, 0.45f, 1.0f));
        if (this->parseErrorLine > 0)
            ImGui::TextUnformatted((this->getLanguage("INVALID")
                                    + " (" + this->getLanguage("LINE") + " " + toStr(this->parseErrorLine)
                                    + ", " + this->getLanguage("COL") + " " + toStr(this->parseErrorCol) + ")")
                                       .c_str());
        else
            ImGui::TextUnformatted(this->getLanguage("INVALID").c_str());
        ImGui::PopStyleColor();
    }

    // Right: cursor position + dirty marker.
    int line = this->lineOfOffset(this->cursorPos) + 1;
    int column = this->utf8Columns(this->cursorPos) + 1;
    std::string status = this->getLanguage("LINE") + " " + toStr(line)
                         + "  " + this->getLanguage("COL") + " " + toStr(column);

    float rightEdge = ImGui::GetWindowWidth();
    if (this->dirty)
    {
        ImGui::SameLine();
        ImGui::SetCursorPosX(rightEdge - 300.f * scale);
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 0.85f, 0.3f, 1.0f));
        ImGui::TextUnformatted(("[!] " + this->getLanguage("UNSAVED")).c_str());
        ImGui::PopStyleColor();
        ImGui::SameLine();
        ImGui::SetCursorPosX(rightEdge - 150.f * scale);
    }
    else
    {
        ImGui::SameLine();
        ImGui::SetCursorPosX(rightEdge - 150.f * scale);
    }

    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.7f, 0.7f, 0.75f, 1.0f));
    ImGui::TextUnformatted(status.c_str());
    ImGui::PopStyleColor();
}

// Raw JSON editor body (status line + code editor). Rendered inside the JSON
// tab so the custom key/mouse handling never runs while the GUI tab is active.
void TriggerEditor::renderJsonBody()
{
    float scale = this->imguiScale;

    this->renderEditorStatus();
    ImGui::Separator();

    this->editorCharW = ImGui::CalcTextSize("M").x;
    this->editorLineH = ImGui::GetTextLineHeightWithSpacing();

    int lines = this->countLines();
    int lineDigits = 1;
    for (int lc = lines; lc >= 10; lc /= 10)
        ++lineDigits;
    this->editorGutterW = (float)(lineDigits + 1) * this->editorCharW + 8.f * scale;

    ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0.06f, 0.06f, 0.09f, 1.0f));
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.f, 0.f));
    // The negative height leaves room below for the log panel.
    ImGui::BeginChild("##trigger-editor-code", ImVec2(0, -150.f * this->imguiScale), true,
                      ImGuiWindowFlags_NoScrollWithMouse);
    ImGui::PopStyleVar();
    ImGui::PopStyleColor();

    ImVec2 base = ImGui::GetCursorScreenPos();
    ImVec2 viewport = ImGui::GetContentRegionAvail();
    this->editorViewW = std::max(1.f, viewport.x);
    this->editorViewH = std::max(1.f, viewport.y);
    this->editorVisibleLines = std::max(1, (int)std::ceil(this->editorViewH / this->editorLineH) + 1);

    // Content size drives the scrollbars.
    float maxColumns = 0.f;
    for (int line = 0; line < lines; ++line)
    {
        int ls = this->lineStartOffset(line);
        int le = this->lineEndOffset(line);
        maxColumns = std::max(maxColumns, (float)this->utf8Columns(le) - (float)this->utf8Columns(ls));
    }
    float contentW = this->editorGutterW + maxColumns * this->editorCharW + 4.f * scale;
    float contentH = (float)lines * this->editorLineH + 4.f * scale;
    ImGui::Dummy(ImVec2(contentW, contentH));

    this->scrollX = ImGui::GetScrollX();
    this->scrollY = ImGui::GetScrollY();

    // Input capture layer on top of the content (same position as the viewport).
    ImGui::SetCursorScreenPos(base);
    ImGui::InvisibleButton("##trigger-editor-input", viewport);
    ImGui::SetItemAllowOverlap();

    this->editorFocused = ImGui::IsItemActive() || ImGui::IsItemFocused();

    // Caret blink.
    this->caretTimer += ImGui::GetIO().DeltaTime;
    if (this->caretTimer > 1.0f)
        this->caretTimer = 0.f;
    this->caretVisible = this->caretTimer < 0.5f;

    if (this->fileLoaded)
    {
        this->handleEditorMouse(base);
        this->handleEditorKeys();
        this->renderEditorText(base);
        this->drawCaret(base);
        this->ensureCaretVisible();
    }
    else
    {
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.6f, 0.6f, 0.6f, 1.0f));
        ImGui::SetCursorScreenPos(ImVec2(base.x + this->editorGutterW + 8.f * this->imguiScale,
                                          base.y + 10.f * this->imguiScale));
        ImGui::TextUnformatted(this->getLanguage("NO-FILE").c_str());
        ImGui::PopStyleColor();
    }

    ImGui::EndChild();
}

void TriggerEditor::renderEditor()
{
    float scale = this->imguiScale;

    // ---- header: current file ----
    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.9f, 0.9f, 0.9f, 1.0f));
    if (this->fileLoaded)
        ImGui::TextUnformatted((this->currentFile + "  (" + this->currentFilePath + ")").c_str());
    else
        ImGui::TextUnformatted(this->getLanguage("NO-FILE").c_str());
    ImGui::PopStyleColor();
    ImGui::Separator();

    this->renderToolbar();
    ImGui::Separator();

    // ---- tabs: GUI (trigger blocks) | Misc (other fields) | JSON (raw, read-only) ----
    // The GUI and Misc tabs edit the parsed model; the JSON tab is a
    // read-only viewer of the current text. If the JSON cannot be converted to
    // the model (invalid JSON, wrong structure, ...), the GUI/Misc tabs are
    // blocked until the file is fixed.
    ImGui::BeginTabBar("##trigger-tabs");

    // In ImGui 1.85 a tab click only registers NextSelectedTabId, which the
    // tab bar applies on the NEXT frame: BeginTabItem() returns false during
    // the click frame, so gating the click on its return value never fires and
    // the SetSelected flag of the active tab would undo the user's click every
    // frame. The click is therefore detected through IsItemClicked() right
    // after BeginTabItem(), which works because the tab item is always
    // submitted (ItemAdd + ButtonBehavior) even when it is not visible yet.
    const auto switchTab = [&](GuiTab target, bool needsConversion)
    {
        ImGuiTabItemFlags flags = (this->guiTab == target) ? ImGuiTabItemFlags_SetSelected : 0;
        bool tab = ImGui::BeginTabItem((target == GuiTab::Visual) ? "GUI"
                                        : (target == GuiTab::Misc) ? "Misc" : "JSON",
                                       nullptr, flags);
        bool clicked = ImGui::IsItemClicked();
        if (clicked && this->guiTab != target)
        {
            if (!needsConversion || this->jsonToGui())
                this->guiTab = target;
            else
            {
                this->guiConvertible = false;
                this->showError("[erro] " + this->guiConvertError);
            }
        }
        if (tab)
            ImGui::EndTabItem();
    };

    switchTab(GuiTab::Visual, true);   // needs the parsed model
    switchTab(GuiTab::Misc, true);     // needs the parsed model
    switchTab(GuiTab::Json, false);    // read-only viewer, always available

    ImGui::EndTabBar();
    ImGui::Separator();

    // ---- active tab content ----
    if (this->guiTab == GuiTab::Visual)
        this->renderGuiEditor();
    else if (this->guiTab == GuiTab::Misc)
        this->renderGuiMiscEditor();
    else
        this->renderJsonBody();

    // Log panel below the editor, inside the right panel.
    ImGui::Separator();
    ImGui::BeginChild("##trigger-log", ImVec2(0, 150.f * this->imguiScale), true);
    this->renderLog();
    ImGui::EndChild();
}

void TriggerEditor::renderSidebar()
{
    float scale = this->imguiScale;

    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.9f, 0.9f, 0.9f, 1.0f));
    ImGui::TextUnformatted(this->getLanguage("TRIGGERS").c_str());
    ImGui::PopStyleColor();
    ImGui::SameLine();
    ImGui::SetCursorPosX(ImGui::GetCursorPosX() + 10.f * scale);

    if (ImGui::SmallButton("[+]"))
        this->createPopupOpen = true;
    if (ImGui::IsItemHovered())
        ImGui::SetTooltip("%s", this->getLanguage("NEW-TRIGGER").c_str());

    if (!this->triggerFolderRef.empty())
    {
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.55f, 0.55f, 0.62f, 1.0f));
        ImGui::TextWrapped("%s: %s", this->getLanguage("FOLDER").c_str(), this->triggerFolderRef.c_str());
        ImGui::PopStyleColor();
    }

    ImGui::Separator();

    if (this->triggerFileList.empty())
    {
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.6f, 0.6f, 0.6f, 1.0f));
        ImGui::TextWrapped("%s", this->getLanguage("NO-TRIGGER-FILES").c_str());
        ImGui::PopStyleColor();
    }
    else
    {
        for (const auto &file : this->triggerFileList)
        {
            bool isMain = (file == this->mainTriggerFile);
            bool selected = (file == this->currentFile);

            ImGui::PushID(file.c_str());
            if (ImGui::Selectable(file.c_str(), selected))
                this->requestAction("file", file);

            // The main (map-linked) trigger gets a small colored tag, drawn
            // directly with the draw list so the row cursor is not moved.
            if (isMain)
            {
                ImVec2 itemMin = ImGui::GetItemRectMin();
                ImVec2 itemMax = ImGui::GetItemRectMax();
                ImDrawList *drawList = ImGui::GetWindowDrawList();
                ImFont *font = ImGui::GetFont();
                std::string tag = this->getLanguage("MAIN");
                float tagW = font->CalcTextSizeA(font->FontSize, FLT_MAX, 0.f, tag.c_str()).x + 10.f * scale;
                float x = itemMax.x - tagW - 6.f * scale;
                float cy = itemMin.y + (itemMax.y - itemMin.y) * 0.5f;
                drawList->AddRectFilled(ImVec2(x, itemMin.y + 2.f * scale),
                                        ImVec2(itemMax.x - 4.f * scale, itemMax.y - 2.f * scale),
                                        IM_COL32(90, 170, 90, 130));
                drawList->AddText(font, font->FontSize,
                                  ImVec2(x + 5.f * scale, cy - font->FontSize * 0.5f),
                                  IM_COL32(235, 255, 235, 255), tag.c_str());
                if (ImGui::IsItemHovered())
                    ImGui::SetTooltip("%s", this->getLanguage("MAIN-TRIGGER-HINT").c_str());
            }
            ImGui::PopID();
        }
    }

    ImGui::Separator();

    // Delete the currently open trigger (never the main one).
    bool canDelete = this->fileLoaded && !this->currentFile.empty() && this->currentFile != this->mainTriggerFile;
    if (canDelete)
    {
        if (ImGui::Button(this->getLanguage("DELETE").c_str(), ImVec2(-1.f, 34.f * scale)))
        {
            this->deleteTarget = this->currentFile;
            this->deletePopupOpen = true;
        }
    }
    else
    {
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.45f, 0.45f, 0.5f, 1.0f));
        ImGui::Button(this->getLanguage("DELETE").c_str(), ImVec2(-1.f, 34.f * scale));
        ImGui::PopStyleColor();
        if (ImGui::IsItemHovered())
            ImGui::SetTooltip("%s", this->getLanguage("DELETE-MAIN-FORBIDDEN").c_str());
    }

    ImGui::Spacing();

    if (ImGui::Button(this->getLanguage("CHANGE-MAP").c_str(), ImVec2(-1.f, 34.f * scale)))
        this->requestAction("map", "");

    if (ImGui::Button(this->getLanguage("BACK").c_str(), ImVec2(-1.f, 34.f * scale)))
        this->requestAction("back", "");
}

void TriggerEditor::renderToolbar()
{
    
    float scale = this->imguiScale;

    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.14f, 0.58f, 0.32f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.18f, 0.68f, 0.38f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.11f, 0.48f, 0.27f, 1.0f));
    bool saveClicked = ImGui::Button(this->getLanguage("SAVE").c_str(), ImVec2(130.f * scale, 32.f * scale));
    ImGui::PopStyleColor(3);
    if (saveClicked && this->fileLoaded)
        this->saveCurrentFile();

    ImGui::SameLine();
    // The JSON tab is read-only: formatting is meaningless there (and would
    // touch a buffer the user cannot edit), so the button is disabled.
    bool formatEnabled = (this->guiTab != GuiTab::Json);
    if (!formatEnabled)
        ImGui::BeginDisabled();
    if (ImGui::Button(this->getLanguage("FORMAT").c_str(), ImVec2(130.f * scale, 32.f * scale)))
        this->formatJson();
    if (!formatEnabled)
        ImGui::EndDisabled();

    // The file path is already shown in the header above the toolbar, so it is
    // not repeated here.
}

void TriggerEditor::renderLog()
{
    float scale = this->imguiScale;

    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.9f, 0.9f, 0.9f, 1.0f));
    ImGui::TextUnformatted(this->getLanguage("LOG").c_str());
    ImGui::PopStyleColor();
    ImGui::SameLine();
    ImGui::SetCursorPosX(ImGui::GetCursorPosX() + 10.f * scale);
    if (ImGui::SmallButton(this->getLanguage("CLEAR-LOG").c_str()))
        this->logLines.clear();
    ImGui::Separator();

    ImGui::BeginChild("##trigger-log-scroll", ImVec2(0, 0), false, ImGuiWindowFlags_AlwaysVerticalScrollbar);
    for (auto &line : this->logLines)
    {
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(
            ((line.color >> 24) & 0xFF) / 255.0f,
            ((line.color >> 16) & 0xFF) / 255.0f,
            ((line.color >> 8) & 0xFF) / 255.0f,
            (line.color & 0xFF) / 255.0f));
        ImGui::TextWrapped("%s", line.text.c_str());
        ImGui::PopStyleColor();
    }
    if (this->logDirty)
    {
        ImGui::SetScrollHereY(1.0f);
        this->logDirty = false;
    }
    ImGui::EndChild();
}

// ---------------------------------------------------------------------------
// Visual (GUI) editor
// ---------------------------------------------------------------------------

// The visual editor shows each type through its metadata definition. Its
// chrome strings are intentionally hardcoded in English (see the feature
// note: descriptions are kept in English for now, regardless of the game
// language, to speed up the initial implementation).

const std::vector<TriggerTypeDef> &TriggerEditor::guiDefs(GuiKind kind) const
{
    switch (kind)
    {
        case GuiKind::Event:     return GUI_EVENT_DEFS;
        case GuiKind::Condition: return GUI_CONDITION_DEFS;
        case GuiKind::Action:    return GUI_ACTION_DEFS;
    }
    return GUI_EVENT_DEFS;
}

std::string TriggerEditor::guiDescription(GuiKind kind, const std::string &type) const
{
    const TriggerTypeDef *def = guiFindDef(this->guiDefs(kind), type);
    if (def != nullptr)
        return def->description;
    return type.empty() ? "(unknown)" : type;
}

// Compact summary of the entry parameters (e.g. "unitName: skeleton
// strings: special/dialog-new, special/dialog-update"), shown in yellow next
// to the entry description. Covers every field of the raw record, including
// unknown ones, so nothing is hidden.
std::string TriggerEditor::guiEntryValues(GuiKind kind, const GuiEntry &entry) const
{
    const char *typeKey = "action";
    switch (kind)
    {
        case GuiKind::Event:     typeKey = "event";     break;
        case GuiKind::Condition: typeKey = "condition"; break;
        case GuiKind::Action:    typeKey = "action";    break;
    }

    if (!entry.raw.is_object())
        return "";

    std::string summary;
    const auto append = [&](const std::string &label, const std::string &value)
    {
        if (value.empty())
            return;
        if (!summary.empty())
            summary += "   ";
        summary += label + ": " + value;
    };

    for (auto &item : entry.raw.items())
    {
        const std::string &key = item.key();
        if (key == typeKey)
            continue;
        const json &value = item.value();
        if (value.is_string())
            append(key, value.get<std::string>());
        else if (value.is_boolean())
            append(key, value.get<bool>() ? "true" : "false");
        else if (value.is_number_integer())
            append(key, toStr(value.get<int>()));
        else if (value.is_number_float())
            append(key, toStr(value.get<float>()));
        else if (value.is_array())
        {
            std::string joined;
            for (const auto &element : value)
                if (element.is_object() && element.contains("value"))
                {
                    if (!joined.empty())
                        joined += ", ";
                    if (element["value"].is_string())
                        joined += element["value"].get<std::string>();
                    else
                        joined += element["value"].dump();
                }
            append(key, joined);
        }
    }
    return summary;
}

std::vector<GuiEntry> &TriggerEditor::guiEntryList(GuiKind kind, GuiTrigger &trigger)
{
    switch (kind)
    {
        case GuiKind::Event:     return trigger.events;
        case GuiKind::Condition: return trigger.conditions;
        case GuiKind::Action:    return trigger.actions;
    }
    return trigger.events;
}

// Parses this->text into the GUI model. Returns false (and stores a message)
// when the JSON cannot be represented in the visual editor; in that case the
// user must fix the JSON before switching to the GUI tab.
bool TriggerEditor::jsonToGui()
{
    this->guiTriggers.clear();
    this->guiConvertError.clear();
    this->guiConvertible = true;

    try
    {
        json root = json::parse(this->text);
        if (!root.is_object())
        {
            this->guiConvertError = "The trigger file must be a JSON object.";
            this->guiConvertible = false;
            return false;
        }

        this->guiRoot = root;

        if (!root.contains("trigger") || root["trigger"].is_null())
        {
            // No trigger array yet: start with a single empty block.
            this->guiTriggers.push_back(GuiTrigger());
            return true;
        }

        if (!root["trigger"].is_array())
        {
            this->guiConvertError = "The \"trigger\" field must be an array of blocks.";
            this->guiConvertible = false;
            return false;
        }

        for (const auto &block : root["trigger"])
        {
            if (!block.is_object())
            {
                this->guiConvertError = "Each trigger block must be an object.";
                this->guiConvertible = false;
                return false;
            }

            GuiTrigger trigger;

            // `typeKey` is the JSON field that carries the entry type for this
            // list ("event" / "condition" / "action"); `group` only selects the
            // and/or and then/else grouping. Reading the type from the wrong
            // field made every action load as unknown (the "then" list read
            // "event" from action records).
            const auto parseList = [&](const char *key, const char *typeKey, int group)
            {
                std::vector<GuiEntry> list;
                if (!block.contains(key) || block[key].is_null())
                    return list;
                if (!block[key].is_array())
                {
                    this->guiConvertError = std::string("The \"") + key + "\" field must be an array.";
                    this->guiConvertible = false;
                    return list;
                }
                for (const auto &entry : block[key])
                {
                    if (!entry.is_object())
                    {
                        this->guiConvertError = std::string("Entries inside \"") + key + "\" must be objects.";
                        this->guiConvertible = false;
                        return list;
                    }
                    GuiEntry e;
                    e.raw = entry;
                    e.group = group;
                    e.type = entry.value(typeKey, "");
                    list.push_back(e);
                }
                return list;
            };

            trigger.events = parseList("events", "event", 0);
            if (!this->guiConvertible)
                return false;
            trigger.conditions = parseList("conditions", "condition", 0);
            if (!this->guiConvertible)
                return false;
            std::vector<GuiEntry> conditionsOr = parseList("conditions-or", "condition", 1);
            if (!this->guiConvertible)
                return false;
            trigger.conditions.insert(trigger.conditions.end(), conditionsOr.begin(), conditionsOr.end());
            // group matches guiToJson: 0 = then, 1 = else (same convention as
            // the popup branch radios and the GuiEntry.group documentation).
            trigger.actions = parseList("then", "action", 0);
            if (!this->guiConvertible)
                return false;
            std::vector<GuiEntry> actionsElse = parseList("else", "action", 1);
            if (!this->guiConvertible)
                return false;
            trigger.actions.insert(trigger.actions.end(), actionsElse.begin(), actionsElse.end());

            this->guiTriggers.push_back(trigger);
        }

        if (this->guiTriggers.empty())
            this->guiTriggers.push_back(GuiTrigger());
        return true;
    }
    catch (const json::parse_error &e)
    {
        this->guiConvertError = std::string("Invalid JSON: ") + e.what();
    }
    catch (...)
    {
        this->guiConvertError = "Invalid JSON.";
    }

    this->guiConvertible = false;
    return false;
}

// Regenerates this->text from the GUI model, keeping every non-trigger field
// of the document (item-drop, environments-drop, player-characters, ...).
void TriggerEditor::guiToJson()
{
    json root = this->guiRoot;
    if (!root.is_object())
        root = json::object();

    json triggers = json::array();

    for (const auto &trigger : this->guiTriggers)
    {
        json block = json::object();

        json events = json::array();
        for (const auto &e : trigger.events)
        {
            json j = e.raw;
            j["event"] = e.type;
            events.push_back(j);
        }
        block["events"] = events;

        json conditions = json::array();
        json conditionsOr = json::array();
        for (const auto &c : trigger.conditions)
        {
            json j = c.raw;
            j["condition"] = c.type;
            if (c.group == 0)
                conditions.push_back(j);
            else
                conditionsOr.push_back(j);
        }
        block["conditions"] = conditions;
        if (!conditionsOr.empty())
            block["conditions-or"] = conditionsOr;

        json thenActions = json::array();
        json elseActions = json::array();
        for (const auto &a : trigger.actions)
        {
            json j = a.raw;
            j["action"] = a.type;
            if (a.group == 0)
                thenActions.push_back(j);
            else
                elseActions.push_back(j);
        }
        block["then"] = thenActions;
        if (!elseActions.empty())
            block["else"] = elseActions;

        triggers.push_back(block);
    }

    root["trigger"] = triggers;
    this->guiRoot = root;
    this->text = root.dump(4) + "\n";
}

// Called after every GUI mutation: keeps the raw JSON text in sync with the
// visual model (the JSON tab is updated in real time).
void TriggerEditor::guiSyncText()
{
    this->guiToJson();
    this->validateJson();
    this->dirty = true;
    this->caretTimer = 0.f;
}

void TriggerEditor::guiMoveEntry(std::vector<GuiEntry> &entries, int index, int direction)
{
    int target = index + direction;
    if (index < 0 || target < 0 || target >= (int)entries.size())
        return;
    std::swap(entries[index], entries[target]);
    this->guiSyncText();
}

void TriggerEditor::guiDeleteEntry(std::vector<GuiEntry> &entries, int index)
{
    if (index < 0 || index >= (int)entries.size())
        return;
    entries.erase(entries.begin() + index);
    this->guiSyncText();
}

// Populates the popup buffers from a raw record (sized to the metadata plus
// any extra values already present in the file, so nothing is lost).
static void guiPopulateBuffers(GuiPopupState &popup, const TriggerTypeDef *def)
{
    popup.unitNameBuf[0] = '\0';
    popup.textBuf[0] = '\0';
    popup.intValueBuf = 0;
    popup.floatValueBuf = 0.f;
    popup.stringsBuf.clear();
    popup.intsBuf.clear();
    popup.floatsBuf.clear();
    popup.boolsBuf.clear();

    const json &raw = popup.draft.raw;
    if (!raw.is_object())
        return;

    const auto copyString = [&](const char *key, char *out, size_t size)
    {
        if (raw.contains(key) && raw[key].is_string())
        {
            std::string v = raw[key].get<std::string>();
            std::strncpy(out, v.c_str(), size - 1);
        }
    };
    copyString("unitName", popup.unitNameBuf, sizeof(popup.unitNameBuf));
    copyString("text", popup.textBuf, sizeof(popup.textBuf));
    popup.intValueBuf = raw.value("integerValue", 0);
    popup.floatValueBuf = raw.value("floatValue", 0.f);

    const auto copyArray = [&](const char *key, auto &out)
    {
        int maxIndex = -1;
        if (def != nullptr)
            for (const auto &p : def->params)
                if (p.field == key)
                    maxIndex = std::max(maxIndex, p.index);
        int size = 0;
        if (raw.contains(key) && raw[key].is_array())
            size = (int)raw[key].size();
        int rows = std::max(maxIndex + 1, size);
        out.resize(rows);
        for (int i = 0; i < rows; ++i)
            if (i < size)
            {
                const json &item = raw[key][i];
                using V = typename std::decay_t<decltype(out)>::value_type;
                if constexpr (std::is_same_v<V, std::string>)
                    out[i] = item.value("value", "");
                else if constexpr (std::is_same_v<V, bool>)
                    out[i] = item.value("value", true);
                else
                    out[i] = (V)item.value("value", 0);
            }
    };
    copyArray("strings", popup.stringsBuf);
    copyArray("integers", popup.intsBuf);
    copyArray("floats", popup.floatsBuf);
    copyArray("booleans", popup.boolsBuf);
}

void TriggerEditor::openGuiPopup(GuiKind kind, int triggerIndex, int entryIndex)
{
    this->guiPopup.open = true;
    this->guiPopup.kind = kind;
    this->guiPopup.editing = (entryIndex >= 0);
    this->guiPopup.triggerIndex = triggerIndex;
    this->guiPopup.entryIndex = entryIndex;
    this->guiPopup.search[0] = '\0';
    this->guiPopup.selectedType.clear();
    this->guiPopup.error.clear();
    this->guiPopup.groupBuf = 0;

    if (this->guiPopup.editing)
    {
        if (triggerIndex >= 0 && triggerIndex < (int)this->guiTriggers.size())
        {
            std::vector<GuiEntry> &list = this->guiEntryList(kind, this->guiTriggers[triggerIndex]);
            if (entryIndex >= 0 && entryIndex < (int)list.size())
            {
                this->guiPopup.draft = list[entryIndex];
                this->guiPopup.selectedType = list[entryIndex].type;
                this->guiPopup.groupBuf = list[entryIndex].group;
                const TriggerTypeDef *def = guiFindDef(this->guiDefs(kind), list[entryIndex].type);
                guiPopulateBuffers(this->guiPopup, def);
                return;
            }
        }
        this->guiPopup.editing = false;
        this->guiPopup.entryIndex = -1;
    }

    this->guiPopup.draft = GuiEntry();
    this->guiPopup.draft.raw = json::object();
    guiPopulateBuffers(this->guiPopup, nullptr);
}

void TriggerEditor::commitGuiPopup()
{
    if (this->guiPopup.selectedType.empty())
        return;

    GuiEntry e = this->guiPopup.draft;
    e.type = this->guiPopup.selectedType;
    e.group = this->guiPopup.groupBuf;

    json raw = e.raw;
    if (!raw.is_object())
        raw = json::object();

    const auto setScalar = [&](const char *key, const char *value)
    {
        if (value != nullptr && value[0] != '\0')
            raw[key] = std::string(value);
        else
            raw.erase(key);
    };
    setScalar("unitName", this->guiPopup.unitNameBuf);
    setScalar("text", this->guiPopup.textBuf);
    // Keep explicit zeros when the original record carried the field (the
    // engine treats a missing scalar as 0 anyway, but don't drop what the
    // file already had); otherwise omit clean empty defaults.
    if (this->guiPopup.intValueBuf != 0 || raw.contains("integerValue"))
        raw["integerValue"] = this->guiPopup.intValueBuf;
    else
        raw.erase("integerValue");
    if (this->guiPopup.floatValueBuf != 0.f || raw.contains("floatValue"))
        raw["floatValue"] = this->guiPopup.floatValueBuf;
    else
        raw.erase("floatValue");

    const auto writeArray = [&](const char *key, const auto &vec)
    {
        using V = typename std::decay_t<decltype(vec)>::value_type;
        bool any = false;
        for (const auto &v : vec)
            if constexpr (std::is_same_v<V, std::string>)
            {
                if (!v.empty())
                {
                    any = true;
                    break;
                }
            }
            else
            {
                any = true;
                break;
            }

        if (!any)
        {
            raw.erase(key);
            return;
        }

        json arr = json::array();
        for (const auto &v : vec)
        {
            json item = json::object();
            if constexpr (std::is_same_v<V, std::string>)
            {
                if (v.empty())
                    continue;
                item["value"] = v;
            }
            else if constexpr (std::is_same_v<V, bool>)
                item["value"] = v;
            else
                item["value"] = v;
            arr.push_back(item);
        }
        raw[key] = arr;
    };
    writeArray("strings", this->guiPopup.stringsBuf);
    writeArray("integers", this->guiPopup.intsBuf);
    writeArray("floats", this->guiPopup.floatsBuf);
    writeArray("booleans", this->guiPopup.boolsBuf);

    e.raw = raw;

    if (this->guiPopup.triggerIndex >= 0 && this->guiPopup.triggerIndex < (int)this->guiTriggers.size())
    {
        std::vector<GuiEntry> &list = this->guiEntryList(this->guiPopup.kind, this->guiTriggers[this->guiPopup.triggerIndex]);
        if (this->guiPopup.editing && this->guiPopup.entryIndex >= 0 && this->guiPopup.entryIndex < (int)list.size())
            list[this->guiPopup.entryIndex] = e;
        else
            list.push_back(e); // newest entries are appended at the bottom
    }

    this->guiPopup.open = false;
    this->guiSyncText();
}

void TriggerEditor::renderGuiPopup()
{
    if (!this->guiPopup.open)
        return;

    
    float scale = this->imguiScale;

    const char *title = "Add";
    const char *kindLabel = "Event";
    switch (this->guiPopup.kind)
    {
        case GuiKind::Event:     kindLabel = "Event";     break;
        case GuiKind::Condition: kindLabel = "Condition"; break;
        case GuiKind::Action:    kindLabel = "Action";    break;
    }
    std::string caption = std::string(this->guiPopup.editing ? "Edit " : "Add ") + kindLabel;

    // The registration popup is deliberately large (~60% of the screen) so the
    // type lookup and the parameter editors have room to breathe. The search
    // box, the lookup list and the parameter group all stretch to the popup
    // width (the bordered "group" around them is the child window frame).
    ImVec2 popupSize(std::max(GUI_POPUP_MIN_WIDTH, this->viewportW * GUI_POPUP_SIZE_FRACTION),
                     std::max(GUI_POPUP_MIN_HEIGHT, this->viewportH * GUI_POPUP_SIZE_FRACTION));

    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 6.0f * scale);
    ImGui::OpenPopup("##trigger-gui-entry");
    ImGui::SetNextWindowSize(popupSize, ImGuiCond_Always);
    if (ImGui::BeginPopupModal("##trigger-gui-entry", nullptr,
                               ImGuiWindowFlags_NoSavedSettings))
    {
        ImGui::TextUnformatted(caption.c_str());
        ImGui::Separator();

        const std::vector<TriggerTypeDef> &defs = this->guiDefs(this->guiPopup.kind);

        // ---- searchable lookup (stretches to the popup width) ----
        ImGui::SetNextItemWidth(-FLT_MIN);
        ImGui::InputTextWithHint("##gui-search", "Search type...",
                                 this->guiPopup.search, sizeof(this->guiPopup.search));

        std::string filter = this->guiPopup.search;
        boost::to_lower(filter);

        float listHeight = popupSize.y * GUI_TYPE_LIST_HEIGHT_FRACTION;
        ImGui::BeginChild("##gui-type-list", ImVec2(0, listHeight), true);
        for (const auto &def : defs)
        {
            std::string haystack = def.description + " " + def.key;
            std::string lower = haystack;
            boost::to_lower(lower);
            if (!filter.empty() && lower.find(filter) == std::string::npos)
                continue;

            bool selected = (def.key == this->guiPopup.selectedType);
            if (ImGui::Selectable((def.description + "  (" + def.key + ")").c_str(), selected))
            {
                bool typeChanged = (def.key != this->guiPopup.selectedType);
                this->guiPopup.selectedType = def.key;
                if (typeChanged && !this->guiPopup.editing)
                {
                    // Fresh entry: reset the buffers for the new type.
                    this->guiPopup.draft = GuiEntry();
                    this->guiPopup.draft.raw = json::object();
                    guiPopulateBuffers(this->guiPopup, &def);
                }
            }
        }
        ImGui::EndChild();

        // ---- parameter editors for the selected type ----
        const TriggerTypeDef *def = guiFindDef(defs, this->guiPopup.selectedType);
        if (def != nullptr)
        {
            ImGui::Spacing();
            ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.6f, 0.8f, 1.0f, 1.0f));
            ImGui::TextWrapped("%s", def->description.c_str());
            ImGui::PopStyleColor();
            ImGui::Separator();

            ImGui::BeginChild("##gui-params", ImVec2(0, 0), true);

            // Group selector for conditions (and/or) and actions (then/else).
            if (this->guiPopup.kind == GuiKind::Condition)
            {
                ImGui::TextUnformatted("Group");
                ImGui::SameLine(120.f * scale);
                ImGui::RadioButton("AND", &this->guiPopup.groupBuf, 0);
                ImGui::SameLine();
                ImGui::RadioButton("OR", &this->guiPopup.groupBuf, 1);
            }
            else if (this->guiPopup.kind == GuiKind::Action)
            {
                ImGui::TextUnformatted("Branch");
                ImGui::SameLine(120.f * scale);
                ImGui::RadioButton("Then", &this->guiPopup.groupBuf, 0);
                ImGui::SameLine();
                ImGui::RadioButton("Else", &this->guiPopup.groupBuf, 1);
            }

            const auto labelFor = [&](const TriggerParam &p) -> std::string
            {
                std::string label = p.label;
                if (!p.required)
                    label += " (optional)";
                return label;
            };

            // Scalars first (unitName / text / integerValue / floatValue).
            for (const auto &p : def->params)
            {
                if (p.index >= 0)
                    continue;
                ImGui::PushID(p.field.c_str());
                if (p.field == "unitName")
                    ImGui::InputText(labelFor(p).c_str(), this->guiPopup.unitNameBuf, sizeof(this->guiPopup.unitNameBuf));
                else if (p.field == "text")
                    ImGui::InputText(labelFor(p).c_str(), this->guiPopup.textBuf, sizeof(this->guiPopup.textBuf));
                else if (p.field == "integerValue")
                    ImGui::InputInt(labelFor(p).c_str(), &this->guiPopup.intValueBuf);
                else if (p.field == "floatValue")
                    ImGui::InputFloat(labelFor(p).c_str(), &this->guiPopup.floatValueBuf);
                ImGui::PopID();
            }

            // Array rows (strings / integers / floats / booleans).
            const auto arrayRows = [&](const std::string &field, int &maxIndex) -> int
            {
                maxIndex = -1;
                for (const auto &p : def->params)
                    if (p.field == field)
                        maxIndex = std::max(maxIndex, p.index);
                return maxIndex;
            };

            int maxIndex = -1;
            if (arrayRows("strings", maxIndex) >= 0)
            {
                ImGui::Separator();
                ImGui::TextUnformatted("Texts");
                for (int i = 0; i < (int)this->guiPopup.stringsBuf.size(); ++i)
                {
                    ImGui::PushID(("s" + std::to_string(i)).c_str());
                    std::string label = "String " + std::to_string(i + 1);
                    for (const auto &p : def->params)
                        if (p.field == "strings" && p.index == i)
                            label = p.label;
                    // ImGui has no built-in std::string input; round-trip
                    // through a local char buffer each frame.
                    char buf[512];
                    std::strncpy(buf, this->guiPopup.stringsBuf[i].c_str(), sizeof(buf) - 1);
                    buf[sizeof(buf) - 1] = '\0';
                    if (ImGui::InputText(label.c_str(), buf, sizeof(buf)))
                        this->guiPopup.stringsBuf[i] = buf;
                    ImGui::PopID();
                }
                if (ImGui::SmallButton("+ Add text"))
                    this->guiPopup.stringsBuf.push_back("");
            }

            if (arrayRows("integers", maxIndex) >= 0)
            {
                ImGui::Separator();
                ImGui::TextUnformatted("Integers");
                for (int i = 0; i < (int)this->guiPopup.intsBuf.size(); ++i)
                {
                    ImGui::PushID(("i" + std::to_string(i)).c_str());
                    std::string label = "Integer " + std::to_string(i + 1);
                    for (const auto &p : def->params)
                        if (p.field == "integers" && p.index == i)
                            label = p.label;
                    ImGui::InputInt(label.c_str(), &this->guiPopup.intsBuf[i]);
                    ImGui::PopID();
                }
                if (ImGui::SmallButton("+ Add integer"))
                    this->guiPopup.intsBuf.push_back(0);
            }

            if (arrayRows("floats", maxIndex) >= 0)
            {
                ImGui::Separator();
                ImGui::TextUnformatted("Floats");
                for (int i = 0; i < (int)this->guiPopup.floatsBuf.size(); ++i)
                {
                    ImGui::PushID(("f" + std::to_string(i)).c_str());
                    std::string label = "Float " + std::to_string(i + 1);
                    for (const auto &p : def->params)
                        if (p.field == "floats" && p.index == i)
                            label = p.label;
                    ImGui::InputFloat(label.c_str(), &this->guiPopup.floatsBuf[i]);
                    ImGui::PopID();
                }
                if (ImGui::SmallButton("+ Add float"))
                    this->guiPopup.floatsBuf.push_back(0.f);
            }

            if (arrayRows("booleans", maxIndex) >= 0)
            {
                ImGui::Separator();
                ImGui::TextUnformatted("Booleans");
                for (int i = 0; i < (int)this->guiPopup.boolsBuf.size(); ++i)
                {
                    ImGui::PushID(("b" + std::to_string(i)).c_str());
                    std::string label = "Boolean " + std::to_string(i + 1);
                    for (const auto &p : def->params)
                        if (p.field == "booleans" && p.index == i)
                            label = p.label;
                    // std::vector<bool> stores bits (proxy references), so
                    // edit through a local bool and write it back on change.
                    bool value = this->guiPopup.boolsBuf[i];
                    if (ImGui::Checkbox(label.c_str(), &value))
                        this->guiPopup.boolsBuf[i] = value;
                    ImGui::PopID();
                }
                if (ImGui::SmallButton("+ Add boolean"))
                    this->guiPopup.boolsBuf.push_back(false);
            }

            ImGui::EndChild();
        }
        else if (!this->guiPopup.selectedType.empty())
        {
            ImGui::Spacing();
            ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 0.6f, 0.4f, 1.0f));
            ImGui::TextWrapped("Unknown type \"%s\" has no editor metadata.\nIt will be kept as-is in the JSON.",
                               this->guiPopup.selectedType.c_str());
            ImGui::PopStyleColor();
        }

        if (!this->guiPopup.error.empty())
        {
            ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 0.5f, 0.5f, 1.0f));
            ImGui::TextWrapped("%s", this->guiPopup.error.c_str());
            ImGui::PopStyleColor();
        }

        ImGui::Separator();
        // No hotkeys while a value is being edited: Enter only saves and
        // Escape only cancels when no text field has focus, so typing a value
        // never triggers the popup action (or loses the entered data).
        bool typing = ImGui::IsAnyItemActive();
        bool saveClicked = ImGui::Button("Save", ImVec2(110.f * scale, 0))
                           || (ImGui::IsKeyPressed(ImGuiKey_Enter)
                               && !this->guiPopup.selectedType.empty() && !typing);
        if (saveClicked)
        {
            if (this->guiPopup.selectedType.empty())
                this->guiPopup.error = "Select a type first.";
            else
                this->commitGuiPopup();
        }

        ImGui::SameLine(0.f, 20.f * scale);
        if (ImGui::Button("Cancel", ImVec2(110.f * scale, 0))
            || (ImGui::IsKeyPressed(ImGuiKey_Escape) && !typing))
        {
            this->guiPopup.open = false;
            this->guiPopup.error.clear();
            ImGui::CloseCurrentPopup();
        }

        ImGui::EndPopup();
    }
    ImGui::PopStyleVar();
}

// ---------------------------------------------------------------------------
// Misc tab: generic editor for the non-trigger top-level fields
// ---------------------------------------------------------------------------
// The trigger file can carry several other sections next to the "trigger"
// array (item-drop, item-drop-list, environments-drop, environments-drop-hit,
// player-characters, config, exit, procedural-*, quest-*, unit-spawn, musics,
// screen-background, ...). The Misc tab edits all of them through a generic
// type-driven form, so any present (or future) field is handled.

void TriggerEditor::renderGuiMiscEditor()
{
    
    float scale = this->imguiScale;

    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.9f, 0.9f, 0.9f, 1.0f));
    ImGui::TextUnformatted("Other fields (item-drop, environments-drop, player-characters, config, exit, ...)");
    ImGui::PopStyleColor();
    ImGui::Separator();

    if (!this->guiRoot.is_object())
    {
        ImGui::TextWrapped("No editable fields (the file could not be parsed).");
        return;
    }

    ImGui::BeginChild("##gui-misc", ImVec2(0, 0), true);
    bool changed = false;

    std::vector<std::string> keys;
    for (auto &item : this->guiRoot.items())
        if (item.key() != "trigger")
            keys.push_back(item.key());

    if (keys.empty())
    {
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.55f, 0.55f, 0.62f, 1.0f));
        ImGui::TextWrapped("This file has no extra fields besides the trigger blocks.");
        ImGui::PopStyleColor();
    }

    for (const auto &key : keys)
    {
        ImGui::PushID(("misc-" + key).c_str());
        const json &value = this->guiRoot[key];
        if (value.is_array() || value.is_object())
        {
            ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.75f, 0.85f, 1.0f, 1.0f));
            ImGui::TextUnformatted(key.c_str());
            ImGui::PopStyleColor();
            ImGui::Separator();
        }
        else
        {
            ImGui::TextUnformatted(key.c_str());
            ImGui::SameLine(200.f * scale);
        }
        changed |= this->guiMiscEditValue(this->guiRoot[key]);
        ImGui::PopID();
    }

    ImGui::EndChild();

    if (changed)
        this->guiSyncText();
}

// Generic value editor: renders an ImGui control matching the JSON type and
// writes changes back to `value`. Returns true when the value changed.
bool TriggerEditor::guiMiscEditValue(json &value)
{
    if (value.is_string())
    {
        std::string text = value.get<std::string>();
        char buf[2048];
        std::strncpy(buf, text.c_str(), sizeof(buf) - 1);
        buf[sizeof(buf) - 1] = '\0';
        ImGui::SetNextItemWidth(-FLT_MIN);
        if (ImGui::InputText("##v", buf, sizeof(buf)))
        {
            value = std::string(buf);
            return true;
        }
        return false;
    }
    if (value.is_boolean())
    {
        bool current = value.get<bool>();
        if (ImGui::Checkbox("##v", &current))
        {
            value = current;
            return true;
        }
        return false;
    }
    if (value.is_number_integer())
    {
        int current = value.get<int>();
        if (ImGui::InputInt("##v", &current))
        {
            value = current;
            return true;
        }
        return false;
    }
    if (value.is_number_float())
    {
        float current = value.get<float>();
        if (ImGui::InputFloat("##v", &current))
        {
            value = current;
            return true;
        }
        return false;
    }
    if (value.is_array())
        return this->guiMiscEditArray(value);
    if (value.is_object())
        return this->guiMiscEditObject(value);
    if (value.is_null())
    {
        if (ImGui::Button("set as string"))
        {
            value = "";
            return true;
        }
        return false;
    }
    return false;
}

bool TriggerEditor::guiMiscEditArray(json &value)
{
    bool changed = false;
    int removeIndex = -1;

    for (int i = 0; i < (int)value.size(); ++i)
    {
        ImGui::PushID(("a" + std::to_string(i)).c_str());
        // The remove button comes FIRST: the row editor (which stretches to
        // the right edge) would otherwise push it off-screen.
        if (ImGui::SmallButton("-"))
            removeIndex = i;
        ImGui::SameLine();

        json &item = value[i];
        bool valueWrapper = item.is_object() && item.size() == 1 && item.contains("value");
        if (valueWrapper)
        {
            // { "value": ... } - the common list shape (drops, unit-spawn, ...).
            ImGui::TextUnformatted("value");
            ImGui::SameLine(70.f * this->imguiScale);
            changed |= this->guiMiscEditValue(item["value"]);
        }
        else
        {
            changed |= this->guiMiscEditValue(item);
        }
        ImGui::PopID();
    }

    if (removeIndex >= 0)
    {
        value.erase(value.begin() + removeIndex);
        changed = true;
    }

    if (ImGui::Button("+ Add"))
    {
        this->guiMiscAddArrayElement(value);
        changed = true;
    }

    return changed;
}

bool TriggerEditor::guiMiscEditObject(json &value)
{
    bool changed = false;
    std::vector<std::string> keys;
    for (auto &item : value.items())
        keys.push_back(item.key());

    for (const auto &key : keys)
    {
        ImGui::PushID(key.c_str());
        if (value[key].is_array() || value[key].is_object())
        {
            ImGui::TextUnformatted(key.c_str());
            ImGui::Indent();
            changed |= this->guiMiscEditValue(value[key]);
            ImGui::Unindent();
        }
        else
        {
            ImGui::TextUnformatted(key.c_str());
            ImGui::SameLine(160.f * this->imguiScale);
            changed |= this->guiMiscEditValue(value[key]);
        }
        ImGui::PopID();
    }

    return changed;
}

void TriggerEditor::guiMiscAddArrayElement(json &value)
{
    if (value.empty())
    {
        value.push_back("");
        return;
    }

    // Clone the shape of the first element so the new row matches the list
    // (e.g. { "value": ... } or { "chance": N, "drops": [...] }).
    std::function<json(const json &)> cloneShape = [&](const json &from) -> json
    {
        if (from.is_string())
            return "";
        if (from.is_boolean())
            return true;
        if (from.is_number_integer())
            return 0;
        if (from.is_number_float())
            return 0.f;
        if (from.is_array())
            return json::array();
        if (from.is_object())
        {
            json out = json::object();
            for (auto &item : from.items())
                out[item.key()] = cloneShape(item.value());
            return out;
        }
        return json();
    };

    const json &first = value[0];
    if (first.is_object())
    {
        json element = json::object();
        for (auto &item : first.items())
            element[item.key()] = cloneShape(item.value());
        value.push_back(element);
    }
    else
    {
        value.push_back(cloneShape(first));
    }
}

void TriggerEditor::renderGuiGroup(GuiKind kind, std::vector<GuiEntry> &entries)
{
    
    float scale = this->imguiScale;

    const char *title = "";
    switch (kind)
    {
        case GuiKind::Event:     title = "Events";     break;
        case GuiKind::Condition: title = "Conditions"; break;
        case GuiKind::Action:    title = "Actions";    break;
    }

    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.9f, 0.9f, 0.9f, 1.0f));
    ImGui::TextUnformatted((std::string(title) + "  (" + toStr((int)entries.size()) + ")").c_str());
    ImGui::PopStyleColor();
    // Compact square "+" button in the corner of each group list (the previous
    // "+Events"/"+Conditions" labels were wider than the right margin, so the
    // button ran past the panel edge and was clipped - you could not see its
    // full extent).
    float addButtonSize = 30.f * scale;
    ImGui::SameLine(ImGui::GetWindowContentRegionMax().x - addButtonSize);
    if (ImGui::Button(("+##add-" + std::string(title)).c_str(), ImVec2(addButtonSize, addButtonSize)))
        this->openGuiPopup(kind, this->guiTriggerIndex, -1);
    if (ImGui::IsItemHovered())
        ImGui::SetTooltip("Add %s", title);

    float listHeight = std::max(GUI_GROUP_MIN_HEIGHT * scale,
                                this->viewportH * GUI_GROUP_VIEWPORT_FRACTION);
    ImGui::BeginChild((std::string("##gui-list-") + title).c_str(), ImVec2(0, listHeight), true);
    if (entries.empty())
    {
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.55f, 0.55f, 0.62f, 1.0f));
        ImGui::TextWrapped("No %s yet.", title);
        ImGui::PopStyleColor();
    }
    else
    {
        for (int i = 0; i < (int)entries.size(); ++i)
        {
            GuiEntry &entry = entries[i];
            ImGui::PushID((title + std::to_string(i)).c_str());

            std::string desc = this->guiDescription(kind, entry.type);
            std::string badge;
            if (kind == GuiKind::Condition && entry.group == 1)
                badge = "  [OR]";
            else if (kind == GuiKind::Action && entry.group == 1)
                badge = "  [ELSE]";

            // Parameter values are drawn in yellow right after the description
            // (via the draw list so the Selectable still spans the whole row
            // for clicks and the context menu). Long summaries are clipped at
            // the list edge; the tooltip shows the full type + values.
            std::string values = this->guiEntryValues(kind, entry);
            ImVec2 rowMin = ImGui::GetCursorScreenPos();
            if (ImGui::Selectable((desc + badge).c_str(), false))
                this->openGuiPopup(kind, this->guiTriggerIndex, i);
            if (!values.empty())
            {
                ImDrawList *drawList = ImGui::GetWindowDrawList();
                float descW = ImGui::CalcTextSize((desc + badge).c_str()).x;
                // Vertically aligned with the Selectable text (which starts at
                // the row top, not at FramePadding.y).
                ImVec2 valuesPos(rowMin.x + descW + 12.f * scale, rowMin.y);
                drawList->AddText(valuesPos, IM_COL32(255, 215, 90, 255), values.c_str());
            }
            if (ImGui::IsItemHovered())
            {
                std::string tooltip = entry.type;
                if (!values.empty())
                    tooltip += "\n" + values;
                ImGui::SetTooltip("%s", tooltip.c_str());
            }

            if (ImGui::BeginPopupContextItem())
            {
                if (ImGui::MenuItem("Move up"))
                    this->guiMoveEntry(entries, i, -1);
                if (ImGui::MenuItem("Move down"))
                    this->guiMoveEntry(entries, i, +1);
                ImGui::Separator();
                if (ImGui::MenuItem("Edit"))
                    this->openGuiPopup(kind, this->guiTriggerIndex, i);
                if (ImGui::MenuItem("Delete"))
                    this->guiDeleteEntry(entries, i);
                ImGui::EndPopup();
            }

            ImGui::PopID();
        }
    }
    ImGui::EndChild();
    ImGui::Separator();
}

void TriggerEditor::renderGuiEditor()
{
    float scale = this->imguiScale;

    if (this->guiTriggers.empty())
        this->guiTriggers.push_back(GuiTrigger());
    if (this->guiTriggerIndex < 0 || this->guiTriggerIndex >= (int)this->guiTriggers.size())
        this->guiTriggerIndex = 0;

    // ---- trigger block selector ----
    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.9f, 0.9f, 0.9f, 1.0f));
    ImGui::TextUnformatted("Trigger block");
    ImGui::PopStyleColor();
    ImGui::SameLine(90.f * scale);
    if (ImGui::BeginCombo("##trigger-block",
                          ("Trigger " + toStr(this->guiTriggerIndex + 1)).c_str()))
    {
        for (int i = 0; i < (int)this->guiTriggers.size(); ++i)
            if (ImGui::Selectable(("Trigger " + toStr(i + 1)).c_str(), i == this->guiTriggerIndex))
                this->guiTriggerIndex = i;
        ImGui::EndCombo();
    }
    ImGui::SameLine();
    if (ImGui::SmallButton("+ Add block"))
    {
        this->guiTriggers.push_back(GuiTrigger());
        this->guiTriggerIndex = (int)this->guiTriggers.size() - 1;
        this->guiSyncText();
    }
    ImGui::SameLine();
    if (this->guiTriggers.size() > 1)
    {
        if (ImGui::SmallButton("- Remove block"))
            this->guiDeleteBlockOpen = true;
    }
    else
    {
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.45f, 0.45f, 0.5f, 1.0f));
        ImGui::SmallButton("- Remove block");
        ImGui::PopStyleColor();
    }

    // ---- header: one square "+" button per group (tooltip on hover). The
    // old "+ Add Event"/"+ Add Condition"/"+ Add Action" labels were wider
    // than the panel on low resolutions and ran past the screen edge. ----
    ImGui::Spacing();
    if (ImGui::Button("+##add-event", ImVec2(30.f * scale, 30.f * scale)))
        this->openGuiPopup(GuiKind::Event, this->guiTriggerIndex, -1);
    if (ImGui::IsItemHovered())
        ImGui::SetTooltip("Add Event");
    ImGui::SameLine();
    if (ImGui::Button("+##add-condition", ImVec2(30.f * scale, 30.f * scale)))
        this->openGuiPopup(GuiKind::Condition, this->guiTriggerIndex, -1);
    if (ImGui::IsItemHovered())
        ImGui::SetTooltip("Add Condition");
    ImGui::SameLine();
    if (ImGui::Button("+##add-action", ImVec2(30.f * scale, 30.f * scale)))
        this->openGuiPopup(GuiKind::Action, this->guiTriggerIndex, -1);
    if (ImGui::IsItemHovered())
        ImGui::SetTooltip("Add Action");

    ImGui::Separator();

    GuiTrigger &trigger = this->guiTriggers[this->guiTriggerIndex];
    this->renderGuiGroup(GuiKind::Event, trigger.events);
    this->renderGuiGroup(GuiKind::Condition, trigger.conditions);
    this->renderGuiGroup(GuiKind::Action, trigger.actions);

    // ---- remove block confirmation ----
    if (this->guiDeleteBlockOpen)
    {
        ImGui::OpenPopup("##trigger-gui-delblock");
        if (ImGui::BeginPopupModal("##trigger-gui-delblock", nullptr, ImGuiWindowFlags_AlwaysAutoResize))
        {
            ImGui::TextUnformatted("Remove trigger block");
            ImGui::Separator();
            ImGui::TextWrapped("This block (%d events, %d conditions, %d actions) will be removed.",
                               (int)trigger.events.size(), (int)trigger.conditions.size(), (int)trigger.actions.size());
            ImGui::Spacing();
            if (ImGui::Button("Remove", ImVec2(100.f * scale, 0)))
            {
                this->guiTriggers.erase(this->guiTriggers.begin() + this->guiTriggerIndex);
                if (this->guiTriggerIndex >= (int)this->guiTriggers.size())
                    this->guiTriggerIndex = (int)this->guiTriggers.size() - 1;
                if (this->guiTriggers.empty())
                    this->guiTriggers.push_back(GuiTrigger());
                this->guiDeleteBlockOpen = false;
                ImGui::CloseCurrentPopup();
                this->guiSyncText();
            }
            ImGui::SameLine(0.f, 20.f * scale);
            if (ImGui::Button("Cancel", ImVec2(100.f * scale, 0))
                || ImGui::IsKeyPressed(ImGuiKey_Escape))
            {
                this->guiDeleteBlockOpen = false;
                ImGui::CloseCurrentPopup();
            }
            ImGui::EndPopup();
        }
    }

    // ---- add/edit entry popup (searchable lookup + parameter editors) ----
    this->renderGuiPopup();
}

// ---------------------------------------------------------------------------
// Map select stage
// ---------------------------------------------------------------------------

void TriggerEditor::renderMapSelect()
{
    
    float scale = this->imguiScale;

    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.9f, 0.9f, 0.9f, 1.0f));
    ImGui::TextUnformatted(this->getLanguage("TITLE").c_str());
    ImGui::PopStyleColor();
    ImGui::SameLine();
    ImGui::SetCursorPosX(ImGui::GetCursorPosX() + 10.f * scale);
    if (ImGui::SmallButton(this->getLanguage("REFRESH").c_str()))
        this->refreshMapList();

    ImGui::SameLine(ImGui::GetWindowContentRegionMax().x - 220.f * scale);
    if (ImGui::Button(this->getLanguage("BACK").c_str(), ImVec2(200.f * scale, 34.f * scale)))
    {
        if (this->performAction("back", ""))
            this->pendingLeave = true;
    }

    ImGui::Separator();

    // ---- map list (left) ----
    ImGui::BeginChild("##trigger-map-list", ImVec2(380.f * scale, 0), true);
    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.9f, 0.9f, 0.9f, 1.0f));
    ImGui::TextUnformatted(this->getLanguage("MAPS").c_str());
    ImGui::PopStyleColor();
    ImGui::Separator();

    if (this->mapList.empty())
    {
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.6f, 0.6f, 0.6f, 1.0f));
        ImGui::TextWrapped("%s", this->getLanguage("NO-MAPS").c_str());
        ImGui::PopStyleColor();
    }
    else
    {
        for (const auto &mapRef : this->mapList)
        {
            bool selected = (mapRef == this->selectedMap);
            if (ImGui::Selectable(mapRef.c_str(), selected))
                this->selectMap(mapRef);
            if (ImGui::IsItemHovered() && ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left))
                this->openMapSession(mapRef);
        }
    }
    ImGui::EndChild();

    ImGui::SameLine();

    // ---- details (right) ----
    ImGui::BeginChild("##trigger-map-info", ImVec2(0, 0), true);
    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.9f, 0.9f, 0.9f, 1.0f));
    ImGui::TextUnformatted(this->getLanguage("SELECT-MAP").c_str());
    ImGui::PopStyleColor();
    ImGui::Separator();

    if (this->selectedMap.empty())
    {
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.6f, 0.6f, 0.6f, 1.0f));
        ImGui::TextWrapped("%s", this->getLanguage("SELECT-MAP-HINT").c_str());
        ImGui::PopStyleColor();
    }
    else
    {
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.7f, 0.8f, 1.0f, 1.0f));
        ImGui::TextWrapped("%s", this->selectedMap.c_str());
        ImGui::PopStyleColor();
        ImGui::Spacing();

        if (ImGui::Button(this->getLanguage("EDIT-TRIGGERS").c_str(), ImVec2(240.f * scale, 40.f * scale)))
            this->openMapSession(this->selectedMap);
    }
    ImGui::EndChild();
}

// ---------------------------------------------------------------------------
// Dialogs
// ---------------------------------------------------------------------------

void TriggerEditor::renderErrorPopup()
{
    if (!this->errorPopupOpen)
        return;

    float scale = this->imguiScale;

    ImGui::PushFont(this->fontNormal);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 6.0f * scale);

    ImGui::OpenPopup("##trigger-editor-error");
    if (ImGui::BeginPopupModal("##trigger-editor-error", nullptr,
                               ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoSavedSettings))
    {
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 0.55f, 0.55f, 1.0f));
        ImGui::TextUnformatted(this->getLanguage("ERROR").c_str());
        ImGui::PopStyleColor();
        ImGui::Separator();

        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.95f, 0.95f, 0.95f, 1.0f));
        for (auto &line : this->errorPopupLines)
            ImGui::TextWrapped("%s", line.c_str());
        ImGui::PopStyleColor();

        ImGui::Separator();
        bool closed = ImGui::Button(this->getLanguage("CLOSE").c_str(), ImVec2(140 * scale, 0))
                      || ImGui::IsKeyPressed(ImGuiKey_Escape);
        if (closed)
        {
            this->errorPopupOpen = false;
            this->errorPopupLines.clear();
            ImGui::CloseCurrentPopup();
        }

        ImGui::EndPopup();
    }

    ImGui::PopStyleVar();
    ImGui::PopFont();
}

// ---------------------------------------------------------------------------
// update / frame
// ---------------------------------------------------------------------------

bool TriggerEditor::update(float timer)
{
    (void)timer;
    bool leaveRequested = false;

    try
    {
        leaveRequested = this->updateImpl();
    }
    catch (const std::exception &e)
    {
        this->showError(std::string("[erro] exceção no Trigger Editor: ") + e.what());
    }
    catch (...)
    {
        this->showError("[erro] exceção desconhecida no Trigger Editor");
    }

    if (this->errorPopupOpen)
        this->renderErrorPopup();

    return leaveRequested;
}

bool TriggerEditor::updateImpl()
{
    bool leaveRequested = false;

    ImVec2 displaySize = ImGui::GetIO().DisplaySize;
    this->viewportW = std::max(1.f, displaySize.x);
    this->viewportH = std::max(1.f, displaySize.y);
    // The UI scale is relative to the game's configured resolution width
    // (designWidth), so the editor looks exactly like it did when Grimsolf
    // forced a 1920x1080 window around it.
    this->imguiScale = std::max(0.5f, std::min(3.0f, this->viewportW / this->designWidth));

    ImGui::PushFont(this->fontNormal);
    ImGui::GetIO().FontGlobalScale = this->imguiScale * GUI_FONT_SCALE;

    ImVec2 windowSize(this->viewportW, this->viewportH);
    ImGui::SetNextWindowSize(windowSize);
    ImGui::SetNextWindowPos(ImVec2(0.f, 0.f), ImGuiCond_Always);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(8 * this->imguiScale, 6 * this->imguiScale));

    ImGui::Begin("##trigger-editor", nullptr,
        ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize |
        ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoSavedSettings);

    ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.08f, 0.08f, 0.11f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 1.0f, 1.0f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(0.3f, 0.3f, 0.35f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.15f, 0.15f, 0.20f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.25f, 0.25f, 0.35f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.30f, 0.30f, 0.40f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(0.12f, 0.12f, 0.16f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_FrameBgHovered, ImVec4(0.20f, 0.20f, 0.26f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_CheckMark, ImVec4(0.50f, 0.85f, 1.0f, 1.0f));

    if (this->stage == Stage::MapSelect)
    {
        this->renderMapSelect();
    }
    else
    {
        // ---- editor layout ----
        ImGui::PushFont(this->fontBig);
        ImGui::TextUnformatted(this->getLanguage("TITLE").c_str());
        ImGui::PopFont();
        ImGui::SameLine(ImGui::GetWindowContentRegionMax().x - 180.f * this->imguiScale);
        if (ImGui::Button(this->getLanguage("BACK").c_str(), ImVec2(160.f * this->imguiScale, 34.f * this->imguiScale)))
            this->requestAction("back", "");
        ImGui::Separator();

        float sidebarWidth = 300.f * this->imguiScale;

        ImGui::BeginChild("##trigger-sidebar", ImVec2(sidebarWidth, 0), true);
        this->renderSidebar();
        ImGui::EndChild();

        ImGui::SameLine();

        // Mouse wheel scrolls the main area in the Visual/Misc tabs. In the
        // JSON tab the code editor widget handles its own wheel, so the panel
        // keeps NoScrollWithMouse there to avoid double-scrolling.
        ImGuiWindowFlags rightPanelFlags = ImGuiWindowFlags_AlwaysVerticalScrollbar;
        if (this->guiTab == GuiTab::Json)
            rightPanelFlags |= ImGuiWindowFlags_NoScrollWithMouse;
        ImGui::BeginChild("##trigger-right", ImVec2(0, 0), false, rightPanelFlags);
        this->renderEditor();
        ImGui::EndChild();
    }

    // ---- create trigger popup ----
    if (this->createPopupOpen)
    {
        ImGui::OpenPopup("##trigger-create");
        if (ImGui::BeginPopupModal("##trigger-create", nullptr, ImGuiWindowFlags_AlwaysAutoResize))
        {
            ImGui::TextUnformatted(this->getLanguage("CREATE-TRIGGER-CAPTION").c_str());
            ImGui::Separator();
            ImGui::TextUnformatted(this->getLanguage("CREATE-TRIGGER-MESSAGE").c_str());
            ImGui::SetNextItemWidth(260.f * this->imguiScale);
            if (ImGui::InputText("##trigger-new-name", this->newFileName, sizeof(this->newFileName),
                                 ImGuiInputTextFlags_EnterReturnsTrue))
                this->createError.clear();

            if (!this->createError.empty())
            {
                ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 0.5f, 0.5f, 1.0f));
                ImGui::TextWrapped("%s", this->createError.c_str());
                ImGui::PopStyleColor();
            }

            ImGui::Spacing();
            if (ImGui::Button(this->getLanguage("CREATE").c_str(), ImVec2(110.f * this->imguiScale, 0))
                || ImGui::IsKeyPressed(ImGuiKey_Enter))
            {
                // Validate inline so an invalid/existing name keeps the popup
                // open with the error message; only close when the file can
                // actually be created (unsaved changes are confirmed through
                // the discard popup by requestAction).
                std::string trimmed = this->newFileName;
                boost::trim(trimmed);
                if (boost::iends_with(trimmed, ".json"))
                    trimmed = trimmed.substr(0, trimmed.size() - 5);
                boost::trim(trimmed);

                bool valid = !trimmed.empty();
                for (char c : trimmed)
                    if (!(std::isalnum((unsigned char)c) != 0 || c == '-' || c == '_'))
                    {
                        valid = false;
                        break;
                    }

                bool exists = !trimmed.empty()
                              && boost::filesystem::exists(this->triggerFolderPath + trimmed + ".json");

                if (!valid)
                    this->createError = this->getLanguage("INVALID-NAME");
                else if (exists)
                    this->createError = this->getLanguage("NAME-EXISTS");
                else
                {
                    this->createError.clear();
                    this->createPopupOpen = false;
                    this->newFileName[0] = '\0';
                    ImGui::CloseCurrentPopup();
                    this->requestAction("create", trimmed);
                }
            }

            ImGui::SameLine(0.f, 20.f * this->imguiScale);
            if (ImGui::Button(this->getLanguage("CANCEL").c_str(), ImVec2(110.f * this->imguiScale, 0))
                || ImGui::IsKeyPressed(ImGuiKey_Escape))
            {
                this->createPopupOpen = false;
                this->createError.clear();
                ImGui::CloseCurrentPopup();
            }

            ImGui::EndPopup();
        }
    }

    // ---- delete trigger popup ----
    if (this->deletePopupOpen)
    {
        ImGui::OpenPopup("##trigger-delete");
        if (ImGui::BeginPopupModal("##trigger-delete", nullptr, ImGuiWindowFlags_AlwaysAutoResize))
        {
            ImGui::TextUnformatted(this->getLanguage("DELETE-TRIGGER-CAPTION").c_str());
            ImGui::Separator();
            ImGui::TextWrapped("%s \"%s\"?", this->getLanguage("DELETE-TRIGGER-MESSAGE").c_str(),
                               this->deleteTarget.c_str());

            if (this->deleteTarget == this->mainTriggerFile)
            {
                ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 0.5f, 0.5f, 1.0f));
                ImGui::TextWrapped("%s", this->getLanguage("DELETE-MAIN-FORBIDDEN").c_str());
                ImGui::PopStyleColor();
            }

            ImGui::Spacing();
            if (ImGui::Button(this->getLanguage("YES").c_str(), ImVec2(100.f * this->imguiScale, 0)))
            {
                this->deletePopupOpen = false;
                ImGui::CloseCurrentPopup();
                this->deleteTriggerFile(this->deleteTarget);
                this->deleteTarget.clear();
            }

            ImGui::SameLine(0.f, 20.f * this->imguiScale);
            if (ImGui::Button(this->getLanguage("NO").c_str(), ImVec2(100.f * this->imguiScale, 0))
                || ImGui::IsKeyPressed(ImGuiKey_Escape))
            {
                this->deletePopupOpen = false;
                this->deleteTarget.clear();
                ImGui::CloseCurrentPopup();
            }

            ImGui::EndPopup();
        }
    }

    // ---- discard (unsaved changes) popup ----
    if (this->discardPopupOpen)
    {
        ImGui::OpenPopup("##trigger-discard");
        if (ImGui::BeginPopupModal("##trigger-discard", nullptr, ImGuiWindowFlags_AlwaysAutoResize))
        {
            ImGui::TextUnformatted(this->getLanguage("DISCARD-CAPTION").c_str());
            ImGui::Separator();
            ImGui::TextWrapped("%s", this->getLanguage("DISCARD-MESSAGE").c_str());

            ImGui::Spacing();
            if (ImGui::Button(this->getLanguage("YES").c_str(), ImVec2(100.f * this->imguiScale, 0)))
            {
                std::string action = this->discardAction;
                std::string target = this->discardTarget;
                this->discardPopupOpen = false;
                this->discardAction.clear();
                this->discardTarget.clear();
                ImGui::CloseCurrentPopup();
                if (this->performAction(action, target))
                    this->pendingLeave = true;
            }

            ImGui::SameLine(0.f, 20.f * this->imguiScale);
            if (ImGui::Button(this->getLanguage("NO").c_str(), ImVec2(100.f * this->imguiScale, 0))
                || ImGui::IsKeyPressed(ImGuiKey_Escape))
            {
                this->discardPopupOpen = false;
                this->discardAction.clear();
                this->discardTarget.clear();
                ImGui::CloseCurrentPopup();
            }

            ImGui::EndPopup();
        }
    }

    ImGui::PopStyleColor(8);
    ImGui::End();
    ImGui::PopStyleVar(3);
    ImGui::PopFont();

    bool leave = this->pendingLeave || leaveRequested;
    this->pendingLeave = false;
    return leave;
}
