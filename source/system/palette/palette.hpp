#include <SFML/Graphics.hpp>
#include <memory>
#include "../model/model.hpp"

#pragma once

#ifndef PALETTE_HPP
#define PALETTE_HPP

enum class PaletteType {ptTerrain};

class Manager;

struct PaletteItem
{
	std::shared_ptr<Model> model;
	std::string filename;
};

class Palette
{
	public:
		int pageIndex;
		PaletteType type;
		Manager* manager;

		std::list<std::string> terrain;
		std::list<PaletteItem> paletteItems;

		bool unloadPalettes();
		bool loadPalettes();
		bool clearPaletteItems();

		bool selectPalette();

		std::list<std::string> loadFileLists(std::string directory);
		std::string getString(std::string value);

		Palette(Manager* manager);
		~Palette();

};

#endif