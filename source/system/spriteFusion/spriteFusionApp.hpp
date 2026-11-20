#pragma once

#ifndef SPRITEFUSIONAPP_HPP
#define SPRITEFUSIONAPP_HPP

#include <memory>
#include <string>
#include <SFML/Graphics.hpp>

class SpriteFusion;

class SpriteFusionApp
{
    public:
        SpriteFusionApp(const std::string &gamePath, const std::string &hudTheme = "");
        ~SpriteFusionApp();

        int run();

    private:
        std::shared_ptr<sf::RenderWindow> window;
        SpriteFusion *spriteFusion = nullptr;
        sf::Clock clock;
        bool closeRequested = false;
};

#endif
