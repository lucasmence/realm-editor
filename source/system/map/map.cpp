#include <regex>
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
	for (auto& object : this->objects)
		this->manager->removeView(std::static_pointer_cast<ViewElement>(object.model));

	this->objects.erase(std::remove_if(this->objects.begin(),
                                       this->objects.end(),
                                       [](MapObjectUnit& objectIndex) { return objectIndex.model; }),
                                       this->objects.end());
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
	this->manager->hud->showMessage("Rendering...");

	this->file["map-size-x"] = this->data.size.x;
	this->file["map-size-y"] = this->data.size.y;
	this->file["music"] = this->data.music;
	this->file["map"]["name"] = this->data.name;
	this->file["map"]["version"] = this->data.version;

	std::vector<std::string> objectsField = { "terrain", "prop", "environment" };

	for (auto& objectField : objectsField)
		this->file[objectField].clear();

	for (auto& object : this->objects)
	{
		std::string fieldName = "";
		switch (object.type)
		{
			case (MapObjectType::motTerrain):
			{
				fieldName = "terrain";
				break;
			}
			case (MapObjectType::motProp):
			{
				fieldName = "prop";
				break;
			}
			case (MapObjectType::motEnvironment):
			{
				fieldName = "environment";
				break;
			}
		}

		if (fieldName == "")
			continue;

		bool found = false;

		std::string filename = std::regex_replace(object.model->filename, std::regex("textures/"), "");

		for (int fieldIndex = 0; fieldIndex < this->file[fieldName].size(); fieldIndex++)
			if (this->file[fieldName][fieldIndex]["texture"] == filename)
			{
				int dimensionIndex = this->file[fieldName][fieldIndex]["dimensions"].size();
				this->file[fieldName][fieldIndex]["dimensions"][dimensionIndex]["x"] = object.position.x;
				this->file[fieldName][fieldIndex]["dimensions"][dimensionIndex]["y"] = object.position.y;
				this->file[fieldName][fieldIndex]["dimensions"][dimensionIndex]["scale"] = object.model->sprite->getScale().x;
				this->file[fieldName][fieldIndex]["dimensions"][dimensionIndex]["rotation"] = object.model->sprite->getRotation();
				found = true;
				break;
			}

		if (!found)
		{
			int index = this->file[fieldName].size();
			this->file[fieldName][index]["texture"] = filename;
			this->file[fieldName][index]["dimensions"][0]["x"] = object.position.x;
			this->file[fieldName][index]["dimensions"][0]["y"] = object.position.y;
			this->file[fieldName][index]["dimensions"][0]["scale"] = object.model->sprite->getScale().x;
			this->file[fieldName][index]["dimensions"][0]["rotation"] = object.model->sprite->getRotation();
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
	this->newMap();

	this->manager->hud->showMessage("Loading map...");

	std::string dimensionField = "dimensions";

	this->filename = script::loadFile();
	this->file = Json::loadFromFile(this->filename);

	this->data.size.x = this->file.value("map-size-x", 1000);
	this->data.size.y = this->file.value("map-size-y", 1000);
	this->data.music = this->file.value("music", "village");

	if (this->file["map"].size() > 0)
	{
		this->data.name = this->file["map"].value("name", "");
		this->data.version = this->file["map"].value("version", "1.0");
	}

	std::vector<std::string> objectsField = { "terrain", "prop", "environment" };

	for (auto& field : objectsField)
	{
		MapObjectType type = MapObjectType::motTerrain;
		int priority = 8;
		
		if (field == "prop")
		{
			priority = 7;
			type = MapObjectType::motProp;
		}
		else if (field == "environment")
		{
			priority = 6;
			type = MapObjectType::motEnvironment;
		}
			
		for (int index = 0; index < this->file[field].size(); index++)
		{
			std::string texture = Json::getString(this->file[field][index].value("texture", ""));

			for (int dimensionIndex = 0; dimensionIndex < this->file[field][index][dimensionField].size(); dimensionIndex++)
			{
				sf::Vector2f position(this->file[field][index][dimensionField][dimensionIndex].value("x", 0.f),
									  this->file[field][index][dimensionField][dimensionIndex].value("y", 0.f));

				std::shared_ptr<Model> model = std::make_shared<Model>(this->manager, position, "textures/" + texture, priority, false);
				model->sprite->setScale(sf::Vector2f(this->file[field][index][dimensionField][dimensionIndex].value("scale", 1.f),
													 this->file[field][index][dimensionField][dimensionIndex].value("scale", 1.f)));
				model->sprite->setRotation(this->file[field][index][dimensionField][dimensionIndex].value("rotation", 0.f));
				this->manager->addView(std::static_pointer_cast<ViewElement>(model));
				this->addObjectUnit(MapObjectUnit{ type, position, 0.f, model });
			}
		}
	}

	this->manager->hud->showMessage("Map loaded successfully!");

	return true;
}

bool Map::newMap()
{
	this->clearObjects();
	return true;
}