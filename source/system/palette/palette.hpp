#include <SFML/Graphics.hpp>
#include <memory>
#include "../model/model.hpp"

#pragma once

#ifndef PALETTE_HPP
#define PALETTE_HPP

enum class PaletteType {ptTerrain, ptProp, ptEnvironment};

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
		std::list<std::string> prop;
		std::list<std::string> environment;
		std::list<PaletteItem> paletteItems;

		bool unloadPalettes();
		bool loadPalettes();
		bool clearPaletteItems();

		bool selectPalette(PaletteType type);

		std::list<std::string> loadFileLists(std::string directory);
		std::string getString(std::string value);
		bool selectPaletteItem(sf::Vector2f cursor);
		bool clearPaletteItem();
		bool erasePaletteItem();
		bool loadPaletteItemList(std::list<std::string>& list, std::string field);

		Palette(Manager* manager);
		~Palette();

};

#endif