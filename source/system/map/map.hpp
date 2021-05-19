#include <SFML/Graphics.hpp>
#include <list>
#include <memory>

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
	float scale;
	float angle;
};

struct MapObjectUnit : public MapObject
{
	std::shared_ptr<Model> model;
	MapObjectUnit(MapObjectType type, sf::Vector2f position, float scale, float angle, std::shared_ptr<Model> model) :
		MapObject{ type, position, scale, angle }, model(model) {};
};

struct MapObjectMatrix : public MapObject
{
	sf::Vector2f positionEnd;
	std::list<std::shared_ptr<Model>> models;
};

class Map
{
	public:
		Manager* manager;
		std::list<MapObjectUnit> objects;

		Map(Manager* manager);
		~Map();

		bool addObjectUnit(MapObjectUnit object);
		bool removeObjectUnit(MapObjectUnit& object);
		bool clearObjects();

};

#endif