#include <SFML/Graphics.hpp>
#include <memory>
#include <list>
#include <vector>
#include "button.hpp"
#include "edit.hpp"

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

		sf::Vector2f hoverShapeSize;
		int gridSize;
		int brushSize;
		int rotation;
		bool mousePressed;
		bool spawnPress;
		bool centerShape;
		std::vector<int> gridSizeList;
		std::vector<int> brushSizeList;
		std::list<std::shared_ptr<Button>> buttons;
		std::list<std::shared_ptr<Edit>> edits;
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
		bool updateEditsColor(sf::Vector2f cursor);
		bool updateMouseReleased();
		bool updateMousePressed(sf::Vector2f cursor);
		bool updateEdit(char text);
		bool updateEditValues();
		bool updateLabels(sf::Vector2f cursor);
		bool updateLabelPaletteStatus(std::shared_ptr<sf::Text> text);
		bool updateCursor(sf::Vector2f cursor);
		bool buttonsClick(sf::Vector2f cursor);
		bool editsClick(sf::Vector2f cursor);
		bool spawnClick(sf::Vector2f cursor);
		bool showMessage(std::string text, float time = 3.f);
		bool changeGridSize(int order);
		bool changeBrushSize(int order);
		bool updateHoverShapeSize();
		bool toggleGridVisibility();
		bool toggleSpawnPress(std::shared_ptr<Button> button);
		bool toggleCenterShape(std::shared_ptr<Button> button);
		bool help();
		Hud(Manager* manager);
		~Hud();

};

#endif