#include "triggerEditor.hpp"
#include "../external/imgui/imgui.h"
#include "../external/imgui/imgui-SFML.h"
#include "../grimsolfTheme.hpp"

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

template <typename T> static std::string toStr(const T &value)
{
    return boost::lexical_cast<std::string>(value);
}

static ImU32 colorKey()         { return GrimsolfTheme::u32(0.85f); }
static ImU32 colorString()      { return GrimsolfTheme::u32(0.92f); }
static ImU32 colorNumber()      { return GrimsolfTheme::u32(0.75f); }
static ImU32 colorBoolean()     { return GrimsolfTheme::u32(0.80f); }
static ImU32 colorPunctuation() { return GrimsolfTheme::u32(0.65f); }
static ImU32 colorBroken()      { return GrimsolfTheme::u32(0.58f); }
static ImU32 colorLineNumber()  { return GrimsolfTheme::u32(0.42f); }
static ImU32 colorSelection()   { return GrimsolfTheme::u32(0.50f, 0.35f); }
static ImU32 colorCaret()       { return GrimsolfTheme::textU32(); }

static ImU32 colorKeyword(float alpha = 1.0f)
{
    const ImVec4 gold = GrimsolfTheme::isLight()
        ? ImVec4(0.55f, 0.40f, 0.00f, alpha)
        : ImVec4(1.00f, 0.84f, 0.35f, alpha);
    return ImGui::ColorConvertFloat4ToU32(gold);
}

static const int INDENT = 2;

static const int HISTORY_LIMIT = 200;

static const float GUI_FONT_SCALE = 0.6f;

static const float GUI_POPUP_MAX_WIDTH_FRACTION = 0.6f;
static const float GUI_POPUP_MIN_WIDTH = 640.f;
static const float GUI_POPUP_MIN_HEIGHT = 480.f;
static const float GUI_TYPE_LIST_MAX_ROWS = 6.f;
static const float GUI_TYPE_LIST_MIN_ROWS = 3.f;
static const float GUI_PARAMS_MIN_ROWS = 5.f;
static const float GUI_PARAMS_MIN_PX = 150.f;

struct ExtraSourceDef
{
    const char *folder;
    const char *label;
};

static const ExtraSourceDef EXTRA_SOURCES[] = {
    { "interactions", "Interactions" },
    { "cutscenes",    "Cutscenes" }
};


static bool isIdentChar(char c)
{
    return std::isalnum((unsigned char)c) != 0 || c == '_' || c == '-' || c == '.';
}

enum class YamlTokenType
{
    Key,
    String,
    Number,
    Boolean,
    Null,
    Punctuation,
    Comment,
    Other
};

struct YamlToken
{
    int start;
    int end;
    YamlTokenType type;
};

static std::vector<YamlToken> tokenizeYaml(const std::string &text)
{
    std::vector<YamlToken> tokens;
    const int n = (int)text.size();

    int i = 0;
    while (i < n)
    {
        int lineStart = i;
        int lineEnd = lineStart;
        while (lineEnd < n && text[lineEnd] != '\n')
            ++lineEnd;

        int cursor = lineStart;
        while (cursor < lineEnd && (text[cursor] == ' ' || text[cursor] == '\t'))
            ++cursor;

        bool lineIsItem = false;
        while (cursor < lineEnd && text[cursor] == '-')
        {
            if (cursor + 1 >= lineEnd || text[cursor + 1] == ' ' || text[cursor + 1] == '\t')
            {
                tokens.push_back(YamlToken{ cursor, cursor + 1, YamlTokenType::Punctuation });
                lineIsItem = true;
                ++cursor;
                while (cursor < lineEnd && (text[cursor] == ' ' || text[cursor] == '\t'))
                    ++cursor;
            }
            else
            {
                break;
            }
        }

        if (lineIsItem && cursor >= lineEnd)
        {
            i = lineEnd + 1;
            continue;
        }

        int keyStart = cursor;
        if (cursor < lineEnd && text[cursor] != '#' && text[cursor] != ':')
        {
            while (cursor < lineEnd && text[cursor] != ':' && text[cursor] != ' ' && text[cursor] != '\t')
                ++cursor;
            int keyEnd = cursor;
            while (cursor < lineEnd && (text[cursor] == ' ' || text[cursor] == '\t'))
                ++cursor;

            if (keyEnd > keyStart && cursor < lineEnd && text[cursor] == ':')
            {
                tokens.push_back(YamlToken{ keyStart, keyEnd, YamlTokenType::Key });
                tokens.push_back(YamlToken{ cursor, cursor + 1, YamlTokenType::Punctuation });
                ++cursor;
                while (cursor < lineEnd && (text[cursor] == ' ' || text[cursor] == '\t'))
                    ++cursor;
            }
            else
            {
                cursor = keyStart;
            }
        }

        bool inComment = false;
        while (cursor < lineEnd)
        {
            char c = text[cursor];

            if (inComment)
            {
                ++cursor;
                continue;
            }

            if (c == '#')
            {
                inComment = true;
                int end = cursor;
                while (end < lineEnd)
                    ++end;
                tokens.push_back(YamlToken{ cursor, end, YamlTokenType::Comment });
                cursor = end;
                continue;
            }

            if (c == '"' || c == '\'')
            {
                char quote = c;
                int start = cursor;
                ++cursor;
                while (cursor < lineEnd)
                {
                    if (quote == '"' && text[cursor] == '\\')
                    {
                        cursor += 2;
                        continue;
                    }
                    if (text[cursor] == quote)
                    {
                        ++cursor;
                        break;
                    }
                    ++cursor;
                }
                tokens.push_back(YamlToken{ start, std::min(cursor, lineEnd), YamlTokenType::String });
                continue;
            }

            if (c == '-' || (c >= '0' && c <= '9'))
            {
                int start = cursor;
                ++cursor;
                while (cursor < lineEnd)
                {
                    char d = text[cursor];
                    if ((d >= '0' && d <= '9') || d == '.' || d == 'e' || d == 'E' || d == '+' || d == '-')
                        ++cursor;
                    else
                        break;
                }
                tokens.push_back(YamlToken{ start, cursor, YamlTokenType::Number });
                continue;
            }

            if (c == '{' || c == '}' || c == '[' || c == ']' || c == ',' || c == '*'
                || c == '&' || c == '!' || c == '|' || c == '>' || c == '%')
            {
                tokens.push_back(YamlToken{ cursor, cursor + 1, YamlTokenType::Punctuation });
                ++cursor;
                continue;
            }

            if (std::isalpha((unsigned char)c) || c == '_')
            {
                int start = cursor;
                while (cursor < lineEnd && isIdentChar(text[cursor]))
                    ++cursor;

                std::string word = text.substr(start, cursor - start);
                YamlTokenType type = YamlTokenType::Other;
                if (word == "true" || word == "false")
                    type = YamlTokenType::Boolean;
                else if (word == "null" || word == "~")
                    type = YamlTokenType::Null;
                tokens.push_back(YamlToken{ start, cursor, type });
                continue;
            }

            ++cursor;
        }

        i = lineEnd + 1;
    }

    return tokens;
}

static ImU32 yamlTokenColor(YamlTokenType type)
{
    switch (type)
    {
        case YamlTokenType::Key:         return colorKey();
        case YamlTokenType::String:      return colorString();
        case YamlTokenType::Number:      return colorNumber();
        case YamlTokenType::Boolean:     return colorBoolean();
        case YamlTokenType::Null:        return colorBoolean();
        case YamlTokenType::Punctuation: return colorPunctuation();
        case YamlTokenType::Comment:     return colorLineNumber();
        default:                         return colorBroken();
    }
}


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


TriggerEditor::TriggerEditor(const std::string &gamePath, ImFont *fontNormal, ImFont *fontBig)
{
    this->gamePath = gamePath;
    this->fontNormal = fontNormal;
    this->fontBig = fontBig;

    json options = this->loadJsonFile(gamePath + "/data/options/options.json");
    if (options.is_object())
        this->designWidth = (float)options.value("video-resolution-width", 1920);
    if (this->designWidth <= 0.f)
        this->designWidth = 1920.f;

    this->refreshMapList();
    this->refreshSources();
}

TriggerEditor::~TriggerEditor()
{
    this->logLines.clear();
    this->mapList.clear();
    this->sources.clear();
    this->undoStack.clear();
    this->redoStack.clear();
}

void TriggerEditor::addLog(const std::string &text, unsigned int color)
{
    const unsigned int themed = ImGui::ColorConvertFloat4ToU32(
        GrimsolfTheme::toGray(ImGui::ColorConvertU32ToFloat4(color)));
    this->logLines.push_back(LogLine{ text, themed });
    if (this->logLines.size() > 500)
        this->logLines.erase(this->logLines.begin());
    this->logDirty = true;
}

void TriggerEditor::showError(const std::string &text)
{
    this->addLog(text, 0xFF6060FF);
    this->errorPopupLines.push_back(text);
    if (this->errorPopupLines.size() > 20)
        this->errorPopupLines.erase(this->errorPopupLines.begin());
    this->errorPopupOpen = true;
}


json TriggerEditor::loadJsonFile(const std::string &path) const
{
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

            if (rel.find("/trigger/") != std::string::npos || rel.rfind("trigger/", 0) == 0)
                continue;

            if (rel.size() > 5)
                rel = rel.substr(0, rel.size() - 5);

            this->mapList.push_back(rel);
        }
    }
    catch (...)
    {
    }

    std::sort(this->mapList.begin(), this->mapList.end());
}

void TriggerEditor::selectMap(const std::string &ref)
{
    this->selectedMap = ref;
    this->mapPath = this->gamePath + "/data/maps/" + ref + ".json";
}

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
        this->showError("[error] Failed to load: " + this->mapPath);
        return false;
    }

    std::string scriptRef = this->resolveMapScript(mapJson);
    bool created = false;

    if (scriptRef.empty())
    {
        scriptRef = this->ensureTriggerLink(mapJson, ref);
        if (scriptRef.empty())
        {
            this->showError("[error] could not link the map to its trigger file: " + this->mapPath);
            return false;
        }
        created = true;
    }

    size_t slash = scriptRef.find_last_of('/');
    this->triggerFolderRef = (slash == std::string::npos) ? "" : scriptRef.substr(0, slash);
    std::string mainStem = (slash == std::string::npos) ? scriptRef : scriptRef.substr(slash + 1);
    this->mainTriggerFile = mainStem + ".yaml";
    this->triggerFolderPath = this->gamePath + "/data/maps/"
                              + (this->triggerFolderRef.empty() ? "" : this->triggerFolderRef + "/");

    std::string mainPath = this->triggerFolderPath + this->mainTriggerFile;
    if (!boost::filesystem::exists(mainPath))
    {
        if (!this->saveTextFile(mainPath, this->defaultTriggerTemplate()))
        {
            this->showError("[error] Failed to create: " + mainPath);
            return false;
        }
        created = true;
    }

    if (created)
        this->addLog("[info] map linked to trigger: " + scriptRef, 0x88CCFFFF);

    this->sourceIndex = 0;
    this->refreshSources();
    this->openTriggerFile(this->mainTriggerFile);
    this->stage = Stage::Editor;
    this->addLog("[info] Map: " + ref, 0x88CCFFFF);
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
    this->sources.clear();
    this->sourceIndex = 0;
    this->currentFileSource = -1;
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
    this->yamlValid = true;
    this->guiTab = GuiTab::Visual;
    this->guiTriggers.clear();
    this->guiRoot = json();
    this->guiTriggerIndex = 0;
    this->guiConvertible = true;
    this->guiConvertError.clear();
    this->guiPopup = GuiPopupState();
    this->guiCommentPopupOpen = false;
    this->guiCommentEntryIndex = -1;
    this->guiCommentInsertIndex = -1;
    this->guiDeleteBlockOpen = false;

    this->refreshSources();
}


