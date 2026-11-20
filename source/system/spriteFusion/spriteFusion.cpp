#include "spriteFusion.hpp"
#include "../external/imgui/imgui.h"
#include "../external/imgui/imgui-SFML.h"
#include "../grimsolfTheme.hpp"

#include <fstream>
#include <cstring>
#include <cctype>
#include <cmath>
#include <algorithm>
#include <boost/filesystem.hpp>


SpriteFusion::SpriteFusion(const std::string &gamePath, ImFont *fontNormal, ImFont *fontBig,
                           bool dropSupported)
{
    this->gamePath = gamePath;
    this->fontNormal = fontNormal;
    this->fontBig = fontBig;
    this->dropSupported = dropSupported;

    json options = this->loadJsonFile(gamePath + "/data/options/options.json");

    this->refreshDirectory();

    if (options.is_object())
    {
        std::string savedFolder = options.value("misc-sprite-fusion-folder", "");
        if (!savedFolder.empty())
        {
            try
            {
                boost::filesystem::path dir = boost::filesystem::path(this->gamePath)
                                              / "resources" / "sprites" / savedFolder;
                if (boost::filesystem::is_directory(dir))
                    this->saveFolder = savedFolder;
            }
            catch (...)
            {
            }
        }
    }
    this->refreshSaveFolderList();
}

SpriteFusion::~SpriteFusion()
{
    this->stack.clear();
    this->folderList.clear();
    this->fileList.clear();
    this->logLines.clear();
}

