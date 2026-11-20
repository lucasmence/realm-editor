#pragma once

#ifndef TRIGGERAPP_HPP
#define TRIGGERAPP_HPP

#include <memory>
#include <string>
#include <SFML/Graphics.hpp>

class TriggerEditor;

class TriggerApp
{
    public:
        TriggerApp(const std::string &gamePath, const std::string &hudTheme = "");
        ~TriggerApp();

        int run();

    private:
        std::shared_ptr<sf::RenderWindow> window;
        TriggerEditor *triggerEditor = nullptr;
        sf::Clock clock;
        bool closeRequested = false;
};

#endif