void TriggerEditor::refreshSources()
{
    this->sources.clear();

    try
    {
        if (!this->triggerFolderPath.empty())
        {
            BrowseSource source;
            source.label = "Triggers";
            source.ref = "data/maps/" + this->triggerFolderRef;
            source.folder = this->triggerFolderPath;
            source.editable = true;
            this->sources.push_back(source);
        }

        for (const ExtraSourceDef &def : EXTRA_SOURCES)
        {
            std::string ref = std::string("data/") + def.folder;
            std::string folder = this->gamePath + "/" + ref + "/";
            if (!boost::filesystem::is_directory(folder))
                continue;

            BrowseSource source;
            source.label = def.label;
            source.ref = ref;
            source.folder = folder;
            source.editable = false;
            this->sources.push_back(source);
        }
    }
    catch (...)
    {
    }

    if (this->sourceIndex < 0 || this->sourceIndex >= (int)this->sources.size())
        this->sourceIndex = 0;

    for (int index = 0; index < (int)this->sources.size(); ++index)
        this->refreshSourceFiles(index);
}

void TriggerEditor::refreshSourceFiles(int index)
{
    if (index < 0 || index >= (int)this->sources.size())
        return;

    BrowseSource &source = this->sources[index];
    source.files.clear();

    try
    {
        boost::filesystem::path dir = source.folder;
        if (!boost::filesystem::is_directory(dir))
            return;

        for (boost::filesystem::recursive_directory_iterator it(dir), end; it != end; ++it)
        {
            if (!boost::filesystem::is_regular_file(it->status()))
                continue;
            if (it->path().extension() != ".yaml")
                continue;

            std::string rel = it->path().lexically_relative(dir).string();
            boost::replace_all(rel, "\\", "/");
            source.files.push_back(rel);
        }
    }
    catch (...)
    {
    }

    std::sort(source.files.begin(), source.files.end());
}

bool TriggerEditor::sourceIsEditable(int index) const
{
    return index >= 0 && index < (int)this->sources.size() && this->sources[index].editable;
}

void TriggerEditor::switchSource(int index)
{
    if (index < 0 || index >= (int)this->sources.size() || index == this->sourceIndex)
        return;

    this->sourceIndex = index;
    this->refreshSourceFiles(index);
    this->addLog("[info] Folder: " + this->sources[index].ref, 0x88CCFFFF);
}

void TriggerEditor::openFileInSource(int index, const std::string &filename)
{
    if (index < 0 || index >= (int)this->sources.size())
        return;

    this->sourceIndex = index;
    this->refreshSourceFiles(index);
    this->openTriggerFile(filename);
}

void TriggerEditor::openSourceSession(int index, const std::string &filename)
{
    if (index < 0 || index >= (int)this->sources.size())
        return;

    this->selectedMap.clear();
    this->mapPath.clear();
    this->triggerFolderRef.clear();
    this->triggerFolderPath.clear();
    this->mainTriggerFile.clear();
    this->stage = Stage::Editor;
    this->openFileInSource(index, filename);
}


void TriggerEditor::openTriggerFile(const std::string &filename)
{
    if (this->sourceIndex < 0 || this->sourceIndex >= (int)this->sources.size())
        return;

    std::string path = this->sources[this->sourceIndex].folder + filename;
    if (!boost::filesystem::exists(path))
    {
        this->showError("[error] Failed to load: " + path);
        return;
    }

    this->text = this->readTextFile(path);
    this->currentFile = filename;
    this->currentFileSource = this->sourceIndex;
    this->currentFilePath = path;
    this->fileLoaded = true;
    this->dirty = false;
    this->cursorPos = 0;
    this->selectionStart = -1;
    this->undoStack.clear();
    this->redoStack.clear();
    this->validateYaml();
    this->openTriggerFileGui();
    this->logSyntaxWarnings();
    this->addLog("[info] Opened: " + path, 0x88CCFFFF);
}

void TriggerEditor::openTriggerFileGui()
{
    this->guiTriggers.clear();
    this->guiTriggerIndex = 0;
    this->guiRoot = json::object();
    this->guiDeleteBlockOpen = false;
    this->guiPopup.open = false;
    this->guiPopup.error.clear();

    if (this->textToGui())
    {
        this->guiTab = GuiTab::Visual;
        this->guiConvertible = true;
    }
    else
    {
        this->guiTab = GuiTab::Json;
        this->guiConvertible = false;
        this->addLog("[error] " + this->guiConvertError, 0xFF6060FF);
    }
}

void TriggerEditor::createTriggerFile(const std::string &name)
{
    if (!this->sourceIsEditable(this->sourceIndex))
    {
        this->showError("[error] Only files inside the map's trigger folder can be created or deleted.");
        return;
    }

    std::string trimmed = name;
    boost::trim(trimmed);

    if (boost::iends_with(trimmed, ".yaml"))
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
        this->createError = "Invalid name. Use only letters, numbers, \"-\" and \"_\".";
        return;
    }

    std::string filename = trimmed + ".yaml";
    std::string path = this->sources[this->sourceIndex].folder + filename;

    if (boost::filesystem::exists(path))
    {
        this->createError = "A file with this name already exists in this folder.";
        return;
    }

    if (!this->saveTextFile(path, this->defaultTriggerTemplate()))
    {
        this->showError("[error] Failed to create: " + path);
        return;
    }

    this->refreshSourceFiles(this->sourceIndex);
    this->addLog("[ok] Created: " + path, 0x55FF88FF);
    this->openTriggerFile(filename);
}

void TriggerEditor::deleteTriggerFile(const std::string &filename)
{
    if (!this->sourceIsEditable(this->currentFileSource))
    {
        this->showError("[error] Only files inside the map's trigger folder can be created or deleted.");
        return;
    }

    if (filename == this->mainTriggerFile)
    {
        this->showError("[error] The main trigger file of the map cannot be deleted.");
        return;
    }

    std::string path = this->sources[this->currentFileSource].folder + filename;
    if (!boost::filesystem::exists(path))
        return;

    if (!boost::filesystem::remove(path))
    {
        this->showError("[error] Failed to delete: " + path);
        return;
    }

    int source = this->currentFileSource;
    if (path == this->currentFilePath)
    {
        this->currentFile.clear();
        this->currentFileSource = -1;
        this->currentFilePath.clear();
        this->text.clear();
        this->fileLoaded = false;
        this->dirty = false;
        this->cursorPos = 0;
        this->selectionStart = -1;
        this->undoStack.clear();
        this->redoStack.clear();
    }

    this->refreshSourceFiles(source);
    this->addLog("[ok] Deleted: " + path, 0x55FF88FF);
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
        this->showError("[error] Failed to save: " + this->currentFilePath);
        return false;
    }

    this->dirty = false;
    this->addLog("[ok] Saved: " + this->currentFilePath, 0x55FF88FF);
    if (!this->yamlValid)
        this->addLog("[error] Invalid JSON: " + this->parseError, 0xFF6060FF);
    this->logSyntaxWarnings();
    return true;
}

std::string TriggerEditor::defaultTriggerTemplate() const
{
    return R"YAML(environments-drop: []
item-drop: []
item-drop-list: []

trigger:
  - events:
      - event: "initialization"
    conditions: []
    then: []
)YAML";
}


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


namespace
{
    size_t editDistance(const std::string &a, const std::string &b)
    {
        std::vector<size_t> row(b.size() + 1);
        for (size_t j = 0; j <= b.size(); ++j)
            row[j] = j;

        for (size_t i = 1; i <= a.size(); ++i)
        {
            size_t previous = row[0];
            row[0] = i;
            for (size_t j = 1; j <= b.size(); ++j)
            {
                size_t current = row[j];
                size_t cost = (a[i - 1] == b[j - 1]) ? 0 : 1;
                row[j] = std::min({ row[j] + 1, row[j - 1] + 1, previous + cost });
                previous = current;
            }
        }

        return row[b.size()];
    }

    std::string closestName(const std::string &value, const std::vector<std::string> &names)
    {
        std::string best;
        size_t bestDistance = 0;
        for (const auto &name : names)
        {
            size_t distance = editDistance(value, name);
            if (best.empty() || distance < bestDistance)
            {
                best = name;
                bestDistance = distance;
            }
        }

        size_t limit = std::max<size_t>(2, value.size() / 3);
        return (!best.empty() && bestDistance <= limit) ? best : "";
    }

    std::string suggestionSuffix(const std::string &value, const std::vector<std::string> &names)
    {
        std::string hint = closestName(value, names);
        return hint.empty() ? "" : ". Did you mean \"" + hint + "\"?";
    }

    std::vector<std::string> keysOf(const std::vector<TriggerTypeDef> &defs)
    {
        std::vector<std::string> keys;
        for (const auto &def : defs)
            keys.push_back(def.key);
        return keys;
    }
}

void TriggerEditor::checkTriggerSyntax(const json &root)
{
    this->syntaxWarnings.clear();

    const auto warn = [this](const std::string &message)
    {
        this->syntaxWarnings.push_back(message);
    };

    if (!root.is_object())
    {
        warn("The document must be a YAML mapping (key: value) at the top level.");
        return;
    }

    if (!root.contains("trigger") || root["trigger"].is_null())
    {
        warn("Missing top-level \"trigger\" list. Add: trigger: []");
        return;
    }

    if (!root["trigger"].is_array())
    {
        warn("\"trigger\" must be a list: each block starts with \"- \" under it.");
        return;
    }

    const std::vector<std::string> blockKeys = { "events", "conditions", "conditions-or", "then", "else", "name" };
    const std::vector<std::string> fieldNames = { "floatValue", "integerValue", "unitName", "text",
                                                  "strings", "integers", "floats", "booleans" };
    const std::vector<std::string> listFields = { "strings", "integers", "floats", "booleans" };

    struct ListRule
    {
        const char *key;
        const char *typeKey;
        const char *kind;
        const std::vector<TriggerTypeDef> *defs;
    };

    const ListRule rules[] = {
        { "events", "event", "event", &GUI_EVENT_DEFS },
        { "conditions", "condition", "condition", &GUI_CONDITION_DEFS },
        { "conditions-or", "condition", "condition", &GUI_CONDITION_DEFS },
        { "then", "action", "action", &GUI_ACTION_DEFS },
        { "else", "action", "action", &GUI_ACTION_DEFS }
    };

    int blockIndex = 0;
    for (const auto &block : root["trigger"])
    {
        ++blockIndex;
        const std::string where = "trigger block " + std::to_string(blockIndex);

        if (!block.is_object())
        {
            warn(where + ": each block must be a mapping with events / conditions / then.");
            continue;
        }

        for (auto it = block.begin(); it != block.end(); ++it)
            if (std::find(blockKeys.begin(), blockKeys.end(), it.key()) == blockKeys.end())
                warn(where + ": unknown key \"" + it.key() + "\"" + suggestionSuffix(it.key(), blockKeys));

        for (const auto &rule : rules)
        {
            if (!block.contains(rule.key) || block[rule.key].is_null())
                continue;

            if (!block[rule.key].is_array())
            {
                warn(where + ": \"" + rule.key + "\" must be a list (\"- \" items).");
                continue;
            }

            const std::vector<std::string> knownNames = keysOf(*rule.defs);
            int entryIndex = 0;
            for (const auto &entry : block[rule.key])
            {
                ++entryIndex;
                const std::string entryWhere = where + ", " + rule.key + " item " + std::to_string(entryIndex);

                if (!entry.is_object())
                {
                    warn(entryWhere + ": must be a mapping with a \"" + rule.typeKey + "\" key.");
                    continue;
                }

                if (!entry.contains(rule.typeKey) || !entry[rule.typeKey].is_string())
                {
                    warn(entryWhere + ": missing \"" + rule.typeKey + "\". Example: "
                         + rule.typeKey + ": \"" + (knownNames.empty() ? "" : knownNames.front()) + "\"");
                    continue;
                }

                const std::string name = entry[rule.typeKey].get<std::string>();
                if (guiFindDef(*rule.defs, name) == nullptr)
                    warn(entryWhere + ": unknown " + rule.kind + " \"" + name + "\""
                         + suggestionSuffix(name, knownNames));

                for (auto field = entry.begin(); field != entry.end(); ++field)
                {
                    const std::string key = field.key();
                    if (key == rule.typeKey)
                        continue;

                    if (std::find(fieldNames.begin(), fieldNames.end(), key) == fieldNames.end())
                        warn(entryWhere + ": unknown field \"" + key + "\"" + suggestionSuffix(key, fieldNames));
                }

                for (const auto &listField : listFields)
                {
                    if (!entry.contains(listField) || entry[listField].is_null())
                        continue;

                    if (!entry[listField].is_array())
                    {
                        warn(entryWhere + ": \"" + listField + "\" must be a list of { value: ... } items.");
                        continue;
                    }

                    for (const auto &item : entry[listField])
                        if (!item.is_object() || !item.contains("value"))
                            warn(entryWhere + ": every item of \"" + listField + "\" needs a value. Example: - value: 0");
                }
            }
        }
    }
}

