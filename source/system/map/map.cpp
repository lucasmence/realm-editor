#include "map.hpp"

Map::Map(Manager* manager)
{
	this->manager = manager;
	this->clearObjects();
}

Map::~Map()
{
	this->clearObjects();
}

bool Map::clearObjects()
{
	this->objects.clear();
	return true;
}

bool Map::addObjectUnit(MapObjectUnit object)
{
	this->objects.emplace_back(object);
	return true;
}