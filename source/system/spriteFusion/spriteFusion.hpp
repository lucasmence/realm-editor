#pragma once

#ifndef SPRITEFUSION_HPP
#define SPRITEFUSION_HPP

#include <string>
#include <vector>
#include <SFML/Graphics.hpp>
#include "../library/json.hpp"

struct ImFont;

struct SpriteFusionItem
{
    std::string name;
    std::string relative;
    sf::Texture texture;
    int width = 0;
    int height = 0;
    bool loaded = false;
};

class SpriteFusion
{
    public:
        SpriteFusion(const std::string &gamePath, ImFont *fontNormal, ImFont *fontBig,
                     bool dropSupported = false);
        ~SpriteFusion();

        bool update(float timer);

        void addExternalFiles(const std::vector<std::string> &paths);

    private:
        struct LogLine
        {
            std::string text;
            unsigned int color;
        };

        std::string gamePath;

        ImFont *fontNormal = nullptr;
        ImFont *fontBig = nullptr;
        float imguiScale = 1.f;
        float viewportW = 1920.f;
        float viewportH = 1080.f;
        float designWidth = 1920.f;

        std::string currentPath;
        std::vector<std::string> folderList;
        std::vector<std::string> fileList;
        std::string selectedFile;
        std::string selectedFileInfo;

        std::vector<SpriteFusionItem> stack;
        bool dropSupported = false;

        int scalePercent = 100;
        char outputName[128] = "sprite-fusion";

        std::string saveFolder;
        std::vector<std::string> saveFolderList;
        char newFolderName[128] = "";

        std::vector<LogLine> logLines;
        bool logDirty = false;

        sf::RenderTexture mergeTexture;
        bool mergeDirty = true;
        int mergeWidth = 0;
        int mergeHeight = 0;

        void refreshDirectory();
        void navigateTo(const std::string &path);
        void navigateUp();
        void selectFile(const std::string &relative);
        void addSelectedToStack();

        void refreshSaveFolderList();
        void setSaveFolder(const std::string &path);
        void navigateSaveFolderUp();
        void createSaveFolder();
        void persistSaveFolder();

        void addToStack(const std::string &relative);
        void addImageFromPath(const std::string &fullPath);
        void removeFromStack(int index);
        void moveStackItem(int index, int direction);
        void reorderStack(int sourceIndex, int targetIndex, bool insertBefore);
        void clearStack();

        void rebuildMerge();
        bool saveMerged();

        void addLog(const std::string &text, unsigned int color);
        json loadJsonFile(const std::string &path) const;
        bool readPngSize(const std::string &path, int &width, int &height) const;

        void renderBrowser();
        void renderStack();
        void renderOptions();
        void renderPreview();
        void renderLog();
};

#endif
