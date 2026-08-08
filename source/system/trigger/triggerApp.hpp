#pragma once

#ifndef TRIGGERAPP_HPP
#define TRIGGERAPP_HPP

#include <memory>
#include <string>
#include <SFML/Graphics.hpp>

class TriggerEditor;

// ---------------------------------------------------------------------------
// TriggerApp - standalone host for the Trigger Editor.
//
// The realm-editor binary can be launched by Grimsolf with --trigger-editor,
// which opens exclusively the Trigger Editor in its own window (the map
// editor UI is not created at all). This small app owns the window, the
// ImGui context and the TriggerEditor instance, and closes the process as
// soon as the editor reports that the user left it (BACK button), so the
// game can restore its own window (Menu::updateWorldEditor).
//
// The game folder (--game-path) is where the maps and language files are
// read from; the working directory stays the realm-editor folder, so the
// editor's own runtime data (data/options/realm-editor.json, font, icon)
// resolves like in the map editor.
// ---------------------------------------------------------------------------
class TriggerApp
{
    public:
        TriggerApp(const std::string &gamePath);
        ~TriggerApp();

        // Runs the editor loop until the window is closed or the user leaves
        // the editor. Returns the process exit code.
        int run();

    private:
        std::shared_ptr<sf::RenderWindow> window;
        TriggerEditor *triggerEditor = nullptr;
        sf::Clock clock;
        bool closeRequested = false;
};

#endif // TRIGGERAPP_HPP
