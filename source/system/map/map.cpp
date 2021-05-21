#include "map.hpp"
#include "../manager.hpp"
#include "../library/script.hpp"

Map::Map(Manager* manager)
{
	this->filename = "";
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

bool Map::renderMap()
{
	this->file["terrain"].clear();

	this->file["map-size-x"] = this->data.size.x;
	this->file["map-size-y"] = this->data.size.y;
	this->file["music"] = this->data.music;
	this->file["map"]["name"] = this->data.name;
	this->file["map"]["version"] = this->data.version;

	for (auto& object : this->objects)
	{
		bool found = false;
		for (int fieldIndex = 0; fieldIndex < this->file["terrain"].size(); fieldIndex++)
			if (this->file["terrain"][fieldIndex]["texture"] == object.model->texture->filename)
			{
				int dimensionIndex = this->file["terrain"][fieldIndex]["dimensions"].size();
				this->file["terrain"][fieldIndex]["dimensions"][dimensionIndex]["x"] = object.position.x;
				this->file["terrain"][fieldIndex]["dimensions"][dimensionIndex]["y"] = object.position.y;
				found = true;
				break;
			}

		if (!found)
		{
			int terrainIndex = this->file["terrain"].size();
			this->file["terrain"][terrainIndex]["texture"] = object.model->texture->filename;
			this->file["terrain"][terrainIndex]["dimensions"][0]["x"] = object.position.x;
			this->file["terrain"][terrainIndex]["dimensions"][0]["y"] = object.position.y;
		}	
	}

	

	return true;
}

bool Map::saveMapAs()
{
	this->filename = "";
	return this->saveMap();
}

bool Map::saveMap()
{
	this->renderMap();

	if (this->filename == "")
		this->filename = script::saveFile();

	std::ofstream fileStream(filename);
	fileStream << this->file;

	this->manager->hud->showMessage("Map saved successfully!");

	return true;
}

bool Map::loadMap()
{
	return true;
}

bool Map::newMap()
{
	return true;
}