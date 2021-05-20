#include <SFML/Graphics.hpp>
#include <memory>
#include <list>
#include "button.hpp"

#pragma once

#ifndef HUD_HPP
#define HUD_HPP

class Manager;

struct MessageBox
{
	std::shared_ptr<Label> label;
	std::shared_ptr<Model> border;
};

class Hud 
{
	public:
		Manager* manager;

		std::list<std::shared_ptr<Button>> buttons;
		std::list<std::shared_ptr<Label>> labels;
		std::list<std::shared_ptr<Model>> models;
		std::list<std::shared_ptr<Model>> grid;
		std::shared_ptr<Model> shapeHover;
		MessageBox messageBox;

		bool unloadLists();
		bool loadLists();
		bool loadModels();
		bool loadGrid();
		bool loadButtons();
		bool loadLabels();
		bool update(sf::Vector2f cursor);
		bool updateClick(sf::Vector2f cursor);
		bool updateButtonsColor(sf::Vector2f cursor);
		bool updateLabels(sf::Vector2f cursor);
		bool updateLabelPaletteStatus(std::shared_ptr<sf::Text> text);
		bool updateCursor(sf::Vector2f cursor);
		bool buttonsClick(sf::Vector2f cursor);
		bool spawnClick(sf::Vector2f cursor);
		bool showMessage(std::string text, float time = 3.f);
		Hud(Manager* manager);
		~Hud();

};

#endif