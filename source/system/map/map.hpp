#include <SFML/Graphics.hpp>
#include <list>
#include <memory>
#include "../library/json.hpp"

#pragma once

#ifndef MAP_HPP
#define MAP_HPP

enum class MapObjectType {motTerrain};

class Manager;
class Model;

struct MapObject
{
	MapObjectType type;
	sf::Vector2f position;
	float angle;
};

struct MapObjectUnit : public MapObject
{
	std::shared_ptr<Model> model;
	MapObjectUnit(MapObjectType type, sf::Vector2f position, float angle, std::shared_ptr<Model> model) :
		MapObject{ type, position, angle}, model(model) {};
};

struct MapObjectMatrix : public MapObject
{
	sf::Vector2f positionEnd;
	std::list<std::shared_ptr<Model>> models;
};

struct MapData
{
	std::string name = "", version = "1.00", music = "village";
	sf::Vector2i size = sf::Vector2i(1000, 1000);
};

class Map
{
	public:
		Manager* manager;
		std::list<MapObjectUnit> objects;
		MapData data;
		json file;
		std::string filename;

		Map(Manager* manager);
		~Map();

		bool addObjectUnit(MapObjectUnit object);
		bool removeObjectUnit(MapObjectUnit& object);
		bool clearObjects();

		bool renderMap();
		bool saveMap();
		bool saveMapAs();
		bool loadMap();
		bool newMap();

};

#endif