#pragma once

#ifndef SPRITEFUSIONAPP_HPP
#define SPRITEFUSIONAPP_HPP

#include <memory>
#include <string>
#include <SFML/Graphics.hpp>

class SpriteFusion;

// ---------------------------------------------------------------------------
// SpriteFusionApp - standalone host for the Sprite Fusion tool.
//
// The realm-editor binary can be launched by Grimsolf with --sprite-fusion,
// which opens exclusively the Sprite Fusion tool in its own window (the map
// editor UI is not created at all). This small app owns the window, the
// ImGui context and the SpriteFusion instance, and closes the process as
// soon as the tool reports that the user left it (BACK button), so the game
// can restore its own window (Menu::updateWorldEditor).
//
// The game folder (--game-path) is where resources/sprites and the language
// files are read from; the working directory stays the realm-editor folder,
// so the editor's own runtime data (data/options/realm-editor.json, font,
// icon) resolves like in the map editor.
// ---------------------------------------------------------------------------
class SpriteFusionApp
{
    public:
        SpriteFusionApp(const std::string &gamePath);
        ~SpriteFusionApp();

        // Runs the tool loop until the window is closed or the user leaves
        // the tool. Returns the process exit code.
        int run();

    private:
        std::shared_ptr<sf::RenderWindow> window;
        SpriteFusion *spriteFusion = nullptr;
        sf::Clock clock;
        bool closeRequested = false;
};

#endif // SPRITEFUSIONAPP_HPP