void TriggerEditor::logSyntaxWarnings()
{
    for (const auto &warning : this->syntaxWarnings)
        this->addLog("[warn] " + warning, 0xFFCC66FF);
}

void TriggerEditor::validateYaml()
{
    this->parseError.clear();
    this->parseErrorLine = 0;
    this->parseErrorCol = 0;
    this->syntaxWarnings.clear();

    if (this->text.empty())
    {
        this->yamlValid = true;
        return;
    }

    try
    {
        json parsed = Json::parseYaml(this->text);
        this->yamlValid = true;
        this->checkTriggerSyntax(parsed);
    }
    catch (const std::exception &e)
    {
        this->yamlValid = false;
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
        this->yamlValid = false;
        this->parseError = "parse error";
    }
}

void TriggerEditor::formatYaml()
{
    this->validateYaml();
    if (!this->yamlValid)
    {
        this->showError("[error] Invalid JSON: " + this->parseError);
        return;
    }

    try
    {
        this->snapshotUndo();
        if (this->textToGui())
            this->guiToText();
        else
        {
            json parsed = Json::parseYaml(this->text);
            this->text = Json::dumpYaml(parsed);
        }
        this->cursorPos = 0;
        this->selectionStart = -1;
        this->dirty = true;
        this->validateYaml();
        this->addLog("[info] JSON formatted", 0x88CCFFFF);
    }
    catch (...)
    {
        this->showError("[error] Invalid JSON");
    }
}


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

    if (!this->editorFocused)
    {
        if (!this->createPopupOpen && !this->deletePopupOpen
            && !this->discardPopupOpen && !this->errorPopupOpen)
            io.InputQueueCharacters.resize(0);
        return;
    }

    io.InputQueueCharacters.resize(0);

    bool ctrl = io.KeyCtrl;
    bool shift = io.KeyShift;

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

    if (ctrl && ImGui::IsKeyPressed(ImGui::GetKeyIndex(ImGuiKey_A)))
    {
        this->selectionStart = 0;
        this->cursorPos = (int)this->text.size();
        this->caretTimer = 0.f;
    }
    else if (ctrl && ImGui::IsKeyPressed(ImGui::GetKeyIndex(ImGuiKey_C)))
    {
        ImGui::SetClipboardText(this->getSelectedText().c_str());
    }
    else if (ImGui::IsKeyPressed(ImGui::GetKeyIndex(ImGuiKey_LeftArrow), true))
    {
        int pos = ctrl ? wordLeft(this->text, this->cursorPos) : this->utf8Back(this->cursorPos);
        moveCursor(pos);
    }
    else if (ImGui::IsKeyPressed(ImGui::GetKeyIndex(ImGuiKey_RightArrow), true))
    {
        int pos = ctrl ? wordRight(this->text, this->cursorPos) : this->utf8Advance(this->cursorPos);
        moveCursor(pos);
    }
    else if (ImGui::IsKeyPressed(ImGui::GetKeyIndex(ImGuiKey_UpArrow), true))
    {
        int line = this->lineOfOffset(this->cursorPos);
        int column = this->utf8Columns(this->cursorPos);
        int newLine = std::max(0, line - 1);
        moveCursor(this->visualToByte(newLine, column));
    }
    else if (ImGui::IsKeyPressed(ImGui::GetKeyIndex(ImGuiKey_DownArrow), true))
    {
        int line = this->lineOfOffset(this->cursorPos);
        int column = this->utf8Columns(this->cursorPos);
        int newLine = std::min(this->countLines() - 1, line + 1);
        moveCursor(this->visualToByte(newLine, column));
    }
    else if (ImGui::IsKeyPressed(ImGui::GetKeyIndex(ImGuiKey_Home), true))
    {
        int line = this->lineOfOffset(this->cursorPos);
        int target = ctrl ? 0 : this->lineStartOffset(line);
        moveCursor(target);
    }
    else if (ImGui::IsKeyPressed(ImGui::GetKeyIndex(ImGuiKey_End), true))
    {
        int line = this->lineOfOffset(this->cursorPos);
        int target = ctrl ? (int)this->text.size() : this->lineEndOffset(line);
        moveCursor(target);
    }
    else if (ImGui::IsKeyPressed(ImGui::GetKeyIndex(ImGuiKey_PageUp), true))
    {
        int line = this->lineOfOffset(this->cursorPos);
        int column = this->utf8Columns(this->cursorPos);
        int newLine = std::max(0, line - std::max(1, this->editorVisibleLines - 1));
        moveCursor(this->visualToByte(newLine, column));
    }
    else if (ImGui::IsKeyPressed(ImGui::GetKeyIndex(ImGuiKey_PageDown), true))
    {
        int line = this->lineOfOffset(this->cursorPos);
        int column = this->utf8Columns(this->cursorPos);
        int newLine = std::min(this->countLines() - 1, line + std::max(1, this->editorVisibleLines - 1));
        moveCursor(this->visualToByte(newLine, column));
    }
    else if (ImGui::IsKeyPressed(ImGui::GetKeyIndex(ImGuiKey_Escape)))
    {
        this->selectionStart = -1;
    }
}

void TriggerEditor::handleEditorMouse(ImVec2 base)
{
    ImGuiIO &io = ImGui::GetIO();

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

    std::vector<YamlToken> tokens = tokenizeYaml(this->text);

    for (int line = firstLine; line <= lastLine; ++line)
    {
        int ls = this->lineStartOffset(line);
        int le = this->lineEndOffset(line);
        float y = base.y - this->scrollY + (float)line * this->editorLineH;

        char number[16];
        std::snprintf(number, sizeof(number), "%d", line + 1);
        drawList->AddText(ImGui::GetFont(), ImGui::GetFontSize(),
                          ImVec2(base.x + 4.f, y), colorLineNumber(), number);

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
                              yamlTokenColor(token.type), this->text.c_str() + s, this->text.c_str() + e);
        }
    }

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

            drawList->AddRectFilled(ImVec2(x1, y), ImVec2(x2, y + this->editorLineH), colorSelection());
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
                            colorCaret());
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

    if (this->yamlValid)
    {
        ImGui::PushStyleColor(ImGuiCol_Text, GrimsolfTheme::gray(0.85f));
        ImGui::TextUnformatted("Valid JSON");
        ImGui::PopStyleColor();
    }
    else
    {
        ImGui::PushStyleColor(ImGuiCol_Text, GrimsolfTheme::gray(0.62f));
        if (this->parseErrorLine > 0)
            ImGui::TextUnformatted(("Invalid JSON (Line " + toStr(this->parseErrorLine)
                                    + ", Col " + toStr(this->parseErrorCol) + ")")
                                       .c_str());
        else
            ImGui::TextUnformatted("Invalid JSON");
        ImGui::PopStyleColor();
    }

    if (!this->syntaxWarnings.empty())
    {
        ImGui::SameLine(0.f, 24.f * scale);
        ImGui::PushStyleColor(ImGuiCol_Text, GrimsolfTheme::gray(0.80f));
        ImGui::TextUnformatted((std::to_string(this->syntaxWarnings.size()) + " warning(s) - see log").c_str());
        ImGui::PopStyleColor();
    }

    int line = this->lineOfOffset(this->cursorPos) + 1;
    int column = this->utf8Columns(this->cursorPos) + 1;
    std::string status = "Line " + toStr(line)
                         + "  Col " + toStr(column);

    float rightEdge = ImGui::GetWindowWidth();
    if (this->dirty)
    {
        ImGui::SameLine();
        ImGui::SetCursorPosX(rightEdge - 300.f * scale);
        ImGui::PushStyleColor(ImGuiCol_Text, GrimsolfTheme::gray(0.85f));
        ImGui::TextUnformatted("[!] Unsaved changes");
        ImGui::PopStyleColor();
        ImGui::SameLine();
        ImGui::SetCursorPosX(rightEdge - 150.f * scale);
    }
    else
    {
        ImGui::SameLine();
        ImGui::SetCursorPosX(rightEdge - 150.f * scale);
    }

    ImGui::PushStyleColor(ImGuiCol_Text, GrimsolfTheme::textColor());
    ImGui::TextUnformatted(status.c_str());
    ImGui::PopStyleColor();
}

void TriggerEditor::renderYamlBody()
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

    ImGui::PushStyleColor(ImGuiCol_ChildBg, GrimsolfTheme::gray(0.04f));
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.f, 0.f));
    ImGui::BeginChild("##trigger-editor-code", ImVec2(0, -150.f * this->imguiScale), true,
                      ImGuiWindowFlags_NoScrollWithMouse);
    ImGui::PopStyleVar();
    ImGui::PopStyleColor();

    ImVec2 base = ImGui::GetCursorScreenPos();
    ImVec2 viewport = ImGui::GetContentRegionAvail();
    this->editorViewW = std::max(1.f, viewport.x);
    this->editorViewH = std::max(1.f, viewport.y);
    this->editorVisibleLines = std::max(1, (int)std::ceil(this->editorViewH / this->editorLineH) + 1);

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

    ImGui::SetCursorScreenPos(base);
    ImGui::InvisibleButton("##trigger-editor-input", viewport);
    ImGui::SetItemAllowOverlap();

    this->editorFocused = ImGui::IsItemActive() || ImGui::IsItemFocused();

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
        ImGui::PushStyleColor(ImGuiCol_Text, GrimsolfTheme::textColor());
        ImGui::SetCursorScreenPos(ImVec2(base.x + this->editorGutterW + 8.f * this->imguiScale,
                                          base.y + 10.f * this->imguiScale));
        ImGui::TextUnformatted("No trigger file open");
        ImGui::PopStyleColor();
    }

    ImGui::EndChild();
}

void TriggerEditor::renderEditor()
{
    float scale = this->imguiScale;

    ImGui::PushStyleColor(ImGuiCol_Text, GrimsolfTheme::textColor());
    if (this->fileLoaded)
        ImGui::TextUnformatted((this->currentFile + "  (" + this->currentFilePath + ")").c_str());
    else
        ImGui::TextUnformatted("No trigger file open");
    ImGui::PopStyleColor();
    ImGui::Separator();

    this->renderToolbar();
    ImGui::Separator();

    ImGui::BeginTabBar("##trigger-tabs");

    const auto switchTab = [&](GuiTab target, bool needsConversion)
    {
        ImGuiTabItemFlags flags = (this->guiTab == target) ? ImGuiTabItemFlags_SetSelected : 0;
        bool tab = ImGui::BeginTabItem((target == GuiTab::Visual) ? "GUI"
                                        : (target == GuiTab::Misc) ? "Misc" : "YAML",
                                       nullptr, flags);
        bool clicked = ImGui::IsItemClicked();
        if (clicked && this->guiTab != target)
        {
            if (!needsConversion || this->textToGui())
                this->guiTab = target;
            else
            {
                this->guiConvertible = false;
                this->showError("[error] " + this->guiConvertError);
            }
        }
        if (tab)
            ImGui::EndTabItem();
    };

    switchTab(GuiTab::Visual, true);
    switchTab(GuiTab::Misc, true);
    switchTab(GuiTab::Json, false);

    ImGui::EndTabBar();
    ImGui::Separator();

    if (this->guiTab == GuiTab::Visual)
        this->renderGuiEditor();
    else if (this->guiTab == GuiTab::Misc)
        this->renderGuiMiscEditor();
    else
        this->renderYamlBody();

    ImGui::Separator();
    ImGui::BeginChild("##trigger-log", ImVec2(0, 150.f * this->imguiScale), true);
    this->renderLog();
    ImGui::EndChild();
}

