#pragma once

#ifndef SPRITEFUSION_HPP
#define SPRITEFUSION_HPP

#include <string>
#include <vector>
#include <SFML/Graphics.hpp>
#include "../library/json.hpp"

struct ImFont;

// One image in the fusion stack (in the exact order they will be merged).
struct SpriteFusionItem
{
    std::string name;      // e.g. "barbarian.png"
    std::string relative;  // path relative to resources/sprites
    sf::Texture texture;
    int width = 0;
    int height = 0;
    bool loaded = false;
};

// ---------------------------------------------------------------------------
// SpriteFusion - merges sprite images into a vertical sprite sheet.
//
// realm-editor tool (own window, launched by Grimsolf with --sprite-fusion
// from Studio -> Sprite Fusion). It works like the duq-sprite-fusion web
// tool: images picked from resources/sprites (the same browser used by the
// in-game Sprite Studio) are stacked vertically, can be reordered with drag
// & drop or the up/down buttons, previewed merged, resized with a scale
// percentage (100 = 100%, default) and saved as a single .png into a
// destination folder of your choice under resources/sprites (folder browser
// in the options panel, with the option to create new folders).
//
// The game folder (--game-path) is where resources/sprites and the language
// files are read from; the working directory stays the realm-editor folder.
// ---------------------------------------------------------------------------
class SpriteFusion
{
    public:
        SpriteFusion(const std::string &gamePath, ImFont *fontNormal, ImFont *fontBig,
                     bool dropSupported = false);
        ~SpriteFusion();

        // Renders the tool UI for one frame. Returns true when the user left
        // the tool (BACK button), closing the process so Grimsolf restores
        // its own window (Menu::updateWorldEditor).
        bool update(float timer);

        // Adds PNG files dropped onto the window from the OS file explorer
        // (Windows WM_DROPFILES). Absolute paths; non-PNG files are ignored
        // with a log entry.
        void addExternalFiles(const std::vector<std::string> &paths);

    private:
        struct LogLine
        {
            std::string text;
            unsigned int color; // packed IM_COL32
        };

        std::string gamePath;
        json languageFile;

        ImFont *fontNormal = nullptr;
        ImFont *fontBig = nullptr;
        float imguiScale = 1.f;
        float viewportW = 1920.f;
        float viewportH = 1080.f;
        float designWidth = 1920.f;

        // Browser (same layout as the game's Sprite Studio browser)
        std::string currentPath; // folder relative to resources/sprites
        std::vector<std::string> folderList;
        std::vector<std::string> fileList;
        std::string selectedFile;      // relative path of the selected PNG
        std::string selectedFileInfo;  // "name (WxH px)"

        // Fusion stack (top to bottom = first to last image)
        std::vector<SpriteFusionItem> stack;
        bool dropSupported = false; // OS file drag & drop into the window

        // Options
        int scalePercent = 100;                    // resize % (100 = 100%)
        char outputName[128] = "sprite-fusion";

        // Destination folder (relative to resources/sprites; "" = root)
        std::string saveFolder;
        std::vector<std::string> saveFolderList;   // subfolders of saveFolder
        char newFolderName[128] = "";

        // Log
        std::vector<LogLine> logLines;
        bool logDirty = false;

        // Merged preview
        sf::RenderTexture mergeTexture;
        bool mergeDirty = true;
        int mergeWidth = 0;
        int mergeHeight = 0;

        // Browser helpers
        void refreshDirectory();
        void navigateTo(const std::string &path);
        void navigateUp();
        void selectFile(const std::string &relative);
        void addSelectedToStack();

        // Destination folder helpers (relative to resources/sprites)
        void refreshSaveFolderList();
        void setSaveFolder(const std::string &path);
        void navigateSaveFolderUp();
        void createSaveFolder();
        void persistSaveFolder();

        // Stack helpers
        void addToStack(const std::string &relative);
        void addImageFromPath(const std::string &fullPath);
        void removeFromStack(int index);
        void moveStackItem(int index, int direction);
        void reorderStack(int sourceIndex, int targetIndex, bool insertBefore);
        void clearStack();

        // Merge / save
        void rebuildMerge();
        bool saveMerged();

        // Misc
        std::string getLanguage(const std::string &field) const;
        void addLog(const std::string &text, unsigned int color);
        json loadJsonFile(const std::string &path) const;
        bool readPngSize(const std::string &path, int &width, int &height) const;

        // Rendering
        void renderBrowser();
        void renderStack();
        void renderOptions();
        void renderPreview();
        void renderLog();
};

#endif // SPRITEFUSION_HPP
