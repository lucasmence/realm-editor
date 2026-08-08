#include "triggerApp.hpp"
#include "triggerEditor.hpp"
#include "../external/imgui/imgui.h"
#include "../external/imgui/imgui-SFML.h"
#include "../library/json.hpp"

#include <cstdlib>

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

TriggerApp::TriggerApp(const std::string &gamePath)
{
    // The editor was designed for a 1920x1080 window; opening it at desktop
    // size gives it room to breathe (the UI scales with the viewport width).
    sf::VideoMode desktop = sf::VideoMode::getDesktopMode();
    this->window = std::make_shared<sf::RenderWindow>(desktop, "realm-editor - Trigger Editor", sf::Style::Default);
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

    this->triggerEditor = new TriggerEditor(gamePath, fontNormal, fontBig);
}

TriggerApp::~TriggerApp()
{
    delete this->triggerEditor;
    this->triggerEditor = nullptr;
    this->window->close();
    ImGui::SFML::Shutdown();
}

int TriggerApp::run()
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

        ImGui::SFML::Update(*this->window, this->clock.restart());

        this->window->clear(sf::Color(20, 20, 28));

        // update() returns true when the user left the editor (BACK button or
        // map-select stage): close the process so Grimsolf restores its own
        // window (Menu::updateWorldEditor).
        if (this->triggerEditor->update(0.f))
            this->closeRequested = true;

        ImGui::SFML::Render(*this->window);
        this->window->display();
    }

    return 0;
}