void TriggerEditor::renderSidebar()
{
    float scale = this->imguiScale;

    ImGui::PushStyleColor(ImGuiCol_Text, GrimsolfTheme::textColor());
    for (int index = 0; index < (int)this->sources.size(); ++index)
    {
        if (index > 0)
            ImGui::SameLine();

        bool active = (index == this->sourceIndex);
        if (active)
        {
            ImGui::PushStyleColor(ImGuiCol_Button, GrimsolfTheme::gray(0.30f));
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, GrimsolfTheme::gray(0.38f));
            ImGui::PushStyleColor(ImGuiCol_ButtonActive, GrimsolfTheme::gray(0.24f));
        }

        if (ImGui::SmallButton(this->sources[index].label.c_str()) && !active)
            this->switchSource(index);

        if (active)
            ImGui::PopStyleColor(3);

        if (ImGui::IsItemHovered())
            ImGui::SetTooltip("%s", this->sources[index].ref.c_str());
    }

    bool canCreate = this->sourceIsEditable(this->sourceIndex);
    ImGui::SameLine();
    ImGui::SetCursorPosX(ImGui::GetCursorPosX() + 10.f * scale);
    if (!canCreate)
        ImGui::BeginDisabled();
    if (ImGui::SmallButton("[+]") && canCreate)
        this->createPopupOpen = true;
    if (!canCreate)
        ImGui::EndDisabled();
    if (ImGui::IsItemHovered())
        ImGui::SetTooltip("%s", canCreate ? "New trigger file"
                                          : "Only files inside the map's trigger folder can be created or deleted.");

    if (!this->sources.empty())
        ImGui::TextWrapped("%s: %s", "Folder", this->sources[this->sourceIndex].ref.c_str());
    ImGui::PopStyleColor();

    ImGui::Separator();

    const BrowseSource *source = this->sources.empty() ? nullptr : &this->sources[this->sourceIndex];

    if (source == nullptr || source->files.empty())
    {
        ImGui::PushStyleColor(ImGuiCol_Text, GrimsolfTheme::textColor());
        ImGui::TextWrapped("%s", "This folder has no trigger files yet.");
        ImGui::PopStyleColor();
    }
    else
    {
        for (const auto &file : source->files)
        {
            bool isMain = source->editable && (file == this->mainTriggerFile);
            bool selected = (this->currentFileSource == this->sourceIndex && file == this->currentFile);

            ImGui::PushID(file.c_str());
            if (ImGui::Selectable(file.c_str(), selected))
                this->requestAction("file", file);

            if (isMain)
            {
                ImVec2 itemMin = ImGui::GetItemRectMin();
                ImVec2 itemMax = ImGui::GetItemRectMax();
                ImDrawList *drawList = ImGui::GetWindowDrawList();
                ImFont *font = ImGui::GetFont();
                std::string tag = "main";
                float tagW = font->CalcTextSizeA(font->FontSize, FLT_MAX, 0.f, tag.c_str()).x + 10.f * scale;
                float x = itemMax.x - tagW - 6.f * scale;
                float cy = itemMin.y + (itemMax.y - itemMin.y) * 0.5f;
                drawList->AddRectFilled(ImVec2(x, itemMin.y + 2.f * scale),
                                        ImVec2(itemMax.x - 4.f * scale, itemMax.y - 2.f * scale),
                                        GrimsolfTheme::u32(0.55f, 0.5f));
                drawList->AddText(font, font->FontSize,
                                  ImVec2(x + 5.f * scale, cy - font->FontSize * 0.5f),
                                  GrimsolfTheme::textU32(), tag.c_str());
                if (ImGui::IsItemHovered())
                    ImGui::SetTooltip("%s", "Main trigger file (linked to the map). It cannot be deleted.");
            }
            ImGui::PopID();
        }
    }

    ImGui::Separator();

    bool currentEditable = this->sourceIsEditable(this->currentFileSource);
    bool canDelete = this->fileLoaded && !this->currentFile.empty() && currentEditable
                     && this->currentFile != this->mainTriggerFile;
    if (canDelete)
    {
        if (ImGui::Button("Delete", ImVec2(-1.f, 34.f * scale)))
        {
            this->deleteTarget = this->currentFile;
            this->deletePopupOpen = true;
        }
    }
    else
    {
        ImGui::PushStyleColor(ImGuiCol_Text, GrimsolfTheme::textColor());
        ImGui::Button("Delete", ImVec2(-1.f, 34.f * scale));
        ImGui::PopStyleColor();
        if (ImGui::IsItemHovered())
            ImGui::SetTooltip("%s", currentEditable ? "The main trigger file of the map cannot be deleted."
                                                     : "Only files inside the map's trigger folder can be created or deleted.");
    }

    ImGui::Spacing();

    if (ImGui::Button("Change map", ImVec2(-1.f, 34.f * scale)))
        this->requestAction("map", "");

    if (ImGui::Button("Back to Studio", ImVec2(-1.f, 34.f * scale)))
        this->requestAction("back", "");
}

void TriggerEditor::renderToolbar()
{
    
    float scale = this->imguiScale;

    ImGui::PushStyleColor(ImGuiCol_Button, GrimsolfTheme::gray(0.34f));
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, GrimsolfTheme::gray(0.42f));
    ImGui::PushStyleColor(ImGuiCol_ButtonActive, GrimsolfTheme::gray(0.26f));
    bool saveClicked = ImGui::Button("Save", ImVec2(130.f * scale, 32.f * scale));
    ImGui::PopStyleColor(3);
    if (saveClicked && this->fileLoaded)
        this->saveCurrentFile();

    ImGui::SameLine();
    bool formatEnabled = (this->guiTab != GuiTab::Json);
    if (!formatEnabled)
        ImGui::BeginDisabled();
    if (ImGui::Button("Format", ImVec2(130.f * scale, 32.f * scale)))
        this->formatYaml();
    if (!formatEnabled)
        ImGui::EndDisabled();

}

