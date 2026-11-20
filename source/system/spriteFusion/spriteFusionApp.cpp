#include "spriteFusionApp.hpp"
#include "spriteFusion.hpp"
#include "../external/imgui/imgui.h"
#include "../external/imgui/imgui-SFML.h"
#include "../library/json.hpp"
#include "../grimsolfTheme.hpp"

#include <cstdlib>
#include <vector>
#include <string>
#include <utility>

#ifdef _WIN32
#include <windows.h>
#include <shellapi.h>
#endif

#ifdef _WIN32
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
            const UINT length = DragQueryFileW(drop, i, nullptr, 0);
            if (length == 0)
                continue;

            std::wstring path(length + 1, L'\0');
            DragQueryFileW(drop, i, &path[0], length + 1);
            path.resize(length);

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

static std::string getConstantValue(const json &file, const std::string &key)
{
    if (!file.is_object() || !file.contains("DATA") || !file["DATA"].is_array())
        return "";
    for (const auto &item : file["DATA"])
        if (item.is_object() && item.value("key", "") == key)
            return item.value("value", "");
    return "";
}

SpriteFusionApp::SpriteFusionApp(const std::string &gamePath, const std::string &hudTheme)
{
    sf::VideoMode desktop = sf::VideoMode::getDesktopMode();
    this->window = std::make_shared<sf::RenderWindow>(desktop, "realm-editor - Sprite Fusion", sf::Style::Default);
    ImGui::SFML::Init(*this->window);
    if (hudTheme.empty())
        GrimsolfTheme::applySaved();
    else
        GrimsolfTheme::apply(hudTheme);


    sf::Image icon;
    if (icon.loadFromFile("realm-editor.png")
        && icon.getSize().x > 0 && icon.getSize().y > 0)
        this->window->setIcon(icon.getSize().x, icon.getSize().y, icon.getPixelsPtr());

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
    HWND hwnd = this->window->getSystemHandle();
    if (hwnd != nullptr)
    {
        g_droppedFiles.clear();
        g_originalWndProc = reinterpret_cast<WNDPROC>(SetWindowLongPtr(
            hwnd, GWLP_WNDPROC, reinterpret_cast<LONG_PTR>(spriteFusionWndProc)));
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
        if (!g_droppedFiles.empty())
        {
            std::vector<std::string> drops = std::move(g_droppedFiles);
            g_droppedFiles.clear();
            this->spriteFusion->addExternalFiles(drops);
        }
#endif

        ImGui::SFML::Update(*this->window, this->clock.restart());

        ImGui::GetStyle().Colors[ImGuiCol_Text] = GrimsolfTheme::textColor();
        ImGui::GetStyle().Colors[ImGuiCol_TextDisabled] = GrimsolfTheme::textDisabledColor();

        this->window->clear(sf::Color(12, 12, 12));

        if (this->spriteFusion->update(0.f))
            this->closeRequested = true;

        ImGui::SFML::Render(*this->window);
        this->window->display();
    }

    return 0;
}
