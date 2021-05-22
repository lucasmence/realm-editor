#include <SFML/Graphics.hpp>
#include <memory>
#include "../model/model.hpp"

#pragma once

#ifndef PALETTE_HPP
#define PALETTE_HPP

enum class PaletteType {ptTerrain};

enum class PaletteStatus {psNone, psInsert, psDelete};

class Manager;

struct PaletteItem
{
	std::shared_ptr<Model> model;
	std::string filename;
};

class Palette
{
	public:
		std::string selectedItem;
		int pageIndex;
		PaletteType type;
		Manager* manager;
		PaletteStatus status;

		std::list<std::string> terrain;
		std::list<PaletteItem> paletteItems;

		bool unloadPalettes();
		bool loadPalettes();
		bool clearPaletteItems();

		bool selectPalette();

		std::list<std::string> loadFileLists(std::string directory);
		std::string getString(std::string value);
		bool selectPaletteItem(sf::Vector2f cursor);
		bool clearPaletteItem();
		bool erasePaletteItem();

		Palette(Manager* manager);
		~Palette();

};

#endif