void TriggerEditor::renderLog()
{
    float scale = this->imguiScale;

    ImGui::PushStyleColor(ImGuiCol_Text, GrimsolfTheme::textColor());
    ImGui::TextUnformatted("Log");
    ImGui::PopStyleColor();
    ImGui::SameLine();
    ImGui::SetCursorPosX(ImGui::GetCursorPosX() + 10.f * scale);
    if (ImGui::SmallButton("Clear"))
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

bool TriggerEditor::textToGui()
{
    this->guiTriggers.clear();
    this->guiConvertError.clear();
    this->guiConvertible = true;

    try
    {
        json root = Json::parseYaml(this->text);
        if (!root.is_object())
        {
            this->guiConvertError = "The trigger file must be a YAML mapping (key: value).";
            this->guiConvertible = false;
            return false;
        }

        this->guiRoot = root;

        if (!root.contains("trigger") || root["trigger"].is_null())
        {
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

            if (block.contains("name") && block["name"].is_string())
                trigger.name = block["name"].get<std::string>();

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
        this->guiAnnotateFromText();
        return true;
    }
    catch (const std::exception &e)
    {
        this->guiConvertError = std::string("Invalid YAML: ") + e.what();
    }
    catch (...)
    {
        this->guiConvertError = "Invalid YAML.";
    }

    this->guiConvertible = false;
    return false;
}

namespace
{
    std::vector<std::string> guiItemYamlLines(const json &item)
    {
        json items = json::array();
        items.push_back(item);

        json document = json::object();
        document["k"] = items;

        std::vector<std::string> lines;
        std::istringstream stream(Json::dumpYaml(document));
        std::string line;
        bool header = true;
        while (std::getline(stream, line))
        {
            if (header)
            {
                header = false;
                continue;
            }
            if (line.size() >= 2 && line[0] == ' ' && line[1] == ' ')
                lines.push_back(line.substr(2));
            else if (!line.empty())
                lines.push_back(line);
        }
        return lines;
    }

    std::vector<std::string> guiCommentYamlLines(const std::string &text)
    {
        std::vector<std::string> lines;
        std::string current;
        for (char c : text)
        {
            if (c == '\n')
            {
                lines.push_back(current);
                current.clear();
            }
            else if (c != '\r')
                current += c;
        }
        lines.push_back(current);

        for (auto &line : lines)
        {
            while (!line.empty() && (line.back() == ' ' || line.back() == '\t'))
                line.pop_back();
            line = line.empty() ? std::string("#") : ("# " + line);
        }
        return lines;
    }

    void guiDisableYamlLines(std::vector<std::string> &lines)
    {
        for (auto &line : lines)
        {
            size_t indent = line.find_first_not_of(' ');
            if (indent == std::string::npos)
                line = "#";
            else
                line = line.substr(0, indent) + "# " + line.substr(indent);
        }
    }
}

void TriggerEditor::guiToText()
{
    json root = this->guiRoot;
    if (!root.is_object())
        root = json::object();

    json triggers = json::array();

    for (const auto &trigger : this->guiTriggers)
    {
        json block = json::object();
        if (!trigger.name.empty())
            block["name"] = trigger.name;

        json events = json::array();
        for (const auto &e : trigger.events)
        {
            if (e.isComment)
                continue;
            json j = e.raw;
            j["event"] = e.type;
            events.push_back(j);
        }
        block["events"] = events;

        json conditions = json::array();
        json conditionsOr = json::array();
        for (const auto &c : trigger.conditions)
        {
            if (c.isComment)
                continue;
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
            if (a.isComment)
                continue;
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
    std::string dumped = Json::dumpYaml(root);

    std::vector<std::string> triggerLines;
    triggerLines.push_back("trigger:");

    for (const auto &trigger : this->guiTriggers)
    {
        struct Section
        {
            const char *key;
            bool always;
        };
        const Section sections[] = {
            { "conditions",    true  },
            { "conditions-or", false },
            { "else",          false },
            { "events",        true  },
            { "then",          true  }
        };
        const int sectionCount = 5;
        std::vector<std::vector<std::string>> sectionLines(sectionCount);

        const auto appendList = [&](int section, const std::vector<GuiEntry> &list,
                                    int group, const char *typeKey)
        {
            for (const auto &entry : list)
            {
                if (entry.group != group)
                    continue;

                if (entry.isComment)
                {
                    for (const auto &line : guiCommentYamlLines(entry.comment))
                        sectionLines[section].push_back("  " + line);
                    continue;
                }

                json record = entry.raw.is_object() ? entry.raw : json::object();
                record[typeKey] = entry.type;

                std::vector<std::string> lines = guiItemYamlLines(record);
                if (entry.disabled)
                    guiDisableYamlLines(lines);
                for (const auto &line : lines)
                    sectionLines[section].push_back("  " + line);
            }
        };

        appendList(0, trigger.conditions, 0, "condition");
        appendList(1, trigger.conditions, 1, "condition");
        appendList(2, trigger.actions, 1, "action");
        appendList(3, trigger.events, 0, "event");
        appendList(4, trigger.actions, 0, "action");

        std::vector<std::string> blockLines;
        if (!trigger.name.empty())
            blockLines.push_back("name: " + json(trigger.name).dump());
        for (int index = 0; index < sectionCount; ++index)
        {
            if (sectionLines[index].empty() && !sections[index].always)
                continue;
            if (sectionLines[index].empty())
            {
                blockLines.push_back(std::string(sections[index].key) + ": []");
                continue;
            }
            blockLines.push_back(std::string(sections[index].key) + ":");
            for (const auto &line : sectionLines[index])
                blockLines.push_back(line);
        }

        triggerLines.push_back("  - " + blockLines.front());
        for (size_t index = 1; index < blockLines.size(); ++index)
            triggerLines.push_back("    " + blockLines[index]);
    }

    std::vector<std::vector<std::string>> chunks;
    {
        std::istringstream stream(dumped);
        std::string line;
        while (std::getline(stream, line))
        {
            if (chunks.empty() || (!line.empty() && line[0] != ' '))
                chunks.push_back(std::vector<std::string>());
            chunks.back().push_back(line);
        }
    }

    bool replaced = false;
    for (auto &chunk : chunks)
        if (!chunk.empty() && (chunk.front() == "trigger:" || chunk.front() == "trigger: []"))
        {
            chunk = triggerLines;
            replaced = true;
            break;
        }
    if (!replaced)
        chunks.push_back(triggerLines);

    std::string text;
    for (const auto &chunk : chunks)
        for (const auto &line : chunk)
            text += line + "\n";

    this->guiRoot = root;
    this->text = text;
}

void TriggerEditor::guiAnnotateFromText()
{
    if (this->guiTriggers.empty() || this->text.empty())
        return;

    std::vector<std::string> lines;
    {
        std::istringstream stream(this->text);
        std::string line;
        while (std::getline(stream, line))
        {
            while (!line.empty() && line.back() == '\r')
                line.pop_back();
            lines.push_back(line);
        }
    }

    int triggerIndent = -1;
    for (const auto &line : lines)
    {
        std::string trimmed = line;
        while (!trimmed.empty() && trimmed.back() == ' ')
            trimmed.pop_back();
        if (trimmed == "trigger:")
        {
            triggerIndent = (int)line.find_first_not_of(' ');
            break;
        }
    }
    if (triggerIndent < 0)
        return;

    const size_t blockIndent = (size_t)triggerIndent + 2;
    const size_t keyIndent = blockIndent + 2;
    const size_t itemIndent = blockIndent + 4;

    struct ScannedItem
    {
        bool live = false;
        bool comment = false;
        std::string commentText;
        std::string yaml;
    };

    struct ScannedList
    {
        std::string key;
        std::vector<ScannedItem> items;
    };

    std::vector<std::vector<ScannedList>> blocks;
    int blockIndex = -1;
    int listIndex = -1;
    int disabledIndex = -1;

    const auto stripCommentMarker = [](const std::string &content) -> std::string
    {
        std::string body = content.substr(1);
        if (!body.empty() && body[0] == ' ')
            body = body.substr(1);
        return body;
    };

    for (const auto &line : lines)
    {
        size_t indent = line.find_first_not_of(' ');
        if (indent == std::string::npos)
        {
            disabledIndex = -1;
            continue;
        }

        std::string content = line.substr(indent);

        if (disabledIndex >= 0 && indent > itemIndent && content[0] == '#'
            && blockIndex >= 0 && listIndex >= 0
            && disabledIndex < (int)blocks[blockIndex][listIndex].items.size())
        {
            ScannedItem &item = blocks[blockIndex][listIndex].items[disabledIndex];
            item.yaml += std::string(indent - itemIndent, ' ') + stripCommentMarker(content) + "\n";
            continue;
        }
        disabledIndex = -1;

        if (indent == blockIndent && content.rfind("- ", 0) == 0)
        {
            std::string rest = content.substr(2);
            size_t colon = rest.find(':');
            ScannedList list;
            list.key = (colon == std::string::npos) ? rest : rest.substr(0, colon);

            blocks.push_back(std::vector<ScannedList>());
            blocks.back().push_back(list);
            blockIndex = (int)blocks.size() - 1;
            listIndex = 0;
            continue;
        }

        if (indent == keyIndent && blockIndex >= 0)
        {
            size_t colon = content.find(':');
            std::string key = (colon == std::string::npos) ? content : content.substr(0, colon);
            bool known = (key == "events" || key == "conditions" || key == "conditions-or"
                          || key == "then" || key == "else");
            if (!known)
            {
                listIndex = -1;
                continue;
            }

            ScannedList list;
            list.key = key;
            blocks[blockIndex].push_back(list);
            listIndex = (int)blocks[blockIndex].size() - 1;
            continue;
        }

        if (indent == itemIndent && blockIndex >= 0 && listIndex >= 0)
        {
            ScannedItem item;
            if (content.rfind("- ", 0) == 0)
            {
                item.live = true;
            }
            else if (content[0] == '#')
            {
                std::string body = stripCommentMarker(content);
                if (body.rfind("- ", 0) == 0)
                {
                    ScannedItem disabledItem;
                    disabledItem.yaml = body + "\n";
                    blocks[blockIndex][listIndex].items.push_back(disabledItem);
                    disabledIndex = (int)blocks[blockIndex][listIndex].items.size() - 1;
                    continue;
                }
                item.comment = true;
                item.commentText = body;
            }
            else
            {
                continue;
            }
            blocks[blockIndex][listIndex].items.push_back(item);
        }
    }

    for (int block = 0; block < (int)this->guiTriggers.size() && block < (int)blocks.size(); ++block)
    {
        GuiTrigger &trigger = this->guiTriggers[block];

        struct Pending
        {
            std::vector<GuiEntry> entries;
            size_t cursor = 0;
            std::vector<GuiEntry> *target = nullptr;
            int group = 0;
        };

        const auto filterGroup = [](const std::vector<GuiEntry> &source, int group)
        {
            std::vector<GuiEntry> out;
            for (const auto &entry : source)
                if (entry.group == group && !entry.isComment)
                    out.push_back(entry);
            return out;
        };

        std::vector<GuiEntry> newEvents;
        std::vector<GuiEntry> newConditions;
        std::vector<GuiEntry> newActions;

        std::vector<Pending> pending(5);
        pending[0].entries = filterGroup(trigger.events, 0);
        pending[0].target = &newEvents;
        pending[0].group = 0;
        pending[1].entries = filterGroup(trigger.conditions, 0);
        pending[1].target = &newConditions;
        pending[1].group = 0;
        pending[2].entries = filterGroup(trigger.conditions, 1);
        pending[2].target = &newConditions;
        pending[2].group = 1;
        pending[3].entries = filterGroup(trigger.actions, 0);
        pending[3].target = &newActions;
        pending[3].group = 0;
        pending[4].entries = filterGroup(trigger.actions, 1);
        pending[4].target = &newActions;
        pending[4].group = 1;

        for (const auto &list : blocks[block])
        {
            int index = -1;
            if (list.key == "events")        index = 0;
            else if (list.key == "conditions")    index = 1;
            else if (list.key == "conditions-or") index = 2;
            else if (list.key == "then")          index = 3;
            else if (list.key == "else")          index = 4;
            if (index < 0)
                continue;

            Pending &current = pending[index];
            const char *typeKey = (index == 0) ? "event" : (index <= 2 ? "condition" : "action");

            for (const auto &item : list.items)
            {
                if (item.live)
                {
                    if (current.cursor < current.entries.size())
                        current.target->push_back(current.entries[current.cursor++]);
                    continue;
                }

                GuiEntry entry;
                entry.group = current.group;

                if (item.comment)
                {
                    entry.isComment = true;
                    entry.comment = item.commentText;
                    current.target->push_back(entry);
                    continue;
                }

                try
                {
                    json parsed = Json::parseYaml(item.yaml);
                    if (!parsed.is_array() || parsed.empty() || !parsed[0].is_object())
                        continue;
                    entry.raw = parsed[0];
                    entry.type = entry.raw.value(typeKey, "");
                    entry.disabled = true;
                    current.target->push_back(entry);
                }
                catch (...)
                {
                }
            }
        }

        for (auto &list : pending)
            while (list.cursor < list.entries.size())
                list.target->push_back(list.entries[list.cursor++]);

        std::stable_sort(newConditions.begin(), newConditions.end(),
                         [](const GuiEntry &a, const GuiEntry &b) { return a.group < b.group; });
        std::stable_sort(newActions.begin(), newActions.end(),
                         [](const GuiEntry &a, const GuiEntry &b) { return a.group < b.group; });

        trigger.events = newEvents;
        trigger.conditions = newConditions;
        trigger.actions = newActions;
    }
}

void TriggerEditor::guiSyncText()
{
    this->guiToText();
    this->validateYaml();
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

void TriggerEditor::guiDropEntry(std::vector<GuiEntry> &entries, int from, int to, bool insertBefore)
{
    int count = (int)entries.size();
    if (from < 0 || from >= count || to < 0 || to >= count || from == to)
        return;

    GuiEntry moved = entries[from];
    entries.erase(entries.begin() + from);

    int target = (to > from) ? (to - 1) : to;
    int insertAt = insertBefore ? target : target + 1;
    if (insertAt < 0)
        insertAt = 0;
    if (insertAt > (int)entries.size())
        insertAt = (int)entries.size();
    entries.insert(entries.begin() + insertAt, moved);
    this->guiSyncText();
}

void TriggerEditor::guiDeleteEntry(std::vector<GuiEntry> &entries, int index)
{
    if (index < 0 || index >= (int)entries.size())
        return;
    entries.erase(entries.begin() + index);
    this->guiSyncText();
}

void TriggerEditor::guiDuplicateEntry(std::vector<GuiEntry> &entries, int index)
{
    if (index < 0 || index >= (int)entries.size())
        return;
    GuiEntry copy = entries[index];
    entries.insert(entries.begin() + index + 1, copy);
    this->guiSyncText();
}

void TriggerEditor::guiCopyEntry(const GuiEntry &entry, GuiKind kind)
{
    this->guiClipboard = entry;
    this->guiClipboardKind = kind;
    this->guiClipboardValid = true;
}

void TriggerEditor::guiPasteEntry(std::vector<GuiEntry> &entries, GuiKind kind, int insertIndex)
{
    if (!this->guiClipboardValid || this->guiClipboardKind != kind)
        return;

    if (insertIndex < 0 || insertIndex > (int)entries.size())
        insertIndex = (int)entries.size();

    entries.insert(entries.begin() + insertIndex, this->guiClipboard);
    this->guiSyncText();
}

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

void TriggerEditor::openGuiPopup(GuiKind kind, int triggerIndex, int entryIndex, int insertIndex)
{
    this->guiPopup.open = true;
    this->guiPopup.kind = kind;
    this->guiPopup.editing = (entryIndex >= 0);
    this->guiPopup.triggerIndex = triggerIndex;
    this->guiPopup.entryIndex = entryIndex;
    this->guiPopup.insertIndex = this->guiPopup.editing ? -1 : insertIndex;
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

    if (this->guiPopup.insertIndex >= 0
        && triggerIndex >= 0 && triggerIndex < (int)this->guiTriggers.size())
    {
        std::vector<GuiEntry> &list = this->guiEntryList(kind, this->guiTriggers[triggerIndex]);
        int reference = this->guiPopup.insertIndex - 1;
        if (reference >= 0 && reference < (int)list.size())
            this->guiPopup.groupBuf = list[reference].group;
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
        {
            int insertAt = this->guiPopup.insertIndex;
            if (insertAt < 0 || insertAt > (int)list.size())
                insertAt = (int)list.size();
            list.insert(list.begin() + insertAt, e);
        }
    }

    this->guiPopup.open = false;
    this->guiSyncText();
}

void TriggerEditor::guiOpenCommentPopup(GuiKind kind, int triggerIndex, int entryIndex, int insertIndex)
{
    this->guiCommentPopupOpen = true;
    this->guiCommentKind = kind;
    this->guiCommentTriggerIndex = triggerIndex;
    this->guiCommentEntryIndex = entryIndex;
    this->guiCommentInsertIndex = (entryIndex >= 0) ? -1 : insertIndex;
    this->guiCommentBuf[0] = '\0';

    if (entryIndex >= 0 && triggerIndex >= 0 && triggerIndex < (int)this->guiTriggers.size())
    {
        std::vector<GuiEntry> &list = this->guiEntryList(kind, this->guiTriggers[triggerIndex]);
        if (entryIndex < (int)list.size() && list[entryIndex].isComment)
            std::strncpy(this->guiCommentBuf, list[entryIndex].comment.c_str(),
                         sizeof(this->guiCommentBuf) - 1);
    }
    this->guiCommentBuf[sizeof(this->guiCommentBuf) - 1] = '\0';
}

void TriggerEditor::commitGuiCommentPopup()
{
    if (this->guiCommentTriggerIndex < 0
        || this->guiCommentTriggerIndex >= (int)this->guiTriggers.size())
    {
        this->guiCommentPopupOpen = false;
        return;
    }

    std::vector<GuiEntry> &list = this->guiEntryList(this->guiCommentKind,
                                                     this->guiTriggers[this->guiCommentTriggerIndex]);

    std::string text = this->guiCommentBuf;
    std::replace(text.begin(), text.end(), '\r', ' ');
    std::replace(text.begin(), text.end(), '\n', ' ');

    if (this->guiCommentEntryIndex >= 0 && this->guiCommentEntryIndex < (int)list.size()
        && list[this->guiCommentEntryIndex].isComment)
    {
        list[this->guiCommentEntryIndex].comment = text;
    }
    else
    {
        GuiEntry entry;
        entry.isComment = true;
        entry.comment = text;

        int insertAt = this->guiCommentInsertIndex;
        if (insertAt < 0 || insertAt > (int)list.size())
            insertAt = (int)list.size();
        int reference = insertAt - 1;
        if (reference >= 0 && reference < (int)list.size())
            entry.group = list[reference].group;
        list.insert(list.begin() + insertAt, entry);
    }

    this->guiCommentPopupOpen = false;
    this->guiCommentEntryIndex = -1;
    this->guiCommentInsertIndex = -1;
    this->guiSyncText();
}

void TriggerEditor::renderGuiCommentPopup()
{
    if (!this->guiCommentPopupOpen)
        return;

    float scale = this->imguiScale;

    ImGui::OpenPopup("##trigger-gui-comment");
    if (ImGui::BeginPopupModal("##trigger-gui-comment", nullptr,
                               ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoSavedSettings))
    {
        ImGui::TextUnformatted(this->guiCommentEntryIndex >= 0 ? "Edit comment" : "Add comment");
        ImGui::Separator();
        ImGui::PushStyleColor(ImGuiCol_Text, GrimsolfTheme::gray(0.90f));
        ImGui::TextWrapped("The comment is written as a \"# ...\" line in the YAML file, at the "
                           "position of the row in the list.");
        ImGui::PopStyleColor();
        ImGui::Spacing();

        ImGui::SetNextItemWidth(520.f * scale);
        bool submitted = ImGui::InputText("##gui-comment-text", this->guiCommentBuf,
                                         sizeof(this->guiCommentBuf),
                                         ImGuiInputTextFlags_EnterReturnsTrue);

        ImGui::Spacing();
        if (ImGui::Button("Save", ImVec2(110.f * scale, 0)) || submitted)
        {
            this->commitGuiCommentPopup();
            ImGui::EndPopup();
            return;
        }

        ImGui::SameLine(0.f, 20.f * scale);
        if (ImGui::Button("Cancel", ImVec2(110.f * scale, 0))
            || ImGui::IsKeyPressed(ImGui::GetKeyIndex(ImGuiKey_Escape)))
        {
            this->guiCommentPopupOpen = false;
            this->guiCommentEntryIndex = -1;
            this->guiCommentInsertIndex = -1;
            ImGui::CloseCurrentPopup();
        }

        ImGui::EndPopup();
    }
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

    const ImGuiStyle &style = ImGui::GetStyle();
    float textH = ImGui::GetTextLineHeight();
    float inputH = ImGui::GetFrameHeight();
    float sp = style.ItemSpacing.y;
    float padY = style.WindowPadding.y * 2.f;
    float listRowH = textH + sp;

    float saveBarH = inputH;
    float paramsAvail = this->viewportH * 0.97f
                        - this->guiPopup.fixedBodyH
                        - saveBarH
                        - padY - sp;
    float paramsH = std::max(paramsAvail, std::max(GUI_PARAMS_MIN_ROWS * (inputH + sp), GUI_PARAMS_MIN_PX));

    float popupH = this->guiPopup.fixedBodyH + paramsH + saveBarH + padY + sp;
    popupH = std::max(popupH, GUI_POPUP_MIN_HEIGHT);
    popupH = std::min(popupH, this->viewportH * 0.97f);

    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 6.0f * scale);
    ImGui::OpenPopup("##trigger-gui-entry");
    ImGui::SetNextWindowSize(ImVec2(std::max(GUI_POPUP_MIN_WIDTH, this->viewportW * GUI_POPUP_MAX_WIDTH_FRACTION),
                                    popupH), ImGuiCond_Always);
    if (ImGui::BeginPopupModal("##trigger-gui-entry", nullptr,
                               ImGuiWindowFlags_NoSavedSettings))
    {
        ImGui::TextUnformatted(caption.c_str());
        ImGui::Separator();

        const std::vector<TriggerTypeDef> &defs = this->guiDefs(this->guiPopup.kind);

        ImGui::SetNextItemWidth(-FLT_MIN);
        ImGui::InputTextWithHint("##gui-search", "Search type...",
                                 this->guiPopup.search, sizeof(this->guiPopup.search));

        std::string filter = this->guiPopup.search;
        boost::to_lower(filter);

        int visibleCount = 0;
        for (const auto &def : defs)
        {
            std::string hay = def.description + " " + def.key;
            std::string low = hay;
            boost::to_lower(low);
            if (filter.empty() || low.find(filter) != std::string::npos)
                ++visibleCount;
        }
        float listRows = std::max((float)visibleCount, GUI_TYPE_LIST_MIN_ROWS);
        listRows = std::min(listRows, GUI_TYPE_LIST_MAX_ROWS);
        float listHeight = listRows * listRowH;
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
                    this->guiPopup.draft = GuiEntry();
                    this->guiPopup.draft.raw = json::object();
                    guiPopulateBuffers(this->guiPopup, &def);
                }
            }
        }
        ImGui::EndChild();

        const TriggerTypeDef *def = guiFindDef(defs, this->guiPopup.selectedType);
        if (def != nullptr)
        {
            ImGui::Spacing();
            ImGui::PushStyleColor(ImGuiCol_Text, GrimsolfTheme::gray(0.85f));
            ImGui::TextWrapped("%s", def->description.c_str());
            ImGui::PopStyleColor();
            ImGui::Separator();

            this->guiPopup.fixedBodyH = ImGui::GetCursorPosY() + style.WindowPadding.y;

            ImGui::BeginChild("##gui-params", ImVec2(0, paramsH), true);

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
            ImGui::PushStyleColor(ImGuiCol_Text, GrimsolfTheme::gray(0.80f));
            ImGui::TextWrapped("Unknown type \"%s\" has no editor metadata.\nIt will be kept as-is in the JSON.",
                               this->guiPopup.selectedType.c_str());
            ImGui::PopStyleColor();
        }

        if (!this->guiPopup.error.empty())
        {
            ImGui::PushStyleColor(ImGuiCol_Text, GrimsolfTheme::gray(0.62f));
            ImGui::TextWrapped("%s", this->guiPopup.error.c_str());
            ImGui::PopStyleColor();
        }

        ImGui::Separator();
        bool typing = ImGui::IsAnyItemActive();
        const bool escapePressed = ImGui::IsKeyPressed(ImGui::GetKeyIndex(ImGuiKey_Escape));
        bool saveClicked = ImGui::Button("Save", ImVec2(110.f * scale, 0))
                           || (ImGui::IsKeyPressed(ImGui::GetKeyIndex(ImGuiKey_Enter))
                               && !this->guiPopup.selectedType.empty() && !typing);
        if (saveClicked)
        {
            if (this->guiPopup.selectedType.empty())
                this->guiPopup.error = "Select a type first.";
            else
                this->commitGuiPopup();
        }

        ImGui::SameLine(0.f, 20.f * scale);
        if (ImGui::Button("Cancel", ImVec2(110.f * scale, 0)) || escapePressed)
        {
            this->guiPopup.open = false;
            this->guiPopup.error.clear();
            ImGui::CloseCurrentPopup();
        }

        ImGui::EndPopup();
    }
    ImGui::PopStyleVar();
}


void TriggerEditor::renderGuiMiscEditor()
{
    
    float scale = this->imguiScale;

    ImGui::PushStyleColor(ImGuiCol_Text, GrimsolfTheme::textColor());
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
        ImGui::PushStyleColor(ImGuiCol_Text, GrimsolfTheme::textColor());
        ImGui::TextWrapped("This file has no extra fields besides the trigger blocks.");
        ImGui::PopStyleColor();
    }

    for (const auto &key : keys)
    {
        ImGui::PushID(("misc-" + key).c_str());
        const json &value = this->guiRoot[key];
        if (value.is_array() || value.is_object())
        {
            ImGui::PushStyleColor(ImGuiCol_Text, GrimsolfTheme::gray(0.90f));
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
        if (ImGui::SmallButton("-"))
            removeIndex = i;
        ImGui::SameLine();

        json &item = value[i];
        bool valueWrapper = item.is_object() && item.size() == 1 && item.contains("value");
        if (valueWrapper)
        {
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

static float guiGroupContentHeight(const std::vector<GuiEntry> &entries)
{
    ImGuiStyle &style = ImGui::GetStyle();
    float padding = style.WindowPadding.y * 2.f + 2.f;
    if (entries.empty())
        return ImGui::GetTextLineHeight() + padding;
    float rowH = ImGui::GetFrameHeight();
    return rowH * (float)entries.size()
         + style.ItemSpacing.y * (float)(entries.size() - 1)
         + padding;
}

static const char *GUI_ENTRY_PAYLOAD = "TRIGGER-GUI-ENTRY";
struct GuiDropPayload
{
    int kind = 0;
    int index = 0;
};

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

    bool hideMemo = false;
    switch (kind)
    {
        case GuiKind::Event:     hideMemo = this->guiHideEvents;     break;
        case GuiKind::Condition: hideMemo = this->guiHideConditions; break;
        case GuiKind::Action:    hideMemo = this->guiHideActions;    break;
    }

    std::string toggleId = hideMemo ? ">##expand-" : "v##collapse-";
    toggleId += title;
    if (ImGui::SmallButton(toggleId.c_str()))
    {
        switch (kind)
        {
            case GuiKind::Event:     this->guiHideEvents = !this->guiHideEvents;     break;
            case GuiKind::Condition: this->guiHideConditions = !this->guiHideConditions; break;
            case GuiKind::Action:    this->guiHideActions = !this->guiHideActions;    break;
        }
    }
    if (ImGui::IsItemHovered())
        ImGui::SetTooltip(hideMemo ? "Show the %s list" : "Hide the %s list", title);

    ImGui::SameLine();
    ImGui::PushStyleColor(ImGuiCol_Text, GrimsolfTheme::textColor());
    ImGui::TextUnformatted((std::string(title) + "  (" + toStr((int)entries.size()) + ")").c_str());
    ImGui::PopStyleColor();
    float addButtonSize = 30.f * scale;
    ImGui::SameLine(ImGui::GetWindowContentRegionMax().x - addButtonSize);
    if (ImGui::Button(("+##add-" + std::string(title)).c_str(), ImVec2(addButtonSize, addButtonSize)))
        this->openGuiPopup(kind, this->guiTriggerIndex, -1);
    if (ImGui::IsItemHovered())
        ImGui::SetTooltip("Add %s", title);

    if (this->guiHideMemos || hideMemo)
    {
        ImGui::Separator();
        return;
    }


    const auto pasteMenuItem = [&]()
    {
        if (ImGui::MenuItem("Paste", "Ctrl+V", false,
                            this->guiClipboardValid && this->guiClipboardKind == kind))
            this->guiPasteEntry(entries, kind);
    };

    float listHeight = guiGroupContentHeight(entries);
    ImGui::BeginChild((std::string("##gui-list-") + title).c_str(), ImVec2(0, listHeight), true,
                      ImGuiWindowFlags_NoScrollbar);
    if (entries.empty())
    {
        ImGui::PushStyleColor(ImGuiCol_Text, GrimsolfTheme::textColor());
        ImGui::TextWrapped("No %s yet.", title);
        ImGui::PopStyleColor();
        if (ImGui::BeginPopupContextItem("##gui-list-empty-ctx"))
        {
            pasteMenuItem();
            ImGui::EndPopup();
        }
    }
    else
    {
        for (int i = 0; i < (int)entries.size(); ++i)
        {
            GuiEntry &entry = entries[i];
            ImGui::PushID((title + std::to_string(i)).c_str());

            std::string desc;
            std::string badge;
            std::string values;
            if (entry.isComment)
            {
                std::string text = entry.comment;
                std::replace(text.begin(), text.end(), '\n', ' ');
                desc = "# " + text;
                badge = "  [COMMENT]";
            }
            else
            {
                desc = this->guiDescription(kind, entry.type);
                if (entry.disabled)
                    badge += "  [DISABLED]";
                if (kind == GuiKind::Condition && entry.group == 1)
                    badge += "  [OR]";
                else if (kind == GuiKind::Action && entry.group == 1)
                    badge += "  [ELSE]";
                values = this->guiEntryValues(kind, entry);
            }

            const bool dimmed = entry.isComment || entry.disabled;
            if (dimmed)
            {
                ImVec4 color = entry.isComment ? GrimsolfTheme::gray(0.78f)
                                               : GrimsolfTheme::gray(0.55f);
                ImGui::PushStyleColor(ImGuiCol_Text, color);
            }

            ImVec2 rowMin = ImGui::GetCursorScreenPos();
            if (ImGui::Selectable((desc + badge).c_str(), false))
            {
                if (entry.isComment)
                    this->guiOpenCommentPopup(kind, this->guiTriggerIndex, i);
                else
                    this->openGuiPopup(kind, this->guiTriggerIndex, i);
            }

            if (dimmed)
                ImGui::PopStyleColor();

            ImVec2 rowMax = ImGui::GetItemRectMax();
            if (!values.empty())
            {
                ImDrawList *drawList = ImGui::GetWindowDrawList();
                float descW = ImGui::CalcTextSize((desc + badge).c_str()).x;
                ImVec2 valuesPos(rowMin.x + descW + 12.f * scale, rowMin.y);
                drawList->AddText(valuesPos, colorKeyword(entry.disabled ? 0.55f : 1.0f),
                                  values.c_str());
            }
            if (ImGui::IsItemHovered())
            {
                std::string tooltip = entry.isComment ? std::string("Comment row") : entry.type;
                if (!values.empty())
                    tooltip += "\n" + values;
                if (entry.disabled)
                    tooltip += "\nDisabled: commented out in the YAML (the game ignores it)";
                tooltip += "\nDrag to reorder";
                ImGui::SetTooltip("%s", tooltip.c_str());
            }

            if (ImGui::BeginDragDropSource())
            {
                GuiDropPayload payload;
                payload.kind = (int)kind;
                payload.index = i;
                ImGui::SetDragDropPayload(GUI_ENTRY_PAYLOAD, &payload, sizeof(payload));
                ImGui::Text("%s", (desc + badge).c_str());
                ImGui::EndDragDropSource();
            }
            int dropFrom = -1;
            bool dropBefore = false;
            if (ImGui::BeginDragDropTarget())
            {
                if (const ImGuiPayload *payload = ImGui::AcceptDragDropPayload(
                        GUI_ENTRY_PAYLOAD, ImGuiDragDropFlags_AcceptBeforeDelivery))
                {
                    if (payload->DataSize == (int)sizeof(GuiDropPayload))
                    {
                        const GuiDropPayload *drop = (const GuiDropPayload *)payload->Data;
                        if (drop->kind == (int)kind)
                        {
                            bool before = ImGui::GetMousePos().y < (rowMin.y + rowMax.y) * 0.5f;
                            float lineY = before ? rowMin.y : rowMax.y;
                            ImGui::GetWindowDrawList()->AddLine(
                                ImVec2(rowMin.x, lineY), ImVec2(rowMax.x, lineY),
                                GrimsolfTheme::u32(0.95f), 2.f * scale);
                            if (payload->IsDelivery())
                            {
                                dropFrom = drop->index;
                                dropBefore = before;
                            }
                        }
                    }
                }
                ImGui::EndDragDropTarget();
            }
            if (dropFrom >= 0 && dropFrom != i)
                this->guiDropEntry(entries, dropFrom, i, dropBefore);

            if (ImGui::BeginPopupContextItem())
            {
                if (!entry.isComment && ImGui::MenuItem("Add"))
                    this->openGuiPopup(kind, this->guiTriggerIndex, -1, i + 1);
                if (!entry.isComment)
                {
                    if (ImGui::MenuItem(entry.disabled ? "Enable" : "Disable",
                                        entry.disabled ? "(off)" : "(on)"))
                    {
                        entry.disabled = !entry.disabled;
                        this->guiSyncText();
                    }
                }
                if (entry.isComment && ImGui::MenuItem("Edit comment"))
                    this->guiOpenCommentPopup(kind, this->guiTriggerIndex, i);
                if (ImGui::MenuItem("Add comment"))
                    this->guiOpenCommentPopup(kind, this->guiTriggerIndex, -1, i + 1);
                ImGui::Separator();
                if (ImGui::MenuItem("Move up"))
                    this->guiMoveEntry(entries, i, -1);
                if (ImGui::MenuItem("Move down"))
                    this->guiMoveEntry(entries, i, +1);
                ImGui::Separator();
                if (ImGui::MenuItem("Edit"))
                    this->openGuiPopup(kind, this->guiTriggerIndex, i);
                if (ImGui::MenuItem("Copy"))
                    this->guiCopyEntry(entry, kind);
                if (ImGui::MenuItem("Duplicate"))
                    this->guiDuplicateEntry(entries, i);
                if (ImGui::MenuItem("Delete"))
                    this->guiDeleteEntry(entries, i);
                ImGui::EndPopup();
            }

            ImGui::PopID();
        }
    }

    bool listHovered = ImGui::IsWindowHovered(ImGuiHoveredFlags_ChildWindows);
    bool listFocused = ImGui::IsWindowFocused(ImGuiFocusedFlags_RootAndChildWindows);
    if (ImGui::BeginPopupContextWindow("##gui-list-ctx",
                                       ImGuiPopupFlags_MouseButtonRight | ImGuiPopupFlags_NoOpenOverItems))
    {
        pasteMenuItem();
        ImGui::EndPopup();
    }
    ImGui::EndChild();

    const bool anyPopupOpen = ImGui::IsPopupOpen("", ImGuiPopupFlags_AnyPopup);
    if (!anyPopupOpen && (listHovered || listFocused) && !ImGui::GetIO().WantTextInput &&
        ImGui::GetIO().KeyCtrl && ImGui::IsKeyPressed(ImGui::GetKeyIndex(ImGuiKey_V)))
        this->guiPasteEntry(entries, kind);

    ImGui::Separator();
}

void TriggerEditor::renderGuiEditor()
{
    float scale = this->imguiScale;

    if (this->guiTriggers.empty())
        this->guiTriggers.push_back(GuiTrigger());
    if (this->guiTriggerIndex < 0 || this->guiTriggerIndex >= (int)this->guiTriggers.size())
        this->guiTriggerIndex = 0;

    ImGui::PushStyleColor(ImGuiCol_Text, GrimsolfTheme::textColor());
    ImGui::TextUnformatted("Trigger block");
    ImGui::PopStyleColor();
    const auto blockLabel = [this](int index) -> std::string
    {
        const std::string &name = this->guiTriggers[index].name;
        return name.empty() ? ("Trigger " + toStr(index + 1)) : name;
    };

    ImGui::SameLine(90.f * scale);
    if (ImGui::BeginCombo("##trigger-block", blockLabel(this->guiTriggerIndex).c_str()))
    {
        for (int i = 0; i < (int)this->guiTriggers.size(); ++i)
        {
            ImGui::PushID(i);
            if (ImGui::Selectable(blockLabel(i).c_str(), i == this->guiTriggerIndex))
                this->guiTriggerIndex = i;
            ImGui::PopID();
        }
        ImGui::EndCombo();
    }
    ImGui::SameLine();
    if (ImGui::SmallButton("Name..."))
    {
        std::snprintf(this->guiNameBuf, sizeof(this->guiNameBuf), "%s",
                      this->guiTriggers[this->guiTriggerIndex].name.c_str());
        this->guiNamePopupOpen = true;
    }
    if (ImGui::IsItemHovered())
        ImGui::SetTooltip("Edit the name of this trigger block (editor only, the game ignores it)");
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
        ImGui::PushStyleColor(ImGuiCol_Text, GrimsolfTheme::textColor());
        ImGui::SmallButton("- Remove block");
        ImGui::PopStyleColor();
    }

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

    ImGui::SameLine();
    if (ImGui::Button(this->guiHideMemos ? "Show memos" : "Hide memos",
                      ImVec2(0, 30.f * scale)))
        this->guiHideMemos = !this->guiHideMemos;
    if (ImGui::IsItemHovered())
        ImGui::SetTooltip(this->guiHideMemos ? "Show the event, condition and action lists"
                                             : "Hide the event, condition and action lists");

    ImGui::Separator();

    GuiTrigger &trigger = this->guiTriggers[this->guiTriggerIndex];

    this->renderGuiGroup(GuiKind::Event, trigger.events);
    this->renderGuiGroup(GuiKind::Condition, trigger.conditions);
    this->renderGuiGroup(GuiKind::Action, trigger.actions);

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
                || ImGui::IsKeyPressed(ImGui::GetKeyIndex(ImGuiKey_Escape)))
            {
                this->guiDeleteBlockOpen = false;
                ImGui::CloseCurrentPopup();
            }
            ImGui::EndPopup();
        }
    }

    this->renderGuiPopup();

    this->renderGuiCommentPopup();

    this->renderGuiNamePopup();
}

