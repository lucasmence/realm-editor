#include <SFML/Graphics.hpp>
#include <list>
#include "../library/json.hpp"

#pragma once

#ifndef MAP_HPP
#define MAP_HPP

enum class MapObjectType {motTerrain, motProp, motEnvironment, motUnit, motMerchant, motPortal, motItem};

class Manager;
class Model;

struct MapObjectFieldString
{
	std::string value;
	bool active;
};

struct MapObjectFieldInt
{
	int value;
	bool active;
};

struct MapObjectFieldFloat
{
	float value;
	bool active;
};

struct MapObjectFieldBool
{
	bool value;
	bool active;
};

struct MapObjectField
{
	std::string field;
	MapObjectFieldString valueString = MapObjectFieldString{"", false};
	MapObjectFieldInt valueInt = MapObjectFieldInt{0, false};
	MapObjectFieldFloat valueFloat = MapObjectFieldFloat{0.f, false};
	MapObjectFieldBool valueBool = MapObjectFieldBool{false, false};
};

struct MapObject
{
	MapObjectType type;
	sf::Vector2f position;
	float angle;
};

struct MapObjectUnit : public MapObject
{
	std::shared_ptr<Model> model;
	std::list<MapObjectField> fields;
	MapObjectUnit(MapObjectType type, sf::Vector2f position, float angle, std::shared_ptr<Model> model, std::list<MapObjectField> fields) :
		MapObject{ type, position, angle}, model(model), fields(fields) {};
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
	MapObjectUnit textureBackground = MapObjectUnit{ MapObjectType::motTerrain, sf::Vector2f(-200.f, -200.f), 0.f, nullptr, {} };
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
		std::string getTextureFromUnit(json line, MapObjectType type);
		std::string getOriginFromField(json line, MapObjectType type);
		std::list<MapObjectField> getSubfieldsFromLine(json line);
		bool updateMapInfo();
		int getObjectPriority(MapObjectType type);

		bool renderMap();
		bool renderObject(json& localfile, MapObjectUnit& object);
		bool renderObjectField(json& localfile, MapObjectField& field);
		bool saveMap();
		bool saveMapAs();
		bool loadMap(std::string file = "");
		bool newMap();
		bool reloadMap();
		bool createTriggerFile();

};

#endif