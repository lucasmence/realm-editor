#include "spriteFusionApp.hpp"
#include "spriteFusion.hpp"
#include "../external/imgui/imgui.h"
#include "../external/imgui/imgui-SFML.h"
#include "../library/json.hpp"

#include <cstdlib>
#include <vector>
#include <string>
#include <utility>

#ifdef _WIN32
#include <windows.h>
#include <shellapi.h>
#endif

#ifdef _WIN32
// ---------------------------------------------------------------------------
// OS file drag & drop (Windows Explorer -> window).
//
// SFML 2.5.1 does not expose a dropped-file event, so the window procedure
// is subclassed (SetWindowLongPtr) to intercept WM_DROPFILES after
// DragAcceptFiles enables the drop target. The dropped paths are queued and
// drained once per frame in SpriteFusionApp::run, so the SFML event loop is
// not disturbed.
// ---------------------------------------------------------------------------
static WNDPROC g_originalWndProc = nullptr;
static std::vector<std::string> g_droppedFiles;

static std::string wideToUtf8(const std::wstring &wstr)
{
    if (wstr.empty())
        return "";
    int size = WideCharToMultiByte(CP_UTF8, 0, wstr.c_str(), (int)wstr.size(),
                                   nullptr, 0, nullptr, nullptr);
    std::string out(size, '\0');
    WideCharToMultiByte(CP_UTF8, 0, wstr.c_str(), (int)wstr.size(),
                        &out[0], size, nullptr, nullptr);
    return out;
}

static LRESULT CALLBACK spriteFusionWndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    if (message == WM_DROPFILES)
    {
        HDROP drop = reinterpret_cast<HDROP>(wParam);
        const UINT count = DragQueryFile(drop, 0xFFFFFFFF, nullptr, 0);
        for (UINT i = 0; i < count; ++i)
        {
            // DragQueryFile returns the length EXCLUDING the null terminator,
            // while cch must include it: the buffer needs length + 1.
            const UINT length = DragQueryFileW(drop, i, nullptr, 0);
            if (length == 0)
                continue;

            std::wstring path(length + 1, L'\0');
            DragQueryFileW(drop, i, &path[0], length + 1);
            path.resize(length); // strip the trailing null before UTF-8

            // Folders cannot be added; only files are queued.
            DWORD attributes = GetFileAttributesW(path.c_str());
            if (attributes != INVALID_FILE_ATTRIBUTES
                && (attributes & FILE_ATTRIBUTE_DIRECTORY) == 0)
                g_droppedFiles.push_back(wideToUtf8(path));
        }
        DragFinish(drop);
        return 0;
    }
    return CallWindowProc(g_originalWndProc, hWnd, message, wParam, lParam);
}
#endif

// Reads a "DATA" entry from the game's constants.json (the same layout
// Grimsolf uses: [ { "key": "FONT-TYPE", "value": "resources/fonts/osd_mono.ttf" } ]).
static std::string getConstantValue(const json &file, const std::string &key)
{
    if (!file.is_object() || !file.contains("DATA") || !file["DATA"].is_array())
        return "";
    for (const auto &item : file["DATA"])
        if (item.is_object() && item.value("key", "") == key)
            return item.value("value", "");
    return "";
}