void TriggerEditor::renderGuiNamePopup()
{
    float scale = this->imguiScale;

    if (this->guiNamePopupOpen)
    {
        ImGui::OpenPopup("##trigger-gui-name");
        this->guiNamePopupOpen = false;
    }

    if (!ImGui::BeginPopupModal("##trigger-gui-name", nullptr, ImGuiWindowFlags_AlwaysAutoResize))
        return;

    if (this->guiTriggerIndex < 0 || this->guiTriggerIndex >= (int)this->guiTriggers.size())
    {
        ImGui::CloseCurrentPopup();
        ImGui::EndPopup();
        return;
    }

    GuiTrigger &trigger = this->guiTriggers[this->guiTriggerIndex];

    ImGui::TextUnformatted("Trigger block name");
    ImGui::Separator();
    ImGui::TextUnformatted("Visual label of this block, used only in the trigger editor.");
    ImGui::TextUnformatted("Leave it empty to keep the default \"Trigger N\" name.");
    ImGui::Spacing();
    ImGui::SetNextItemWidth(340.f * scale);
    const bool submitted = ImGui::InputText("##gui-block-name", this->guiNameBuf,
                                            sizeof(this->guiNameBuf),
                                            ImGuiInputTextFlags_EnterReturnsTrue);
    ImGui::Spacing();

    const auto applyName = [&](const char *value)
    {
        std::string name = value;
        while (!name.empty() && std::isspace((unsigned char)name.front()))
            name.erase(name.begin());
        while (!name.empty() && std::isspace((unsigned char)name.back()))
            name.pop_back();
        trigger.name = name;
        this->guiSyncText();
    };

    if (ImGui::Button("Save", ImVec2(100.f * scale, 0)) || submitted)
    {
        applyName(this->guiNameBuf);
        ImGui::CloseCurrentPopup();
    }
    ImGui::SameLine(0.f, 20.f * scale);
    if (ImGui::Button("Clear", ImVec2(100.f * scale, 0)))
    {
        applyName("");
        ImGui::CloseCurrentPopup();
    }
    ImGui::SameLine(0.f, 20.f * scale);
    if (ImGui::Button("Cancel", ImVec2(100.f * scale, 0))
        || ImGui::IsKeyPressed(ImGui::GetKeyIndex(ImGuiKey_Escape)))
        ImGui::CloseCurrentPopup();

    ImGui::EndPopup();
}


