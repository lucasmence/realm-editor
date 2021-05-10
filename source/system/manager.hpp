#include <list>
#include <memory>
#include <iostream>
#include <SFML/Graphics.hpp>
#include <SFML/Audio.hpp>
#include "text/text.hpp"
#include "model/model.hpp"
#include "text/text.hpp"
#include "object/viewElement.hpp"

#pragma once

#ifndef MANAGER_HPP
#define MANAGER_HPP

enum class ObjectType {otModel, otText};

struct ManagerList
{
	std::list<std::shared_ptr<ViewElement>> viewElements;
	std::list<std::shared_ptr<ViewElement>> models;
	std::list<std::shared_ptr<ViewElement>> texts;
	std::list<std::shared_ptr<Texture>> textures;
};

class Manager
{
	public:
		std::shared_ptr<sf::RenderWindow> window;
		std::shared_ptr<sf::Font> font;
		ManagerList list;

		Manager();
		~Manager();
		bool unloadAll();

		bool update();
		bool event();
		bool eventClick(sf::Event& event);
		bool addView(ObjectType type, std::shared_ptr<ViewElement> element);
		bool removeView(ObjectType type, std::shared_ptr<ViewElement> element);
};

#endif