#include <SFML/Graphics.hpp>
#include <memory>
#include "../model/model.hpp"

#pragma once

#ifndef PALETTE_HPP
#define PALETTE_HPP

enum class PaletteType {ptTerrain, ptProp, ptEnvironment, ptUnit, ptMerchant, ptPortal, ptItem};

enum class PaletteStatus {psNone, psInsert, psDelete};

enum class PaletteFormShape {fsNone, fsSquare, fsCircle};

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
		std::string selectedTexture;
		std::string selectedOrigin;
		int pageIndex;
		PaletteType type;
		Manager* manager;
		PaletteStatus status;
		PaletteFormShape formShape;

		std::list<std::string> terrain;
		std::list<std::string> prop;
		std::list<std::string> environment;
		std::list<std::string> unit;
		std::list<std::string> merchant;
		std::list<std::string> item;
		std::list<std::string> portal;
		std::list<PaletteItem> paletteItems;

		bool unloadPalettes();
		bool loadPalettes();
		bool clearPaletteItems();

		bool selectPalette(PaletteType type);

		std::list<std::string> loadFileLists(std::string directory, std::string subDirectory = "");
		std::list<std::string> loadFileFromDirectory(std::string directory, std::string base = "", std::string subDirectory = "");
		std::string getString(std::string value);
		bool checkModels(std::shared_ptr<Model> modelX, std::shared_ptr<Model> modelY);
		bool selectPaletteItem(sf::Vector2f cursor, std::shared_ptr<Model> model = nullptr);
		bool clearPaletteItem();
		bool erasePaletteItem();
		bool loadPaletteItemList(std::list<std::string>& list, std::string field);
		std::shared_ptr<Model> loadPaletteItemModel(std::string filename, sf::Vector2f position);
		bool loadPaletteShape(std::shared_ptr<Model> model, std::string filename, sf::Vector2f size = sf::Vector2f(0.f, 0.f));

		Palette(Manager* manager);
		~Palette();

};

#endif