json SpriteFusion::loadJsonFile(const std::string &path) const
{
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

void SpriteFusion::addLog(const std::string &text, unsigned int color)
{
    const unsigned int themed = ImGui::ColorConvertFloat4ToU32(
        GrimsolfTheme::toGray(ImGui::ColorConvertU32ToFloat4(color)));
    this->logLines.push_back(LogLine{ text, themed });
    if (this->logLines.size() > 500)
        this->logLines.erase(this->logLines.begin());
    this->logDirty = true;
}

bool SpriteFusion::readPngSize(const std::string &path, int &width, int &height) const
{
    std::ifstream stream(path, std::ios::binary);
    if (!stream)
        return false;

    unsigned char header[24];
    stream.read(reinterpret_cast<char *>(header), 24);
    if (stream.gcount() < 24)
        return false;

    static const unsigned char pngSignature[8] = { 0x89, 'P', 'N', 'G', 0x0D, 0x0A, 0x1A, 0x0A };
    if (std::memcmp(header, pngSignature, 8) != 0)
        return false;

    width = (header[16] << 24) | (header[17] << 16) | (header[18] << 8) | header[19];
    height = (header[20] << 24) | (header[21] << 16) | (header[22] << 8) | header[23];
    return width > 0 && height > 0;
}


void SpriteFusion::refreshDirectory()
{
    this->folderList.clear();
    this->fileList.clear();
    this->selectedFile = "";
    this->selectedFileInfo = "";

    try
    {
        boost::filesystem::path dir = boost::filesystem::path(this->gamePath)
                                      / "resources" / "sprites" / this->currentPath;
        if (!boost::filesystem::is_directory(dir))
            return;

        for (auto &entry : boost::filesystem::directory_iterator(dir))
        {
            if (boost::filesystem::is_directory(entry))
                this->folderList.push_back(entry.path().filename().string());
            else if (boost::filesystem::is_regular_file(entry) && entry.path().extension() == ".png")
                this->fileList.push_back(entry.path().filename().string());
        }
    }
    catch (...)
    {
        this->addLog("[error] could not read the folder", 0xFF6060FF);
    }

    std::sort(this->folderList.begin(), this->folderList.end());
    std::sort(this->fileList.begin(), this->fileList.end());
}

void SpriteFusion::navigateTo(const std::string &path)
{
    try
    {
        boost::filesystem::path dir = boost::filesystem::path(this->gamePath)
                                      / "resources" / "sprites" / path;
        if (!boost::filesystem::is_directory(dir))
            return;

        this->currentPath = path;
        this->refreshDirectory();
    }
    catch (...)
    {
    }
}

void SpriteFusion::navigateUp()
{
    size_t pos = this->currentPath.find_last_of('/');
    this->navigateTo(pos == std::string::npos ? "" : this->currentPath.substr(0, pos));
}

void SpriteFusion::selectFile(const std::string &relative)
{
    this->selectedFile = relative;

    int w = 0, h = 0;
    std::string fullPath = this->gamePath + "/resources/sprites/" + relative;
    if (this->readPngSize(fullPath, w, h))
        this->selectedFileInfo = relative + " (" + std::to_string(w) + "x" + std::to_string(h) + " px)";
    else
        this->selectedFileInfo = relative + " (?)";
}

void SpriteFusion::addSelectedToStack()
{
    if (this->selectedFile.empty())
    {
        this->addLog("[warn] select a PNG file in the browser first", 0xFFD070FF);
        return;
    }
    this->addToStack(this->selectedFile);
}


void SpriteFusion::addToStack(const std::string &relative)
{
    this->addImageFromPath(this->gamePath + "/resources/sprites/" + relative);
}

void SpriteFusion::addExternalFiles(const std::vector<std::string> &paths)
{
    for (const std::string &path : paths)
        this->addImageFromPath(path);
}

void SpriteFusion::addImageFromPath(const std::string &fullPath)
{
    std::string lower = fullPath;
    std::transform(lower.begin(), lower.end(), lower.begin(),
                   [](unsigned char c) { return (char)std::tolower(c); });
    if (lower.size() < 4 || lower.compare(lower.size() - 4, 4, ".png") != 0)
    {
        this->addLog("[warn] " + fullPath + " is not a PNG file and was ignored", 0xFFD070FF);
        return;
    }

    sf::Texture texture;
    if (!texture.loadFromFile(fullPath))
    {
        this->addLog("[error] could not load " + fullPath, 0xFF6060FF);
        return;
    }

    SpriteFusionItem item;
    item.name = fullPath.substr(fullPath.find_last_of("/\\") + 1);
    item.relative = fullPath;
    item.texture = texture;
    item.width = (int)texture.getSize().x;
    item.height = (int)texture.getSize().y;
    item.loaded = true;

    this->stack.push_back(item);
    this->mergeDirty = true;

    this->addLog("[ok] " + item.name + " (" + std::to_string(item.width) + "x"
                 + std::to_string(item.height) + " px) added to the stack", 0x88CCFFFF);
}

void SpriteFusion::removeFromStack(int index)
{
    if (index < 0 || index >= (int)this->stack.size())
        return;

    std::string name = this->stack[index].name;
    this->stack.erase(this->stack.begin() + index);
    this->mergeDirty = true;
    this->addLog("[info] " + name + " removed from the stack", 0xFFD070FF);
}

void SpriteFusion::moveStackItem(int index, int direction)
{
    if (index < 0 || index >= (int)this->stack.size())
        return;
    int target = index + direction;
    if (target < 0 || target >= (int)this->stack.size())
        return;

    std::swap(this->stack[index], this->stack[target]);
    this->mergeDirty = true;
}

void SpriteFusion::reorderStack(int sourceIndex, int targetIndex, bool insertBefore)
{
    if (sourceIndex < 0 || sourceIndex >= (int)this->stack.size())
        return;
    if (targetIndex < 0 || targetIndex >= (int)this->stack.size())
        return;
    if (sourceIndex == targetIndex)
        return;

    SpriteFusionItem item = this->stack[sourceIndex];
    this->stack.erase(this->stack.begin() + sourceIndex);

    int newTarget = targetIndex;
    if (targetIndex > sourceIndex)
        newTarget = targetIndex - 1;

    int insertAt = insertBefore ? newTarget : newTarget + 1;
    insertAt = std::max(0, std::min(insertAt, (int)this->stack.size()));
    this->stack.insert(this->stack.begin() + insertAt, item);
    this->mergeDirty = true;
    this->addLog("[info] " + item.name + " #" + std::to_string(insertAt + 1), 0x88CCFFFF);
}

void SpriteFusion::clearStack()
{
    if (this->stack.empty())
        return;
    this->stack.clear();
    this->mergeDirty = true;
    this->addLog("[info] stack cleared", 0xFFD070FF);
}


void SpriteFusion::refreshSaveFolderList()
{
    this->saveFolderList.clear();

    try
    {
        boost::filesystem::path dir = boost::filesystem::path(this->gamePath)
                                      / "resources" / "sprites" / this->saveFolder;
        if (!boost::filesystem::is_directory(dir))
            return;

        for (auto &entry : boost::filesystem::directory_iterator(dir))
            if (boost::filesystem::is_directory(entry))
                this->saveFolderList.push_back(entry.path().filename().string());
    }
    catch (...)
    {
    }

    std::sort(this->saveFolderList.begin(), this->saveFolderList.end());
}

void SpriteFusion::setSaveFolder(const std::string &path)
{
    try
    {
        boost::filesystem::path dir = boost::filesystem::path(this->gamePath)
                                      / "resources" / "sprites" / path;
        if (!boost::filesystem::is_directory(dir))
            return;

        if (path == this->saveFolder)
            return;

        this->saveFolder = path;
        this->refreshSaveFolderList();
        this->persistSaveFolder();
    }
    catch (...)
    {
    }
}

void SpriteFusion::persistSaveFolder()
{
    try
    {
        std::string optionsPath = this->gamePath + "/data/options/options.json";
        json options = this->loadJsonFile(optionsPath);
        if (!options.is_object())
            return;

        options["misc-sprite-fusion-folder"] = this->saveFolder;
        std::ofstream fileStream(optionsPath);
        if (!fileStream)
        {
            this->addLog("[warn] could not write " + optionsPath, 0xFFD070FF);
            return;
        }
        fileStream << options.dump(4);
    }
    catch (...)
    {
    }
}

void SpriteFusion::navigateSaveFolderUp()
{
    if (this->saveFolder.empty())
        return;
    size_t pos = this->saveFolder.find_last_of('/');
    this->setSaveFolder(pos == std::string::npos ? "" : this->saveFolder.substr(0, pos));
}

void SpriteFusion::createSaveFolder()
{
    std::string name = this->newFolderName;
    while (!name.empty() && (name.front() == ' ' || name.front() == '\t'))
        name.erase(name.begin());
    while (!name.empty() && (name.back() == ' ' || name.back() == '\t'))
        name.pop_back();

    if (name.empty() || name == "." || name == ".."
        || name.find('/') != std::string::npos
        || name.find('\\') != std::string::npos)
    {
        this->addLog("[error] invalid folder name", 0xFF6060FF);
        return;
    }

    std::string relative = this->saveFolder.empty() ? name : this->saveFolder + "/" + name;
    try
    {
        boost::filesystem::path dir = boost::filesystem::path(this->gamePath)
                                      / "resources" / "sprites" / relative;
        if (boost::filesystem::exists(dir))
        {
            this->addLog("[warn] folder already exists: " + relative, 0xFFD070FF);
            this->setSaveFolder(relative);
            return;
        }

        if (!boost::filesystem::create_directories(dir))
        {
            this->addLog("[error] could not create the folder: " + relative, 0xFF6060FF);
            return;
        }

        this->setSaveFolder(relative);
        this->newFolderName[0] = '\0';
        this->addLog("[ok] folder created: " + relative, 0x88CCFFFF);
    }
    catch (...)
    {
        this->addLog("[error] could not create the folder: " + relative, 0xFF6060FF);
    }
}


void SpriteFusion::rebuildMerge()
{
    this->mergeDirty = false;

    if (this->stack.empty())
    {
        this->mergeWidth = 0;
        this->mergeHeight = 0;
        return;
    }

    float factor = (float)this->scalePercent / 100.f;
    if (factor <= 0.f)
        factor = 0.01f;

    int maxWidth = 1;
    float totalHeight = 0.f;
    for (auto &item : this->stack)
    {
        maxWidth = std::max(maxWidth, item.width);
        totalHeight += (float)item.height;
    }

    int outW = std::max(1, (int)std::ceil((float)maxWidth * factor));
    int outH = std::max(1, (int)std::ceil(totalHeight * factor));

    if (!this->mergeTexture.create(outW, outH))
    {
        this->mergeWidth = 0;
        this->mergeHeight = 0;
        return;
    }

    this->mergeTexture.clear(sf::Color::Transparent);

    float yOffset = 0.f;
    for (auto &item : this->stack)
    {
        sf::Sprite sprite(item.texture);
        sprite.setScale(factor, factor);
        sprite.setPosition(0.f, yOffset);
        this->mergeTexture.draw(sprite);
        yOffset += (float)item.height * factor;
    }
    this->mergeTexture.display();

    this->mergeWidth = outW;
    this->mergeHeight = outH;
}

bool SpriteFusion::saveMerged()
{
    if (this->stack.empty())
    {
        this->addLog("[error] The stack is empty. Add images from the browser on the left "
                     "(double-click a PNG or select it and click Add).", 0xFF6060FF);
        return false;
    }

    this->rebuildMerge();
    if (this->mergeWidth <= 0 || this->mergeHeight <= 0)
    {
        this->addLog("[error] could not save the merged image", 0xFF6060FF);
        return false;
    }

    std::string name = this->outputName;
    while (!name.empty() && (name.front() == ' ' || name.front() == '\t'))
        name.erase(name.begin());
    while (!name.empty() && (name.back() == ' ' || name.back() == '\t'))
        name.pop_back();

    if (name.empty())
    {
        this->addLog("[error] inform a file name before saving", 0xFF6060FF);
        return false;
    }
    if (name.size() > 4 && name.compare(name.size() - 4, 4, ".png") == 0)
        name = name.substr(0, name.size() - 4);

    std::string outPath = this->gamePath + "/resources/sprites";
    if (!this->saveFolder.empty())
        outPath += "/" + this->saveFolder;
    outPath += "/" + name + ".png";

    try
    {
        boost::filesystem::path outDir = boost::filesystem::path(outPath).parent_path();
        if (!boost::filesystem::exists(outDir))
            boost::filesystem::create_directories(outDir);
    }
    catch (...)
    {
        this->addLog("[error] could not save the merged image: " + outPath, 0xFF6060FF);
        return false;
    }

    if (!this->mergeTexture.getTexture().copyToImage().saveToFile(outPath))
    {
        this->addLog("[error] could not save the merged image: " + outPath, 0xFF6060FF);
        return false;
    }

    this->addLog("[ok] merged image saved: " + outPath + " ("
                 + std::to_string(this->mergeWidth) + "x" + std::to_string(this->mergeHeight)
                 + " px)", 0x55FF88FF);
    return true;
}


void SpriteFusion::renderBrowser()
{
    float scale = this->imguiScale;

    ImGui::PushStyleColor(ImGuiCol_Text, GrimsolfTheme::textColor());
    ImGui::TextUnformatted("Sprites (resources/sprites)");
    ImGui::PopStyleColor();
    ImGui::SameLine();
    ImGui::SetCursorPosX(ImGui::GetCursorPosX() + 8 * scale);
    if (ImGui::SmallButton("Refresh"))
        this->refreshDirectory();
    ImGui::Separator();

    if (!this->currentPath.empty())
    {
        if (ImGui::Selectable("..", false))
            this->navigateUp();
        ImGui::Separator();
    }

    for (auto &folder : this->folderList)
    {
        ImGui::PushStyleColor(ImGuiCol_Text, GrimsolfTheme::gray(0.85f));
        bool entered = ImGui::Selectable(("[" + folder + "]").c_str(), false);
        ImGui::PopStyleColor();
        if (entered)
            this->navigateTo(this->currentPath.empty() ? folder : this->currentPath + "/" + folder);
    }

    if (!this->folderList.empty())
        ImGui::Separator();

    if (this->fileList.empty())
    {
        ImGui::PushStyleColor(ImGuiCol_Text, GrimsolfTheme::textColor());
        ImGui::TextWrapped("%s", "This folder has no PNG files.");
        ImGui::PopStyleColor();
    }

    for (size_t i = 0; i < this->fileList.size(); ++i)
    {
        const std::string &file = this->fileList[i];
        std::string relative = this->currentPath.empty() ? file : this->currentPath + "/" + file;
        bool selected = (relative == this->selectedFile);

        if (ImGui::Selectable(file.c_str(), selected))
            this->selectFile(relative);

        if (ImGui::IsItemHovered() && ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left))
            this->addToStack(relative);
    }

    ImGui::Separator();

    ImGui::PushStyleColor(ImGuiCol_Text, GrimsolfTheme::gray(0.80f));
    ImGui::TextWrapped("%s", this->selectedFileInfo.empty()
                             ? "double-click a PNG to add it, or select it and click Add"
                             : this->selectedFileInfo.c_str());
    ImGui::PopStyleColor();

    if (this->dropSupported)
    {
        ImGui::PushStyleColor(ImGuiCol_Text, GrimsolfTheme::textColor());
        ImGui::TextWrapped("%s", "You can also drag PNG files from the file explorer into this window.");
        ImGui::PopStyleColor();
    }

    if (ImGui::Button("Add", ImVec2(-1, 30 * scale)))
        this->addSelectedToStack();
}

