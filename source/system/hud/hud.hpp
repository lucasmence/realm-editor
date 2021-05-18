#include <SFML/Graphics.hpp>
#include <memory>
#include <list>
#include "button.hpp"

#pragma once

#ifndef HUD_HPP
#define HUD_HPP

class Manager;

class Hud 
{
	public:
		Manager* manager;

		std::list<std::shared_ptr<Button>> buttons;
		std::list<std::shared_ptr<Label>> labels;
		std::list<std::shared_ptr<Model>> models;
		std::shared_ptr<Model> shapeHover;

		bool unloadLists();
		bool loadLists();
		bool loadModels();
		bool loadButtons();
		bool loadLabels();
		bool update(sf::Vector2f cursor);
		bool updateClick(sf::Vector2f cursor);
		bool updateButtonsColor(sf::Vector2f cursor);
		bool updateLabels(sf::Vector2f cursor);
		bool updateCursor(sf::Vector2f cursor);
		bool buttonsClick(sf::Vector2f cursor);
		bool spawnClick(sf::Vector2f cursor);
		Hud(Manager* manager);
		~Hud();

};

#endif