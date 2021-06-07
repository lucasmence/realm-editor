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
#include "map/map.hpp"

#pragma once

#ifndef MANAGER_HPP
#define MANAGER_HPP

struct ManagerConstant
{
	std::string fontFilePath;
	std::vector<int> gridSize;
	std::vector<int> brushSize;
};

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
		std::shared_ptr<sf::View> canvas;
		std::shared_ptr<Map> map;
		ManagerList list;
		ManagerConstant constant;

		sf::Vector2f canvasPosition;
		sf::Image icon;

		bool hasFocus;
		bool open;
		std::string appName;

		Manager();
		~Manager();
		bool unloadAll();
		bool loadWindowOpening();

		bool update();
		bool event();
		bool eventClick(sf::Event& event);
		bool eventKey(sf::Event& event);
		bool eventType(sf::Event& event);
		bool eventMouseReleased(sf::Event& event);
		bool eventMouseMoved(sf::Event& event);
		bool addView(std::shared_ptr<ViewElement> element);
		bool removeView(std::shared_ptr<ViewElement> element);
		bool addViewElement(std::shared_ptr<ViewElement> element);
		bool moveCanvas(sf::Vector2f position);
		bool setCanvasCenter(sf::Vector2f position);
		bool setCanvas();
		bool loadConstants();
		bool display();
		std::shared_ptr<Texture> getTexture(std::string filename);
		std::string setTitle(std::string value);
		sf::Vector2f getMousePosition();
};

#endif