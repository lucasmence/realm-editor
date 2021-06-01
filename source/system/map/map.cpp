#include <regex>
#include <typeinfo>
#include <boost/lexical_cast.hpp>
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

bool Map::updateMapInfo()
{
	this->manager->hud->setEditValue("edtMapSizeX", boost::lexical_cast<std::string>(this->data.size.x));
	this->manager->hud->setEditValue("edtMapSizeY", boost::lexical_cast<std::string>(this->data.size.y));
	this->manager->hud->setEditValue("edtMapName", boost::lexical_cast<std::string>(this->data.name));
	this->manager->hud->setEditValue("edtMapMusic", boost::lexical_cast<std::string>(this->data.music));
	this->manager->hud->setEditValue("edtMapVersion", boost::lexical_cast<std::string>(this->data.version));

	return true;
}

int Map::getObjectPriority(MapObjectType type)
{
	switch (type)
	{
		case (MapObjectType::motTerrain):
		{
			return 8;
			break;
		}
		case (MapObjectType::motProp):
		{
			return 7;
			break;
		}
		case (MapObjectType::motEnvironment):
		{
			return 6;
			break;
		}
		case (MapObjectType::motUnit):
		{
			return 5;
			break;
		}
		case (MapObjectType::motMerchant):
		{
			return 5;
			break;
		}
		case (MapObjectType::motPortal):
		{
			return 4;
			break;
		}
	}

	return 0;
}

std::string Map::getOriginFromField(json line, MapObjectType type)
{
	switch (type)
	{
		case (MapObjectType::motUnit):
		{
			return line.value("unit", "");
			break;
		}

		case (MapObjectType::motMerchant):
		{
			return line.value("merchant", "");
			break;
		}

		case (MapObjectType::motPortal):
		{
			return line.value("type", "");
			break;
		}

		default:
		{
			return line.value("texture", "");
		}
	}

	return "";
}