void TriggerEditor::renderMapSelect()
{
    
    float scale = this->imguiScale;

    ImGui::PushStyleColor(ImGuiCol_Text, GrimsolfTheme::textColor());
    ImGui::TextUnformatted("Trigger Editor");
    ImGui::PopStyleColor();
    ImGui::SameLine();
    ImGui::SetCursorPosX(ImGui::GetCursorPosX() + 10.f * scale);
    if (ImGui::SmallButton("Refresh"))
    {
        this->refreshMapList();
        this->refreshSources();
    }

    ImGui::SameLine(ImGui::GetWindowContentRegionMax().x - 220.f * scale);
    if (ImGui::Button("Back to Studio", ImVec2(200.f * scale, 34.f * scale)))
    {
        if (this->performAction("back", ""))
            this->pendingLeave = true;
    }

    ImGui::Separator();

    ImGui::BeginChild("##trigger-map-list", ImVec2(380.f * scale, 0), true);
    ImGui::PushStyleColor(ImGuiCol_Text, GrimsolfTheme::textColor());
    ImGui::TextUnformatted("Maps");
    ImGui::PopStyleColor();
    ImGui::Separator();

    if (this->mapList.empty())
    {
        ImGui::PushStyleColor(ImGuiCol_Text, GrimsolfTheme::textColor());
        ImGui::TextWrapped("%s", "No maps found under data/maps/. Create a map with the World Editor first.");
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

    bool hasExtraFiles = false;
    for (const auto &source : this->sources)
        if (!source.editable && !source.files.empty())
            hasExtraFiles = true;

    if (hasExtraFiles)
    {
        ImGui::Spacing();
        ImGui::Separator();
        ImGui::PushStyleColor(ImGuiCol_Text, GrimsolfTheme::textColor());
        ImGui::TextUnformatted("Other folders");
        ImGui::PopStyleColor();
        ImGui::Separator();

        for (int index = 0; index < (int)this->sources.size(); ++index)
        {
            if (this->sources[index].editable)
                continue;

            const std::string ref = this->sources[index].ref;
            const std::vector<std::string> files = this->sources[index].files;

            for (const auto &file : files)
            {
                std::string label = ref + "/" + file;
                ImGui::PushID(label.c_str());
                if (ImGui::Selectable(label.c_str()))
                    this->openSourceSession(index, file);
                ImGui::PopID();
            }
        }
    }
    ImGui::EndChild();

    ImGui::SameLine();

    ImGui::BeginChild("##trigger-map-info", ImVec2(0, 0), true);
    ImGui::PushStyleColor(ImGuiCol_Text, GrimsolfTheme::textColor());
    ImGui::TextUnformatted("Select a map");
    ImGui::PopStyleColor();
    ImGui::Separator();

    if (this->selectedMap.empty())
    {
        ImGui::PushStyleColor(ImGuiCol_Text, GrimsolfTheme::textColor());
        ImGui::TextWrapped("%s", "Pick a map on the left and click Edit Triggers. If the map has no trigger yet, "
                                 "the editor creates and links it automatically (data/maps/<folder>/trigger/<map>.json). "
                                 "Interaction and cutscene files can be opened right away, without a map.");
        ImGui::PopStyleColor();
    }
    else
    {
        ImGui::PushStyleColor(ImGuiCol_Text, GrimsolfTheme::gray(0.80f));
        ImGui::TextWrapped("%s", this->selectedMap.c_str());
        ImGui::PopStyleColor();
        ImGui::Spacing();

        if (ImGui::Button("Edit Triggers", ImVec2(240.f * scale, 40.f * scale)))
            this->openMapSession(this->selectedMap);
    }
    ImGui::EndChild();
}


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
        ImGui::PushStyleColor(ImGuiCol_Text, GrimsolfTheme::gray(0.62f));
        ImGui::TextUnformatted("Error");
        ImGui::PopStyleColor();
        ImGui::Separator();

        ImGui::PushStyleColor(ImGuiCol_Text, GrimsolfTheme::textColor());
        for (auto &line : this->errorPopupLines)
            ImGui::TextWrapped("%s", line.c_str());
        ImGui::PopStyleColor();

        ImGui::Separator();
        bool closed = ImGui::Button("Close", ImVec2(140 * scale, 0))
                      || ImGui::IsKeyPressed(ImGui::GetKeyIndex(ImGuiKey_Escape));
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
        this->showError(std::string("[error] exceção no Trigger Editor: ") + e.what());
    }
    catch (...)
    {
        this->showError("[error] exceção desconhecida no Trigger Editor");
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

    ImGui::PushStyleColor(ImGuiCol_WindowBg, GrimsolfTheme::gray(0.05f));
    ImGui::PushStyleColor(ImGuiCol_Text, GrimsolfTheme::textColor());
    ImGui::PushStyleColor(ImGuiCol_Border, GrimsolfTheme::gray(0.35f));
    ImGui::PushStyleColor(ImGuiCol_Button, GrimsolfTheme::gray(0.16f));
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, GrimsolfTheme::gray(0.27f));
    ImGui::PushStyleColor(ImGuiCol_ButtonActive, GrimsolfTheme::gray(0.32f));
    ImGui::PushStyleColor(ImGuiCol_FrameBg, GrimsolfTheme::gray(0.12f));
    ImGui::PushStyleColor(ImGuiCol_FrameBgHovered, GrimsolfTheme::gray(0.20f));
    ImGui::PushStyleColor(ImGuiCol_CheckMark, GrimsolfTheme::gray(1.0f));

    if (this->stage == Stage::MapSelect)
    {
        this->renderMapSelect();
    }
    else
    {
        ImGui::PushFont(this->fontBig);
        ImGui::TextUnformatted("Trigger Editor");
        ImGui::PopFont();
        ImGui::SameLine(ImGui::GetWindowContentRegionMax().x - 180.f * this->imguiScale);
        if (ImGui::Button("Back to Studio", ImVec2(160.f * this->imguiScale, 34.f * this->imguiScale)))
            this->requestAction("back", "");
        ImGui::Separator();

        float sidebarWidth = 300.f * this->imguiScale;

        ImGui::BeginChild("##trigger-sidebar", ImVec2(sidebarWidth, 0), true);
        this->renderSidebar();
        ImGui::EndChild();

        ImGui::SameLine();

        ImGuiWindowFlags rightPanelFlags = ImGuiWindowFlags_AlwaysVerticalScrollbar;
        if (this->guiTab == GuiTab::Json)
            rightPanelFlags |= ImGuiWindowFlags_NoScrollWithMouse;
        ImGui::BeginChild("##trigger-right", ImVec2(0, 0), false, rightPanelFlags);
        this->renderEditor();
        ImGui::EndChild();
    }

    if (this->createPopupOpen)
    {
        ImGui::OpenPopup("##trigger-create");
        if (ImGui::BeginPopupModal("##trigger-create", nullptr, ImGuiWindowFlags_AlwaysAutoResize))
        {
            ImGui::TextUnformatted("New trigger file");
            ImGui::Separator();
            ImGui::TextUnformatted("File name (without .json):");
            ImGui::SetNextItemWidth(260.f * this->imguiScale);
            if (ImGui::InputText("##trigger-new-name", this->newFileName, sizeof(this->newFileName),
                                 ImGuiInputTextFlags_EnterReturnsTrue))
                this->createError.clear();

            if (!this->createError.empty())
            {
                ImGui::PushStyleColor(ImGuiCol_Text, GrimsolfTheme::gray(0.62f));
                ImGui::TextWrapped("%s", this->createError.c_str());
                ImGui::PopStyleColor();
            }

            ImGui::Spacing();
            if (ImGui::Button("Create", ImVec2(110.f * this->imguiScale, 0))
                || ImGui::IsKeyPressed(ImGui::GetKeyIndex(ImGuiKey_Enter)))
            {
                std::string trimmed = this->newFileName;
                boost::trim(trimmed);
                if (boost::iends_with(trimmed, ".yaml"))
                    trimmed = trimmed.substr(0, trimmed.size() - 5);
                boost::trim(trimmed);

                bool valid = !trimmed.empty();
                for (char c : trimmed)
                    if (!(std::isalnum((unsigned char)c) != 0 || c == '-' || c == '_'))
                    {
                        valid = false;
                        break;
                    }

                bool exists = !trimmed.empty() && !this->sources.empty()
                              && boost::filesystem::exists(this->sources[this->sourceIndex].folder + trimmed + ".yaml");

                if (!valid)
                    this->createError = "Invalid name. Use only letters, numbers, \"-\" and \"_\".";
                else if (exists)
                    this->createError = "A file with this name already exists in this folder.";
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
            if (ImGui::Button("Cancel", ImVec2(110.f * this->imguiScale, 0))
                || ImGui::IsKeyPressed(ImGui::GetKeyIndex(ImGuiKey_Escape)))
            {
                this->createPopupOpen = false;
                this->createError.clear();
                ImGui::CloseCurrentPopup();
            }

            ImGui::EndPopup();
        }
    }

    if (this->deletePopupOpen)
    {
        ImGui::OpenPopup("##trigger-delete");
        if (ImGui::BeginPopupModal("##trigger-delete", nullptr, ImGuiWindowFlags_AlwaysAutoResize))
        {
            ImGui::TextUnformatted("Delete trigger file");
            ImGui::Separator();
            ImGui::TextWrapped("%s \"%s\"?", "Are you sure you want to delete",
                               this->deleteTarget.c_str());

            if (this->deleteTarget == this->mainTriggerFile)
            {
                ImGui::PushStyleColor(ImGuiCol_Text, GrimsolfTheme::gray(0.62f));
                ImGui::TextWrapped("%s", "The main trigger file of the map cannot be deleted.");
                ImGui::PopStyleColor();
            }

            ImGui::Spacing();
            if (ImGui::Button("Yes", ImVec2(100.f * this->imguiScale, 0)))
            {
                this->deletePopupOpen = false;
                ImGui::CloseCurrentPopup();
                this->deleteTriggerFile(this->deleteTarget);
                this->deleteTarget.clear();
            }

            ImGui::SameLine(0.f, 20.f * this->imguiScale);
            if (ImGui::Button("No", ImVec2(100.f * this->imguiScale, 0))
                || ImGui::IsKeyPressed(ImGui::GetKeyIndex(ImGuiKey_Escape)))
            {
                this->deletePopupOpen = false;
                this->deleteTarget.clear();
                ImGui::CloseCurrentPopup();
            }

            ImGui::EndPopup();
        }
    }

    if (this->discardPopupOpen)
    {
        ImGui::OpenPopup("##trigger-discard");
        if (ImGui::BeginPopupModal("##trigger-discard", nullptr, ImGuiWindowFlags_AlwaysAutoResize))
        {
            ImGui::TextUnformatted("Unsaved changes");
            ImGui::Separator();
            ImGui::TextWrapped("%s", "The current file has unsaved changes. Discard them?");

            ImGui::Spacing();
            if (ImGui::Button("Yes", ImVec2(100.f * this->imguiScale, 0)))
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
            if (ImGui::Button("No", ImVec2(100.f * this->imguiScale, 0))
                || ImGui::IsKeyPressed(ImGui::GetKeyIndex(ImGuiKey_Escape)))
            {
                this->discardPopupOpen = false;
                this->discardAction.clear();
                this->discardTarget.clear();
                ImGui::CloseCurrentPopup();
            }

            ImGui::EndPopup();
        }
    }

    ImGui::PopStyleColor(9);
    ImGui::End();
    ImGui::PopStyleVar(3);
    ImGui::PopFont();

    bool leave = this->pendingLeave || leaveRequested;
    this->pendingLeave = false;
    return leave;
}
