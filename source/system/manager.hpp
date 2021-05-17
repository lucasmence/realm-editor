#include <list>
#include <memory>
#include <iostream>
#include <SFML/Graphics.hpp>
#include <SFML/Audio.hpp>
#include "object/viewElement.hpp"
#include "text/label.hpp"
#include "model/model.hpp"
#include "hud/hud.hpp"
#include "palette/palette.hpp"

#pragma once

#ifndef MANAGER_HPP
#define MANAGER_HPP

struct ManagerList
{
	std::list<std::shared_ptr<ViewElement>> viewElements;
	std::list<std::shared_ptr<Texture>> textures;
};

class Manager
{
	public:
		std::shared_ptr<sf::RenderWindow> window;
		std::shared_ptr<sf::Font> font;
		std::shared_ptr<Hud> hud;
		std::shared_ptr<Palette> palette;
		ManagerList list;

		bool hasFocus;
		bool open;

		Manager();
		~Manager();
		bool unloadAll();
		bool loadWindowOpening();

		bool update();
		bool event();
		bool eventClick(sf::Event& event);
		bool addView(std::shared_ptr<ViewElement> element);
		bool removeView(std::shared_ptr<ViewElement> element);
		bool addViewElement(std::shared_ptr<ViewElement> element);
		sf::Vector2f getMousePosition();
};

#endif