void SpriteFusion::renderStack()
{
    float scale = this->imguiScale;

    ImGui::PushStyleColor(ImGuiCol_Text, GrimsolfTheme::textColor());
    ImGui::TextUnformatted("Fusion stack");
    ImGui::PopStyleColor();
    ImGui::SameLine();
    ImGui::TextDisabled("(%d)", (int)this->stack.size());
    ImGui::SameLine();
    ImGui::SetCursorPosX(ImGui::GetCursorPosX() + 8 * scale);
    if (ImGui::SmallButton("Add"))
        this->addSelectedToStack();
    ImGui::SameLine();
    if (ImGui::SmallButton("Clear all"))
        this->clearStack();
    if (ImGui::IsItemHovered())
        ImGui::SetTooltip("%s", "Clear all");
    ImGui::Separator();

    if (this->stack.empty())
    {
        ImGui::PushStyleColor(ImGuiCol_Text, GrimsolfTheme::textColor());
        ImGui::TextWrapped("%s", "The stack is empty. Add images from the browser on the left "
                                 "(double-click a PNG or select it and click Add).");
        ImGui::PopStyleColor();
        return;
    }

    ImGui::PushStyleColor(ImGuiCol_Text, GrimsolfTheme::textColor());
    ImGui::TextWrapped("%s", "Drag the thumbnails to reorder · ^ v to move · x to remove");
    ImGui::PopStyleColor();
    ImGui::Spacing();

    for (int i = 0; i < (int)this->stack.size(); ++i)
    {
        SpriteFusionItem &item = this->stack[i];
        ImGui::PushID(1000 + i);
        bool mutated = false;

        ImGui::BeginGroup();
        {
            float thumbBox = 52.f * scale;
            float fit = std::min(thumbBox / (float)item.width,
                                 thumbBox / (float)item.height);
            ImVec2 imgSize(std::max(1.f, (float)item.width * fit),
                           std::max(1.f, (float)item.height * fit));
            ImGui::Image(item.texture, sf::Vector2f(imgSize.x, imgSize.y));
            if (ImGui::BeginDragDropSource(ImGuiDragDropFlags_SourceAllowNullID))
            {
                ImGui::SetDragDropPayload("SPRITE-FUSION-ITEM", &i, sizeof(int));
                ImGui::Text("%s", item.name.c_str());
                ImGui::EndDragDropSource();
            }

            ImGui::SameLine();

            ImGui::PushStyleColor(ImGuiCol_Text, GrimsolfTheme::textColor());
            ImGui::TextUnformatted(item.name.c_str());
            ImGui::PopStyleColor();
            ImGui::TextDisabled("%dx%d px  #%d", item.width, item.height, i + 1);

            ImGui::SameLine();
            ImGui::SetCursorPosX(ImGui::GetCursorPosX()
                                 + std::max(0.f, ImGui::GetContentRegionAvail().x - 86.f * scale));
            if (i > 0 && ImGui::SmallButton("^"))
            {
                this->moveStackItem(i, -1);
                mutated = true;
            }
            ImGui::SameLine();
            if (i < (int)this->stack.size() - 1 && ImGui::SmallButton("v"))
            {
                this->moveStackItem(i, +1);
                mutated = true;
            }
            ImGui::SameLine();
            if (ImGui::SmallButton("x"))
            {
                this->removeFromStack(i);
                mutated = true;
            }
        }
        ImGui::EndGroup();

        if (!mutated && ImGui::BeginDragDropTarget())
        {
            if (const ImGuiPayload *payload = ImGui::AcceptDragDropPayload("SPRITE-FUSION-ITEM"))
            {
                int source = *(const int *)payload->Data;
                float midY = (ImGui::GetItemRectMin().y + ImGui::GetItemRectMax().y) * 0.5f;
                bool insertBefore = ImGui::GetMousePos().y < midY;
                if (source != i)
                {
                    this->reorderStack(source, i, insertBefore);
                    mutated = true;
                }
            }
            ImGui::EndDragDropTarget();
        }

        ImGui::PopID();

        if (mutated)
            return;
    }
}