SpriteFusionApp::SpriteFusionApp(const std::string &gamePath)
{
    // The tool was designed for a 1920x1080 window; opening it at desktop
    // size gives it room to breathe (the UI scales with the viewport width).
    sf::VideoMode desktop = sf::VideoMode::getDesktopMode();
    this->window = std::make_shared<sf::RenderWindow>(desktop, "realm-editor - Sprite Fusion", sf::Style::Default);
    ImGui::SFML::Init(*this->window);

    // Optional icon (realm-editor.png sits next to the executable folder
    // layout; skip it when missing or 0x0 to avoid X server crashes).
    sf::Image icon;
    if (icon.loadFromFile("realm-editor.png")
        && icon.getSize().x > 0 && icon.getSize().y > 0)
        this->window->setIcon(icon.getSize().x, icon.getSize().y, icon.getPixelsPtr());

    // Register the game font with the same sizes the in-game menus use
    // (constants.json FONT-MENU-IMGUI / FONT-MENU-IMGUI-BIG), falling back to
    // the ImGui default font when the game font cannot be loaded.
    ImGuiIO &io = ImGui::GetIO();
    ImFont *fontNormal = nullptr;
    ImFont *fontBig = nullptr;

    json constants = Json::loadFromFile(gamePath + "/data/options/constants.json");
    std::string fontPath = gamePath + "/" + getConstantValue(constants, "FONT-TYPE");
    int sizeNormal = std::atoi(getConstantValue(constants, "FONT-MENU-IMGUI").c_str());
    int sizeBig = std::atoi(getConstantValue(constants, "FONT-MENU-IMGUI-BIG").c_str());
    if (sizeNormal <= 0)
        sizeNormal = 16;
    if (sizeBig <= 0)
        sizeBig = 20;

    if (!fontPath.empty())
    {
        fontNormal = io.Fonts->AddFontFromFileTTF(fontPath.c_str(), (float)sizeNormal);
        fontBig = io.Fonts->AddFontFromFileTTF(fontPath.c_str(), (float)sizeBig);
    }
    if (fontBig == nullptr)
        fontBig = fontNormal;
    if (fontNormal == nullptr)
    {
        fontNormal = io.Fonts->AddFontDefault();
        fontBig = fontNormal;
    }
    ImGui::SFML::UpdateFontTexture();

#ifdef _WIN32
    // Enable dropping files onto this window and subclass its procedure to
    // receive WM_DROPFILES (SFML 2.5 has no dropped-file event).
    HWND hwnd = this->window->getSystemHandle();
    if (hwnd != nullptr)
    {
        g_droppedFiles.clear();
        g_originalWndProc = reinterpret_cast<WNDPROC>(SetWindowLongPtr(
            hwnd, GWLP_WNDPROC, reinterpret_cast<LONG_PTR>(spriteFusionWndProc)));
        // Only enable the drop target when the subclass actually took effect.
        if (g_originalWndProc != nullptr)
            DragAcceptFiles(hwnd, TRUE);
    }
#endif

    this->spriteFusion = new SpriteFusion(gamePath, fontNormal, fontBig,
#ifdef _WIN32
                                          true
#else
                                          false
#endif
    );
}

SpriteFusionApp::~SpriteFusionApp()
{
#ifdef _WIN32
    // Restore the original window procedure and disable the drop target.
    HWND hwnd = this->window->getSystemHandle();
    if (hwnd != nullptr)
    {
        DragAcceptFiles(hwnd, FALSE);
        if (g_originalWndProc != nullptr)
            SetWindowLongPtr(hwnd, GWLP_WNDPROC,
                             reinterpret_cast<LONG_PTR>(g_originalWndProc));
        g_originalWndProc = nullptr;
        g_droppedFiles.clear();
    }
#endif

    delete this->spriteFusion;
    this->spriteFusion = nullptr;
    this->window->close();
    ImGui::SFML::Shutdown();
}

int SpriteFusionApp::run()
{
    while (this->window->isOpen() && !this->closeRequested)
    {
        sf::Event event;
        while (this->window->pollEvent(event))
        {
            ImGui::SFML::ProcessEvent(event);
            if (event.type == sf::Event::Closed)
                this->closeRequested = true;
        }

#ifdef _WIN32
        // Drain the files dropped since the last frame into the fusion stack.
        if (!g_droppedFiles.empty())
        {
            std::vector<std::string> drops = std::move(g_droppedFiles);
            g_droppedFiles.clear();
            this->spriteFusion->addExternalFiles(drops);
        }
#endif

        ImGui::SFML::Update(*this->window, this->clock.restart());

        this->window->clear(sf::Color(20, 20, 28));

        // update() returns true when the user left the tool (BACK button):
        // close the process so Grimsolf restores its own window
        // (Menu::updateWorldEditor).
        if (this->spriteFusion->update(0.f))
            this->closeRequested = true;

        ImGui::SFML::Render(*this->window);
        this->window->display();
    }

    return 0;
}
