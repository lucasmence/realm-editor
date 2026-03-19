#include <list>
#include <vector>
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

enum class PathType { ptLoadMap, ptSaveMap, ptGamepath };

struct ManagerConstant
{
	std::string fontFilePath;
	std::string gamePath;
	std::string gameVersion;
	std::vector<int> gridSize;
	std::vector<int> brushSize;
	sf::FloatRect minimapSize;
};

struct ManagerList
{
	std::list<std::shared_ptr<ViewElement>> viewElements;
	std::list<std::shared_ptr<Texture>> textures;
};

struct MapEdge
{
	sf::Vector2f positionBegin;
	sf::Vector2f positionEnd;
};

struct FileEntry 
{
	std::string name;
	std::string path;
	bool isFolder;
};

struct FilePathData 
{
	PathType type;
	std::string confirmButtonName;
	std::string dialogCaption;
	std::string path;
	std::string file;
	FileEntry currentEntry;
	bool isFolder;
	bool active;
	std::list<FileEntry> filePath;
	bool cancelButtonVisible;
	bool overwriteDialog;
};

class Manager
{
	public:
		std::shared_ptr<sf::RenderWindow> window;
		std::shared_ptr<sf::Font> font;
		std::shared_ptr<Hud> hud;
		std::shared_ptr<Palette> palette;
		std::shared_ptr<sf::View> canvas;
		std::shared_ptr<sf::View> minimapView;
		std::shared_ptr<Map> map;
		ManagerList list;
		ManagerConstant constant;

		sf::Vector2f canvasPosition;
		sf::Image icon;

		bool hasFocus;
		bool open;
		bool minimapViewUpdate;
		bool minimapVisible;
		sf::Clock deltaClock;
		std::string appName;
		sf::FloatRect minimapViewArea;
		std::vector<MapEdge> mapEdges;
		FilePathData filePathData;

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
		bool resetView();
		bool calculateMapEdges();
		std::shared_ptr<Texture> getTexture(std::string filename);
		std::string setTitle(std::string value);
		sf::Vector2f getMousePosition();
		bool loadGamepathAfter();

		bool choosePath(PathType type, std::string confirmButtonName, std::string dialogCaption, bool getFolder = false, bool cancelButtonVisible = true);
		std::list<FileEntry> returnFiles(std::string pathname);
		bool updatePathImgui();
		bool imguiUpdate();
};

#endif