void SpriteFusion::renderOptions()
{
    float scale = this->imguiScale;

    ImGui::PushStyleColor(ImGuiCol_Text, GrimsolfTheme::textColor());
    ImGui::TextUnformatted("Options");
    ImGui::PopStyleColor();
    ImGui::Separator();

    ImGui::SetNextItemWidth(120 * scale);
    if (ImGui::InputInt("Scale (%)", &this->scalePercent, 1, 10))
    {
        this->scalePercent = std::max(1, std::min(this->scalePercent, 1000));
        this->mergeDirty = true;
    }
    if (ImGui::IsItemHovered())
        ImGui::SetTooltip("%s", "Resizes the merged image. 100 = 100% (default).");

    ImGui::SetNextItemWidth(220 * scale);
    ImGui::InputText("File name", this->outputName,
                     sizeof(this->outputName));

    std::string outPathDisplay = "resources/sprites";
    if (!this->saveFolder.empty())
        outPathDisplay += "/" + this->saveFolder;
    outPathDisplay += "/" + std::string(this->outputName) + ".png";
    ImGui::PushStyleColor(ImGuiCol_Text, GrimsolfTheme::textColor());
    ImGui::TextWrapped("%s", outPathDisplay.c_str());
    ImGui::PopStyleColor();

    ImGui::Spacing();

    ImGui::PushStyleColor(ImGuiCol_Text, GrimsolfTheme::textColor());
    ImGui::TextUnformatted("Destination folder");
    ImGui::PopStyleColor();
    ImGui::SameLine();
    ImGui::SetCursorPosX(ImGui::GetCursorPosX() + 8 * scale);
    if (ImGui::SmallButton("Refresh"))
        this->refreshSaveFolderList();
    if (ImGui::IsItemHovered())
        ImGui::SetTooltip("%s", "The merged image is saved into this folder, relative to resources/sprites.");

    std::string folderDisplay = "resources/sprites";
    if (!this->saveFolder.empty())
        folderDisplay += "/" + this->saveFolder;
    ImGui::PushStyleColor(ImGuiCol_Text, GrimsolfTheme::gray(0.90f));
    ImGui::TextWrapped("%s/", folderDisplay.c_str());
    ImGui::PopStyleColor();

    ImGui::BeginChild("##sf-dest-folders", ImVec2(0, 96 * scale), true);
    {
        if (!this->saveFolder.empty())
        {
            if (ImGui::Selectable("..", false))
                this->navigateSaveFolderUp();
            ImGui::Separator();
        }

        if (this->saveFolderList.empty())
        {
            ImGui::PushStyleColor(ImGuiCol_Text, GrimsolfTheme::textColor());
            ImGui::TextWrapped("%s", "This folder has no subfolders.");
            ImGui::PopStyleColor();
        }

        for (auto &folder : this->saveFolderList)
        {
            ImGui::PushStyleColor(ImGuiCol_Text, GrimsolfTheme::gray(0.85f));
            bool entered = ImGui::Selectable(("[" + folder + "]").c_str(), false);
            ImGui::PopStyleColor();
            if (entered)
                this->setSaveFolder(this->saveFolder.empty() ? folder : this->saveFolder + "/" + folder);
        }
    }
    ImGui::EndChild();

    ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x - 76 * scale);
    ImGui::InputTextWithHint("##sf-newfolder", "New folder",
                             this->newFolderName, sizeof(this->newFolderName));
    ImGui::SameLine();
    if (ImGui::Button("Create", ImVec2(68 * scale, 0)))
        this->createSaveFolder();

    ImGui::Spacing();

    if (this->mergeWidth > 0)
    {
        ImGui::TextDisabled("%s: %dx%d px", "merged size",
                            this->mergeWidth, this->mergeHeight);
    }

    ImGui::Spacing();

    if (ImGui::Button("Save PNG", ImVec2(140 * scale, 34 * scale)))
        this->saveMerged();
    ImGui::SameLine();
    if (ImGui::Button("Clear all", ImVec2(140 * scale, 34 * scale)))
        this->clearStack();
}