std::string Map::getTextureFromUnit(json line, MapObjectType type)
{
	switch (type)
	{
		case (MapObjectType::motUnit):
		{
			std::string subFilename = line.value("unit", "");
			if (subFilename == "")
				return "";

			json subFile = Json::loadFromFile("data/" + subFilename +".json");

			std::string texture = Json::getString(subFile.value("texture", ""));
			subFile.clear();

			return texture;

			break;
		}

		case (MapObjectType::motMerchant):
		{
			std::string subFilename = line.value("merchant", "");
			if (subFilename == "")
				return "";

			json subFile = Json::loadFromFile("data/" + subFilename + ".json");

			std::string texture = Json::getString(subFile["models"][0].value("value", ""));
			subFile.clear();

			return texture;
			break;
		}
	}

	return "";
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

bool Map::renderObjectField(json& localfile, MapObjectField& field)
{
	if (field.valueString.active)
		localfile[field.field] = field.valueString.value;
	else if (field.valueInt.active)
		localfile[field.field] = field.valueInt.value;
	else if (field.valueFloat.active)
		localfile[field.field] = field.valueFloat.value;
	else if (field.valueBool.active)
		localfile[field.field] = field.valueBool.value;
	return true;
}

bool Map::renderObject(json& localfile, MapObjectUnit& object)
{
	localfile["x"] = object.position.x;
	localfile["y"] = object.position.y;
	localfile["scale"] = object.model->getScale().x;
	localfile["rotation"] = object.model->getRotation();

	switch (object.type)
	{
		case (MapObjectType::motTerrain): case (MapObjectType::motEnvironment): case (MapObjectType::motProp):
		{
			int priority = object.model->priority - this->getObjectPriority(object.type);
			if (priority > 0)
				localfile["priority"] = object.model->priority;
			break;
		}
	}

	for (auto& field : object.fields)
		this->renderObjectField(localfile, field);

	return true;
}

bool Map::renderMap()
{
	this->manager->hud->showMessage("Rendering map...");

	this->file["map-size-x"] = this->data.size.x;
	this->file["map-size-y"] = this->data.size.y;
	this->file["music"] = this->data.music;
	this->file["map"]["name"] = this->data.name;
	this->file["map"]["version"] = this->data.version;

	std::vector<std::string> objectsField = { "terrain", "prop", "environment", "unit", "merchant", "portal" };

	for (auto& objectField : objectsField)
		this->file[objectField].clear();

	for (auto& object : this->objects)
	{
		std::string fieldName = "", fieldCaption = "texture";
		std::string filename = std::regex_replace(object.model->filename, std::regex("textures/"), "");

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
			case (MapObjectType::motUnit):
			{
				fieldName = "unit";
				fieldCaption = "unit";
				filename = object.model->origin;
				break;
			}
			case (MapObjectType::motMerchant):
			{
				fieldName = "merchant";
				fieldCaption = "merchant";
				filename = object.model->origin;
				break;
			}
			case (MapObjectType::motPortal):
			{
				fieldName = "portal";
				fieldCaption = "type";
				filename = object.model->origin;
				break;
			}
		}

		if (fieldName == "")
			continue;

		bool found = false;

		for (int fieldIndex = 0; fieldIndex < this->file[fieldName].size(); fieldIndex++)
			if (this->file[fieldName][fieldIndex][fieldCaption] == filename)
			{
				int dimensionIndex = this->file[fieldName][fieldIndex]["dimensions"].size();
				this->renderObject(this->file[fieldName][fieldIndex]["dimensions"][dimensionIndex], object);
				found = true;
				break;
			}

		if (!found)
		{
			int index = this->file[fieldName].size();
			this->file[fieldName][index][fieldCaption] = filename;
			this->renderObject(this->file[fieldName][index]["dimensions"][0], object);
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

	this->manager->hud->showMessage("Ready to save...");
	if (this->filename == "")
		this->filename = script::saveFile();

	std::ofstream fileStream(filename);
	fileStream << this->file;

	this->manager->setTitle(this->filename);

	this->manager->hud->showMessage("Map saved successfully!");

	return true;
}

bool Map::loadMap()
{
	this->newMap();

	std::string dimensionField = "dimensions";

	this->filename = script::loadFile();
	this->file = Json::loadFromFile(this->filename);

	this->manager->hud->showMessage("Loading map...");

	this->data.size.x = this->file.value("map-size-x", 1000);
	this->data.size.y = this->file.value("map-size-y", 1000);
	this->data.music = this->file.value("music", "village");

	if (this->file["map"].size() > 0)
	{
		this->data.name = this->file["map"].value("name", "");
		this->data.version = this->file["map"].value("version", "1.0");
	}

	this->updateMapInfo();

	std::vector<std::string> objectsField = { "terrain", "prop", "environment", "unit", "merchant", "portal" };

	for (auto& field : objectsField)
	{
		MapObjectType type = MapObjectType::motTerrain;
		
		if (field == "prop")
			type = MapObjectType::motProp;
		else if (field == "environment")
			type = MapObjectType::motEnvironment;
		else if (field == "unit")
			type = MapObjectType::motUnit;
		else if (field == "merchant")
			type = MapObjectType::motMerchant;
		else if (field == "portal")
			type = MapObjectType::motPortal;

		int priority = this->getObjectPriority(type);
			
		for (int index = 0; index < this->file[field].size(); index++)
		{
			std::string texture = "";

			if (type == MapObjectType::motUnit || type == MapObjectType::motMerchant)
				texture = this->getTextureFromUnit(this->file[field][index], type);
			else if (type == MapObjectType::motPortal)
				texture = Json::getString(this->file[field][index].value("type", ""));
			else
				texture = Json::getString(this->file[field][index].value("texture", ""));

			if (texture == "")
				continue;

			for (int dimensionIndex = 0; dimensionIndex < this->file[field][index][dimensionField].size(); dimensionIndex++)
			{
				int priorityIndex = priority + this->file[field][index][dimensionField][dimensionIndex].value("priority", 0);

				sf::Vector2f position(this->file[field][index][dimensionField][dimensionIndex].value("x", 0.f),
									  this->file[field][index][dimensionField][dimensionIndex].value("y", 0.f));

				std::shared_ptr<Model> model = nullptr;
				switch (type)
				{
					case (MapObjectType::motPortal):
					{
						model = std::make_shared<Model>(this->manager, position, "", priorityIndex, false, "", this->getOriginFromField(this->file[field][index], type));
						this->manager->palette->loadPaletteShape(model, texture);
						break;
					}

					default:
					{
						model = std::make_shared<Model>(this->manager, position, "textures/" + texture, priorityIndex, false, "",
														this->getOriginFromField(this->file[field][index], type));
						model->sprite->setScale(sf::Vector2f(this->file[field][index][dimensionField][dimensionIndex].value("scale", 1.f),
															 this->file[field][index][dimensionField][dimensionIndex].value("scale", 1.f)));
						model->sprite->setRotation(this->file[field][index][dimensionField][dimensionIndex].value("rotation", 0.f));
					}
				}
				this->manager->addView(std::static_pointer_cast<ViewElement>(model));

				std::vector<std::string> subFieldsExceptions = {"texture", "unit", "merchant", "x", "y", "scale", "rotation", "priority"};
				std::list<MapObjectField> subFields = {};
				for (auto& subFieldIndex : this->file[field][index][dimensionField][dimensionIndex].items())
				{
					bool found = false;
					for (auto& exception : subFieldsExceptions)
						if (exception == subFieldIndex.key())
						{
							found = true;
							break;
						}

					if (found)
						continue;

					MapObjectField subFieldObject;
					subFieldObject.field = subFieldIndex.key();
					if (subFieldIndex.value().type_name() == "string")
						subFieldObject.valueString = MapObjectFieldString{ subFieldIndex.value(), true };				
					else if (subFieldIndex.value().type_name() == "number")
						subFieldObject.valueFloat = MapObjectFieldFloat{ subFieldIndex.value(), true };
					else if (subFieldIndex.value().type_name() == "boolean")
						subFieldObject.valueBool = MapObjectFieldBool{ subFieldIndex.value(), true };						

					subFields.emplace_back(subFieldObject);
				}

				this->addObjectUnit(MapObjectUnit{ type, position, 0.f, model, subFields });
			}
		}
	}

	this->manager->setTitle(this->filename);

	this->manager->hud->showMessage("Map loaded successfully!");

	return true;
}

bool Map::newMap()
{
	this->clearObjects();
	this->manager->palette->clearPaletteItem();

	this->filename = "";
	this->data.size = sf::Vector2i(1000, 1000);
	this->data.name = "another_map";
	this->data.music = "village";
	this->data.version = "1.00";

	this->updateMapInfo();

	this->manager->setTitle("New");

	return true;
}