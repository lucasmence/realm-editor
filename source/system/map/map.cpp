#include "map.hpp"
#include "../manager.hpp"

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

bool Map::removeObjectUnit(MapObjectUnit& object)
{
	this->manager->removeView(std::static_pointer_cast<ViewElement>(object.model));
	this->objects.erase(std::remove_if(this->objects.begin(),
                                       this->objects.end(),
                                       [object](MapObjectUnit& objectIndex) { return object.model == objectIndex.model; }),
                                       this->objects.end());
	return true;
}