void SpriteFusion::renderPreview()
{
    float scale = this->imguiScale;

    ImGui::PushStyleColor(ImGuiCol_Text, GrimsolfTheme::textColor());
    ImGui::TextUnformatted("Preview");
    ImGui::PopStyleColor();
    ImGui::SameLine();
    ImGui::TextDisabled("(%d%%)", this->scalePercent);
    ImGui::Separator();

    if (this->mergeDirty)
        this->rebuildMerge();

    float availW = ImGui::GetContentRegionAvail().x;
    float availH = ImGui::GetContentRegionAvail().y;
    if (availW <= 0.f || availH <= 0.f)
        return;

    ImGui::PushStyleColor(ImGuiCol_ChildBg, GrimsolfTheme::gray(0.035f));
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
    ImGui::BeginChild("##sf-preview-viewport", ImVec2(availW, availH), true,
                      ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);
    ImGui::PopStyleVar();
    ImGui::PopStyleColor();

    if (this->mergeWidth > 0 && this->mergeHeight > 0)
    {
        ImVec2 base = ImGui::GetCursorScreenPos();
        ImDrawList *drawList = ImGui::GetWindowDrawList();
        const float tile = 12.f * scale;
        int cols = (int)std::ceil(availW / tile);
        int rows = (int)std::ceil(availH / tile);
        for (int y = 0; y < rows; ++y)
            for (int x = 0; x < cols; ++x)
            {
                ImU32 color = ((x + y) % 2 == 0) ? GrimsolfTheme::u32(0.10f)
                                                 : GrimsolfTheme::u32(0.07f);
                drawList->AddRectFilled(ImVec2(base.x + x * tile, base.y + y * tile),
                                        ImVec2(base.x + (x + 1) * tile, base.y + (y + 1) * tile),
                                        color);
            }

        float fit = std::min(availW / (float)this->mergeWidth,
                             availH / (float)this->mergeHeight);
        fit = std::max(0.01f, std::min(fit, 8.f));
        ImVec2 imgSize(std::max(1.f, (float)this->mergeWidth * fit),
                       std::max(1.f, (float)this->mergeHeight * fit));
        ImGui::SetCursorPos(ImVec2((availW - imgSize.x) * 0.5f, (availH - imgSize.y) * 0.5f));
        ImGui::Image(this->mergeTexture, sf::Vector2f(imgSize.x, imgSize.y));
    }
    else
    {
        ImGui::PushStyleColor(ImGuiCol_Text, GrimsolfTheme::textColor());
        ImGui::TextWrapped("%s", "The stack is empty. Add images from the browser on the left "
                                 "(double-click a PNG or select it and click Add).");
        ImGui::PopStyleColor();
    }

    ImGui::EndChild();
}

void SpriteFusion::renderLog()
{
    float scale = this->imguiScale;

    ImGui::PushStyleColor(ImGuiCol_Text, GrimsolfTheme::textColor());
    ImGui::TextUnformatted("Log");
    ImGui::PopStyleColor();
    ImGui::SameLine();
    ImGui::SetCursorPosX(ImGui::GetCursorPosX() + 8 * scale);
    if (ImGui::SmallButton("Clear all"))
        this->logLines.clear();
    ImGui::Separator();

    ImGui::BeginChild("##sf-log", ImVec2(0, 0), true);
    for (auto &line : this->logLines)
        ImGui::TextColored(ImVec4((line.color >> 16 & 0xFF) / 255.f,
                                  (line.color >> 8 & 0xFF) / 255.f,
                                  (line.color & 0xFF) / 255.f,
                                  1.0f), "%s", line.text.c_str());
    if (this->logDirty)
    {
        ImGui::SetScrollHereY(1.0f);
        this->logDirty = false;
    }
    ImGui::EndChild();
}


bool SpriteFusion::update(float timer)
{
    ImVec2 displaySize = ImGui::GetIO().DisplaySize;
    this->viewportW = std::max(1.f, displaySize.x);
    this->viewportH = std::max(1.f, displaySize.y);
    this->imguiScale = std::max(0.5f, std::min(3.0f, this->viewportW / this->designWidth));

    ImGui::PushFont(this->fontNormal);
    ImGui::GetIO().FontGlobalScale = this->imguiScale;

    ImVec2 windowSize(this->viewportW, this->viewportH);
    ImGui::SetNextWindowSize(windowSize);
    ImGui::SetNextWindowPos(ImVec2(0.f, 0.f), ImGuiCond_Always);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(8 * this->imguiScale, 6 * this->imguiScale));

    ImGui::Begin("##sprite-fusion", nullptr,
        ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize |
        ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoSavedSettings);
    ImGui::PushStyleColor(ImGuiCol_WindowBg, GrimsolfTheme::gray(0.05f));
    ImGui::PushStyleColor(ImGuiCol_Text, GrimsolfTheme::textColor());
    ImGui::PushStyleColor(ImGuiCol_Button, GrimsolfTheme::gray(0.16f));
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, GrimsolfTheme::gray(0.27f));
    ImGui::PushStyleColor(ImGuiCol_ButtonActive, GrimsolfTheme::gray(0.32f));
    ImGui::PushStyleColor(ImGuiCol_FrameBg, GrimsolfTheme::gray(0.12f));
    ImGui::PushStyleColor(ImGuiCol_FrameBgHovered, GrimsolfTheme::gray(0.20f));
    ImGui::PushStyleColor(ImGuiCol_CheckMark, GrimsolfTheme::gray(1.0f));
    ImGui::PushStyleColor(ImGuiCol_Header, GrimsolfTheme::gray(0.24f));
    ImGui::PushStyleColor(ImGuiCol_HeaderHovered, GrimsolfTheme::gray(0.30f));

    bool leaveRequested = false;

    {
        if (this->fontBig)
        {
            ImGui::PushFont(this->fontBig);
            ImGui::TextUnformatted("Sprite Fusion");
            ImGui::PopFont();
        }
        else
        {
            ImGui::TextUnformatted("Sprite Fusion");
        }
        ImGui::SameLine();
        ImGui::SetCursorPosX(ImGui::GetCursorPosX() + 10 * this->imguiScale);
        ImGui::TextDisabled("resources/sprites");
        ImGui::SameLine();
        ImGui::SetCursorPosX(ImGui::GetWindowContentRegionMax().x - 150 * this->imguiScale);
        if (ImGui::Button("Back to Studio", ImVec2(140 * this->imguiScale, 30 * this->imguiScale)))
            leaveRequested = true;
        ImGui::Separator();
    }

    float logH = 130 * this->imguiScale;
    float colsH = std::max(50.f, ImGui::GetContentRegionAvail().y - logH - 12 * this->imguiScale);
    float browserW = 250 * this->imguiScale;
    float stackW = 360 * this->imguiScale;
    float previewW = std::max(100.f, ImGui::GetContentRegionAvail().x - browserW - stackW - 20 * this->imguiScale);

    ImGui::BeginChild("##sf-browser", ImVec2(browserW, colsH), true);
    this->renderBrowser();
    ImGui::EndChild();

    ImGui::SameLine();

    ImGui::BeginChild("##sf-stack", ImVec2(stackW, colsH), true);
    this->renderStack();
    ImGui::EndChild();

    ImGui::SameLine();

    ImGui::BeginChild("##sf-right", ImVec2(previewW, colsH), true);
    this->renderOptions();
    ImGui::Separator();
    this->renderPreview();
    ImGui::EndChild();

    ImGui::Separator();

    ImGui::BeginChild("##sf-log-region", ImVec2(0, logH), true);
    this->renderLog();
    ImGui::EndChild();

    ImGui::PopStyleColor(10);
    ImGui::End();
    ImGui::PopStyleVar(3);
    ImGui::PopFont();

    return leaveRequested;
}
