#include <cstring>
#include <boost/lexical_cast.hpp>
#include <boost/algorithm/string.hpp>
#include <math.h>
#include "hud.hpp"
#include "../manager.hpp"
#include "../library/position.hpp"
#include "../external/imgui/imgui.h"
#include "../external/imgui/imgui-SFML.h"

Hud::Hud(Manager* manager)
{
	this->manager = manager;
	this->gridSizeList = this->manager->constant.gridSize;
	this->brushSizeList = this->manager->constant.brushSize;
	this->gridSize = 2;
	this->brushSize = 0;
	this->rotation = 0;
	this->priority = 0;
	this->formShapeSize = 0;
	this->scale = 1.f;
	this->zoom = 1.f;
	this->gridSpawn = true;
	this->gridVisible = true;
	this->spawnPress = false;
	this->mousePressed = false;
	this->centerShape = false;
	this->mouseRightButton = false;
	this->matrixActivated = false;
	this->matrixTriggered = false;
	this->matrixPosSpawn = false;
	this->wallActivated = false;
	this->itemSelect = false;
	this->itemSelected = false;
	this->itemSelectedMove = false;
	this->dragCursor = false;
	this->removeBgVisible = false;
	this->gettingExtraValues = false;
	this->formShapeSelected = "none";
	this->hoverShapeSize = sf::Vector2f(0.f, 0.f);
	this->mousePressPosition = sf::Vector2f(0.f, 0.f);
	this->mapTempTick = GameTick{ 0, 50000 };
	this->weatherChance = 100;
	this->weatherName = "";
	this->particles = "none";
	this->mapName = "map";
	this->mapMusic = "none";
	this->mapVersion = "1.00";
	this->bgTexture = "";
	this->notificationText = "";
	this->notificationTimer = 0.f;
	this->notificationDuration = 3.f;
	this->historySpawnBuffer.clear();
	this->undoStack.clear();
	this->redoStack.clear();
	this->matrixSpawnInProgress = false;
	this->showPreferencesWindow = false;

	memset(imguiMapName, 0, sizeof(imguiMapName));
	memset(imguiMapMusic, 0, sizeof(imguiMapMusic));
	memset(imguiMapVersion, 0, sizeof(imguiMapVersion));
	memset(imguiWeatherName, 0, sizeof(imguiWeatherName));
	memset(imguiParticles, 0, sizeof(imguiParticles));
	for (int i = 0; i < 7; i++)
		memset(imguiExtraFields[i], 0, sizeof(imguiExtraFields[i]));
	strncpy(imguiMapName, "map", sizeof(imguiMapName) - 1);
	strncpy(imguiMapMusic, "none", sizeof(imguiMapMusic) - 1);
	strncpy(imguiMapVersion, "1.00", sizeof(imguiMapVersion) - 1);
	strncpy(imguiWeatherName, "", sizeof(imguiWeatherName) - 1);
	strncpy(imguiParticles, "none", sizeof(imguiParticles) - 1);

	this->shapeHover = std::make_shared<Model>(this->manager, sf::Vector2f(0.f, 300.f), "", 1, false);
	this->shapeHover->loadShape(sf::Vector2f(1.f, 1.f), sf::Color(150, 200, 150, 100));
	this->shapeHover->visible = false;
	this->manager->addView(std::static_pointer_cast<ViewElement>(this->shapeHover));

	this->shapeMapArea = std::make_shared<Model>(this->manager, sf::Vector2f(0.f, 0.f), "", 5, false);
	this->shapeMapArea->loadShape(sf::Vector2f(1.f, 1.f), sf::Color(225, 100, 175, 100));
	this->manager->addView(std::static_pointer_cast<ViewElement>(this->shapeMapArea));
	this->shapeMapArea->visible = false;

	this->shapeMatrix = std::make_shared<Model>(this->manager, sf::Vector2f(0.f, 0.f), "", 1, false);
	this->shapeMatrix->loadShape(sf::Vector2f(1.f, 1.f), sf::Color(150, 255, 0, 150));
	this->manager->addView(std::static_pointer_cast<ViewElement>(this->shapeMatrix));
	this->shapeMatrix->visible = false;

	this->shapeItemSelected = std::make_shared<Model>(this->manager, sf::Vector2f(0.f, 0.f), "", 0, false);
	this->shapeItemSelected->loadShape(sf::Vector2f(1.f, 1.f), sf::Color(255, 255, 255, 100));
	this->manager->addView(std::static_pointer_cast<ViewElement>(this->shapeItemSelected));
	this->shapeItemSelected->visible = false;

	this->shapeMinimap = nullptr;
	this->itemModelSelected = nullptr;

	this->loadLists();
}

Hud::~Hud()
{
	this->manager->removeView(std::static_pointer_cast<ViewElement>(this->shapeHover));
	this->manager->removeView(std::static_pointer_cast<ViewElement>(this->shapeMapArea));
	this->manager->removeView(std::static_pointer_cast<ViewElement>(this->shapeMatrix));
	this->manager->removeView(std::static_pointer_cast<ViewElement>(this->shapeItemSelected));
	this->manager->removeView(std::static_pointer_cast<ViewElement>(this->shapeMinimap));
	this->itemModelSelected = nullptr;
	this->unloadLists();
}

bool Hud::update(sf::Vector2f cursor)
{
	this->updateCursor(cursor);
	this->updateHoverShapeSize();
	this->updateHoverMapSize();
	this->updateMousePressed(cursor);
	this->updateHoverGeneral();
	this->updateMapTemp();
	return true;
}

bool Hud::updateMapTemp()
{
	this->mapTempTick.tickValue += 1;
	if (this->mapTempTick.tickMax <= this->mapTempTick.tickValue)
	{
		this->manager->map->saveMapTemp();
		this->mapTempTick.tickValue = 0;
		return true;
	}
	return false;
}

bool Hud::updateClick(sf::Vector2f cursor, bool rightButton)
{
	this->mousePressPosition = cursor;
	this->mousePressed = true;
	this->mouseRightButton = rightButton;
	this->selectedItemUpdate();
	this->updateItemSelectedMove(cursor);
	this->matrixActivate(cursor);
	this->spawnClick(cursor);
	this->selectItem(cursor);
	return true;
}

bool Hud::updateItemSelectedMove(sf::Vector2f cursor)
{
	if (!this->itemSelected || !this->itemSelectedMove || !this->checkMapClick(cursor))
		return false;

	for (auto& object : this->manager->map->objects)
		if (object.model == this->itemModelSelected)
		{
			object.model->setPosition(sf::Vector2f(cursor.x - object.model->getGlobalBounds().width / 2.f, cursor.y - object.model->getGlobalBounds().height / 2.f));
			this->shapeItemSelected->setPosition(object.model->getPosition());
			return true;
		}
	return false;
}

bool Hud::updateMouseReleased(sf::Vector2f cursor)
{
	this->mousePressed = false;
	this->matrixDeactivate(cursor);
	return true;
}

bool Hud::updateMousePressed(sf::Vector2f cursor)
{
	if (!this->mousePressed)
		return false;
	if (this->spawnPress)
		return this->spawnClick(cursor);
	else if (this->matrixTriggered)
		return this->updateShapeMatrix(cursor);
	else
		this->updateDragCursor(cursor);
	return false;
}

bool Hud::updateCursor(sf::Vector2f cursor)
{
	if (this->manager->palette->selectedItem == "")
		return false;

	for (auto& model : this->models)
		if (model->shape)
			if (model->shape->getGlobalBounds().contains(cursor))
			{
				this->shapeHover->visible = false;
				return false;
			}
	
	this->shapeHover->visible = true;

	sf::Vector2f shapePosition(0.f, 0.f);
	if (this->centerShape)
		shapePosition = sf::Vector2f(this->shapeHover->shape->getGlobalBounds().width / 2.f, this->shapeHover->shape->getGlobalBounds().height / 2.f);

	if (this->gridSpawn)
		shapePosition = position::getGridPosition(sf::Vector2f(this->gridSizeList.at(this->gridSize) / 2.f + (shapePosition.x),
			this->gridSizeList.at(this->gridSize) / 2.f + (shapePosition.y)), cursor);
	else 
		shapePosition = sf::Vector2f(cursor.x - shapePosition.x, cursor.y - shapePosition.y);
	
	this->shapeHover->shape->setPosition(shapePosition);
	return true;
}

bool Hud::updateDragCursor(sf::Vector2f cursor)
{
	if (!this->dragCursor || !this->checkMapClick(cursor))
		return false;

	this->manager->setCanvasCenter(sf::Vector2f(this->mousePressPosition.x - cursor.x, this->mousePressPosition.y - cursor.y));
	this->mousePressPosition = this->manager->getMousePosition();
	return true;
}

bool Hud::zoomMap(int value)
{
	this->zoom += 0.05f * value;
	this->manager->canvas->zoom(this->zoom);
	return true;
}

bool Hud::zoomMapReset()
{
	this->zoom = 1.f;
	this->manager->canvas->zoom(this->zoom);
	return true;
}

bool Hud::setEditValue(std::string editName, std::string value)
{
	if (editName == "edtRotation")
		this->rotation = atoi(value.c_str());
	else if (editName == "edtFormShapeSize")
		this->formShapeSize = atoi(value.c_str());
	else if (editName == "edtScale")
		this->scale = atoi(value.c_str()) / 100.f;
	else if (editName == "edtPriority")
		this->priority = atoi(value.c_str());
	else if (editName == "edtMapSizeX")
		this->manager->map->data.size.x = atoi(value.c_str());
	else if (editName == "edtMapSizeY")
		this->manager->map->data.size.y = atoi(value.c_str());
	else if (editName == "edtMapName") {
		this->mapName = value;
		this->manager->map->data.name = value;
		strncpy(imguiMapName, value.c_str(), sizeof(imguiMapName) - 1);
	}
	else if (editName == "edtMapMusic") {
		this->mapMusic = value;
		this->manager->map->data.music = value;
		strncpy(imguiMapMusic, value.c_str(), sizeof(imguiMapMusic) - 1);
	}
	else if (editName == "edtMapVersion") {
		this->mapVersion = value;
		this->manager->map->data.version = value;
		strncpy(imguiMapVersion, value.c_str(), sizeof(imguiMapVersion) - 1);
	}
	else if (editName == "edtWeatherChance") {
		this->weatherChance = atoi(value.c_str());
		this->manager->map->data.weatherChance = this->weatherChance;
	}
	else if (editName == "edtWeatherName") {
		this->weatherName = value;
		this->manager->map->data.weatherName = value;
		strncpy(imguiWeatherName, value.c_str(), sizeof(imguiWeatherName) - 1);
	}
	else if (editName == "edtParticles") {
		this->particles = value;
		this->manager->map->data.particles = value;
		strncpy(imguiParticles, value.c_str(), sizeof(imguiParticles) - 1);
	}
	else {
		for (int index = 0; index < 7; index++) {
			std::string fieldName = "edtExtraField-" + boost::lexical_cast<std::string>(index);
			if (editName == fieldName) {
				strncpy(imguiExtraFields[index], value.c_str(), sizeof(imguiExtraFields[index]) - 1);
				return true;
			}
		}
		return false;
	}
	return true;
}

bool Hud::help()
{
	this->showMessage("Coming soon...");
	return true;
}

bool Hud::changeGridSize(int order)
{
	this->gridSize += order;
	if (this->gridSize < 0)
		this->gridSize = 0;
	else if (this->gridSize >= (int)this->gridSizeList.size())
		this->gridSize = (int)this->gridSizeList.size() - 1;
	return this->loadGrid();
}

bool Hud::changeBrushSize(int order)
{
	this->brushSize += order;
	if (this->brushSize < 0)
		this->brushSize = 0;
	else if (this->brushSize >= (int)this->brushSizeList.size())
		this->brushSize = (int)this->brushSizeList.size() - 1;
	this->updateHoverShapeSize();
	return true;
}

bool Hud::updateShapeMatrix(sf::Vector2f cursor)
{
	if (!this->shapeMatrix->visible || !this->matrixTriggered)
		return false;
	sf::Vector2f position(cursor.x - this->shapeMatrix->shape->getPosition().x, cursor.y - this->shapeMatrix->shape->getPosition().y);
	std::static_pointer_cast<sf::RectangleShape>(this->shapeMatrix->shape)->setSize(position);
	return true;
}

bool Hud::updateHoverMapSize()
{
	if (!this->shapeMapArea->visible)
		return false;
	std::static_pointer_cast<sf::RectangleShape>(this->shapeMapArea->shape)->setSize(sf::Vector2f(this->manager->map->data.size.x, this->manager->map->data.size.y));
	return true;
}

bool Hud::updateHoverGeneral()
{
	this->shapeItemSelected->visible = this->itemSelected;
	if (this->shapeMinimap && this->shapeMinimap->shape)
		std::static_pointer_cast<sf::RectangleShape>(this->shapeMinimap->shape)->setSize(sf::Vector2f(this->manager->minimapViewArea.width / 2.f, this->manager->minimapViewArea.height / 2.f));
	return true;
}

bool Hud::updateHoverShapeSize()
{
	std::static_pointer_cast<sf::RectangleShape>(this->shapeHover->shape)->setSize(sf::Vector2f(this->hoverShapeSize.x * (sqrt((float)this->brushSizeList.at(this->brushSize))),
		this->hoverShapeSize.y * (sqrt((float)this->brushSizeList.at(this->brushSize)))));
	this->shapeHover->shape->setOrigin(sf::Vector2f(0.f, 0.f));
	this->shapeHover->shape->setRotation(this->rotation);
	this->shapeHover->shape->setScale(this->scale, this->scale);
	return true;
}

bool Hud::toggleGridVisibility()
{
	this->gridVisible = !this->gridVisible;
	for (auto& gridIndex : this->grid)
		gridIndex->visible = this->gridVisible;
	return true;
}

bool Hud::updateMapBounds()
{
	sf::Vector2f positionLowerest(-1.f, -1.f);
	sf::Vector2f positionExtra(0.f, 0.f);
	for (auto& object : this->manager->map->objects)
		if (object.type == MapObjectType::motTerrain || object.type == MapObjectType::motPortal)
		{
			if (positionLowerest.x > object.model->getPosition().x || positionLowerest.x == -1.f)
			{
				positionLowerest.x = object.model->getPosition().x;
				positionExtra.x = (object.type == MapObjectType::motPortal) ? -object.model->getGlobalBounds().width : 0.f;
			}
			if (positionLowerest.y > object.model->getPosition().y || positionLowerest.y == -1.f)
			{
				positionLowerest.y = object.model->getPosition().y;
				positionExtra.y = (object.type == MapObjectType::motPortal) ? -object.model->getGlobalBounds().height : 0.f;
			}
		}

	for (auto& object : this->manager->map->objects)
		object.model->setPosition(sf::Vector2f(object.model->getPosition().x - positionLowerest.x + positionExtra.x, object.model->getPosition().y - positionLowerest.y + positionExtra.y));

	sf::Vector2f positionHighest(-1.f, -1.f);
	positionExtra = sf::Vector2f(0.f, 0.f);
	for (auto& object : this->manager->map->objects)
		if (object.type == MapObjectType::motTerrain || object.type == MapObjectType::motPortal)
		{
			if (positionHighest.x < object.model->getPosition().x)
			{
				positionHighest.x = object.model->getPosition().x;
				positionExtra.x = (object.type == MapObjectType::motTerrain) ? object.model->getGlobalBounds().width : 0.f;
			}
			if (positionHighest.y < object.model->getPosition().y)
			{
				positionHighest.y = object.model->getPosition().y;
				positionExtra.y = (object.type == MapObjectType::motTerrain) ? object.model->getGlobalBounds().height : 0.f;
			}
		}

	this->setEditValue("edtMapSizeX", boost::lexical_cast<std::string>((int)(positionHighest.x + positionExtra.x)));
	this->setEditValue("edtMapSizeY", boost::lexical_cast<std::string>((int)(positionHighest.y + positionExtra.y)));
	return true;
}

bool Hud::formShapeClick(const std::string& shapeName)
{
	if (shapeName == "square") {
		this->formShapeSelected = "square";
		this->manager->palette->formShape = PaletteFormShape::fsSquare;
	} else if (shapeName == "circle") {
		this->formShapeSelected = "circle";
		this->manager->palette->formShape = PaletteFormShape::fsCircle;
	} else {
		this->formShapeSelected = "none";
		this->manager->palette->formShape = PaletteFormShape::fsNone;
	}
	return true;
}

bool Hud::removeBackground()
{
	this->manager->map->data.textureBackground.model = nullptr;
	this->removeBgVisible = false;
	this->bgTexture = "";
	this->manager->hud->showMessage("Terrain background removed!");
	return true;
}

bool Hud::checkMapClick(sf::Vector2f cursor)
{
	for (auto& model : this->models)
		if (model->getGlobalBounds().contains(cursor))
			return false;
	return true;
}

bool Hud::selectedItemUpdate()
{
	if (!this->itemSelected)
		return false;
	std::list<MapObjectField> fields = this->getExtraEditValuesByType();
	for (auto& object : this->manager->map->objects)
		if (object.model == this->itemModelSelected) {
			object.fields = fields;
			return true;
		}
	return false;
}

bool Hud::deleteSelectedItem()
{
	if (!this->itemModelSelected)
		return false;
	MapObjectUnit objectSelected{ MapObjectType::motTerrain , sf::Vector2f(0.f, 0.f), 0.f, nullptr, {} };
	for (auto& object : this->manager->map->objects)
		if (object.model == this->itemModelSelected) {
			objectSelected = object;
			break;
		}
	if (!objectSelected.model)
		return false;
	this->manager->palette->clearPaletteItem();
	this->historySpawnBuffer.emplace_back(objectSelected);
	std::vector<MapObjectUnit> objectList = this->updateBitmaskRemove(objectSelected);
	this->manager->map->removeObjectUnit(objectSelected);
	this->updateBitmaskList(objectList);
	objectList.clear();
	this->recordHistory(HistoryActionType::hatDelete, "Deleted selected item");
	return true;
}

bool Hud::selectItem(sf::Vector2f cursor)
{
	if (!this->itemSelect || !this->checkMapClick(cursor))
		return false;

	MapObjectUnit objectSelected{ MapObjectType::motTerrain , sf::Vector2f(0.f, 0.f), 0.f, nullptr, {} };
	for (auto& object : this->manager->map->objects)
		if (object.model->getGlobalBounds().contains(cursor)) {
			if (objectSelected.model != nullptr && objectSelected.model->priority < object.model->priority)
				continue;
			objectSelected = object;
		}

	if (!objectSelected.model)
		return false;

	PaletteType paletteType = PaletteType::ptTerrain;
	this->getPaletteType(paletteType, objectSelected.type);
	this->manager->palette->selectPalette(paletteType);

	if (paletteType == PaletteType::ptPortal)
		this->manager->palette->selectPaletteItem(sf::Vector2f(0.f, 0.f), objectSelected.model);

	this->itemSelected = true;
	std::static_pointer_cast<sf::RectangleShape>(this->shapeItemSelected->shape)->setSize(sf::Vector2f(objectSelected.model->getGlobalBounds().width, objectSelected.model->getGlobalBounds().height));
	this->shapeItemSelected->setPosition(objectSelected.model->getPosition());
	this->itemModelSelected = objectSelected.model;

	for (auto& field : objectSelected.fields)
	{
		std::string value = "";
		if (field.valueString.active)
			value = field.valueString.value;
		else if (field.valueInt.active)
			value = boost::lexical_cast<std::string>(field.valueInt.value);
		else if (field.valueFloat.active)
			value = boost::lexical_cast<std::string>(field.valueFloat.value);
		else if (field.valueBool.active)
			value = field.valueBool.value ? "true" : "false";

		for (int index = 0; index < 7; index++)
			if ((int)this->extraFieldOrigins.size() > index && this->extraFieldOrigins[index] == field.field) {
				strncpy(imguiExtraFields[index], value.c_str(), sizeof(imguiExtraFields[index]) - 1);
				break;
			}
	}
	return false;
}

bool Hud::getPaletteType(PaletteType& paletteType, MapObjectType type)
{
	switch (type) {
		case MapObjectType::motTerrain: paletteType = PaletteType::ptTerrain; break;
		case MapObjectType::motProp: paletteType = PaletteType::ptProp; break;
		case MapObjectType::motEnvironment: paletteType = PaletteType::ptEnvironment; break;
		case MapObjectType::motUnit: paletteType = PaletteType::ptUnit; break;
		case MapObjectType::motMerchant: paletteType = PaletteType::ptMerchant; break;
		case MapObjectType::motItem: paletteType = PaletteType::ptItem; break;
		case MapObjectType::motPortal: paletteType = PaletteType::ptPortal; break;
	}
	return true;
}

bool Hud::showMessage(std::string text, float time)
{
	this->notificationText = text;
	this->notificationDuration = time;
	this->notificationTimer = 0.f;
	return true;
}

bool Hud::matrixActivate(sf::Vector2f cursor)
{
	if (this->matrixTriggered || this->spawnPress || !this->matrixActivated || (this->manager->palette->status != PaletteStatus::psInsert && !this->wallActivated))
		return false;
	this->matrixTriggered = true;
	this->shapeMatrix->setPosition(cursor);
	std::static_pointer_cast<sf::RectangleShape>(this->shapeMatrix->shape)->setSize(sf::Vector2f(1.f, 1.f));
	this->shapeMatrix->visible = true;
	return true;
}

bool Hud::matrixDeactivate(sf::Vector2f cursor)
{
	if (!this->matrixTriggered)
		return false;
	this->matrixTriggered = false;
	this->shapeMatrix->visible = false;
	this->matrixGenerate(cursor);
	return true;
}

bool Hud::matrixGenerate(sf::Vector2f cursor)
{
	sf::Vector2f initialPosition(cursor.x - this->shapeMatrix->shape->getPosition().x, cursor.y - this->shapeMatrix->shape->getPosition().y);
	sf::Vector2f finalPosition(0.f, 0.f);

	if (initialPosition.x < 0.f) {
		initialPosition.x = cursor.x;
		finalPosition.x = this->shapeMatrix->shape->getPosition().x;
	} else {
		initialPosition.x = this->shapeMatrix->shape->getPosition().x;
		finalPosition.x = cursor.x;
	}
	if (initialPosition.y < 0.f) {
		initialPosition.y = cursor.y;
		finalPosition.y = this->shapeMatrix->shape->getPosition().y;
	} else {
		initialPosition.y = this->shapeMatrix->shape->getPosition().y;
		finalPosition.y = cursor.y;
	}

	sf::Vector2f realSize(finalPosition.x - initialPosition.x, finalPosition.y - initialPosition.y);

	// Start recording matrix spawns as one batch
	this->matrixSpawnInProgress = true;

	if (this->wallActivated)
	{
		if (!this->checkMapClick(cursor))
			return false;
		this->matrixPosSpawn = false;
		this->manager->palette->selectPalette(PaletteType::ptPortal);
		this->manager->palette->status = PaletteStatus::psInsert;
		this->manager->palette->selectedOrigin = "wall";
		this->manager->palette->selectedItem = "wall";
		this->shapeHover->shape->setPosition(initialPosition);
		this->shapeHover->visible = true;

		this->updateExtraEditsValue(
			{ "Width", "Height", "Index" },
			{ EditType::etInteger, EditType::etInteger, EditType::etInteger },
			{ boost::lexical_cast<std::string>(int(this->shapeMatrix->shape->getGlobalBounds().width)), boost::lexical_cast<std::string>(int(this->shapeMatrix->shape->getGlobalBounds().height)), "0" },
			{ 99999, 99999, 99 },
			{ "width", "height", "index" });

		this->setExtraEditsValue({
			boost::lexical_cast<std::string>(int(this->shapeMatrix->shape->getGlobalBounds().width)),
			boost::lexical_cast<std::string>(int(this->shapeMatrix->shape->getGlobalBounds().height)) });

		this->spawnClick(initialPosition);
		this->shapeHover->visible = false;
		this->matrixPosSpawn = true;
		return true;
	}

	sf::Vector2f textureSize(
		this->shapeHover->shape->getGlobalBounds().width / this->shapeHover->shape->getScale().x,
		this->shapeHover->shape->getGlobalBounds().height / this->shapeHover->shape->getScale().y);

	int lines = (int)(realSize.x / textureSize.x);
	int columns = (int)(realSize.y / textureSize.y);

	sf::Vector2f position = initialPosition;
	for (int x = 0; x < lines; x++) {
		position = sf::Vector2f(initialPosition.x + textureSize.x * x, initialPosition.y);
		this->spawnClick(position);
		for (int y = 0; y < columns; y++) {
			position = sf::Vector2f(initialPosition.x + textureSize.x * x, initialPosition.y + textureSize.y * y);
			this->shapeHover->shape->setPosition(position);
			this->spawnClick(position);
		}
	}
	// Record all matrix spawns as a single history entry
	this->matrixSpawnInProgress = false;
	if (this->historySpawnBuffer.size() > 0)
	{
		int totalItems = (int)this->historySpawnBuffer.size();
		std::string desc = this->wallActivated
			? "Spawned wall (" + boost::lexical_cast<std::string>(totalItems) + " item(s))"
			: "Matrix spawn (" + boost::lexical_cast<std::string>(totalItems) + " item(s))";
		this->recordHistory(HistoryActionType::hatSpawn, desc);
	}

	this->matrixPosSpawn = true;
	return true;
}

std::list<MapObjectField> Hud::getExtraEditValuesByType()
{
	std::list<MapObjectField> fields = {};
	std::vector<ImguiEditValue> extraValues = this->getExtraEditsValue();

	switch (this->manager->palette->type)
	{
		case (PaletteType::ptTerrain) :
		{
			fields.emplace_back(MapObjectField{ "allow-teleport", MapObjectFieldString{ "", false},
				MapObjectFieldInt{ 0, false },
				MapObjectFieldFloat{ 0.f, false },
				MapObjectFieldBool{ extraValues.at(0).boolean, true } });
			break;
		}
		case (PaletteType::ptProp):
		{
			fields.emplace_back(MapObjectField{ "variable", MapObjectFieldString{ extraValues.at(0).string, true} });
			fields.emplace_back(MapObjectField{ "destructible", MapObjectFieldString{ "", false},
				MapObjectFieldInt{ 0, false },
				MapObjectFieldFloat{ 0.f, false },
				MapObjectFieldBool{ extraValues.at(1).boolean, true } });
			fields.emplace_back(MapObjectField{ "death-allowed", MapObjectFieldString{ "", false},
				MapObjectFieldInt{ 0, false },
				MapObjectFieldFloat{ 0.f, false },
				MapObjectFieldBool{ extraValues.at(2).boolean, true } });
			break;
		}
		case (PaletteType::ptEnvironment):
		{
			fields.emplace_back(MapObjectField{ "front", MapObjectFieldString{ "", false}, MapObjectFieldInt{ 0, false }, MapObjectFieldFloat{ 0.f, false }, MapObjectFieldBool{ extraValues.at(0).boolean, true } });
			fields.emplace_back(MapObjectField{ "variable", MapObjectFieldString{ extraValues.at(1).string, true} });
			fields.emplace_back(MapObjectField{ "subtype", MapObjectFieldString{"", false}, MapObjectFieldInt{ extraValues.at(2).integer, true} });
			break;
		}
		case (PaletteType::ptUnit):
		{
			fields.emplace_back(MapObjectField{ "group", MapObjectFieldString{"", false}, MapObjectFieldInt{ extraValues.at(0).integer, true} });
			fields.emplace_back(MapObjectField{ "boss", MapObjectFieldString{ "", false}, MapObjectFieldInt{ 0, false }, MapObjectFieldFloat{ 0.f, false }, MapObjectFieldBool{ extraValues.at(1).boolean, true } });
			fields.emplace_back(MapObjectField{ "alliance", MapObjectFieldString{ extraValues.at(2).string, true} });
			fields.emplace_back(MapObjectField{ "item-drop", MapObjectFieldString{ extraValues.at(3).string, true} });
			fields.emplace_back(MapObjectField{ "variable", MapObjectFieldString{ extraValues.at(4).string, true} });
			break;
		}
		case (PaletteType::ptPortal):
		{
			if (this->manager->palette->selectedOrigin == "spawner")
			{
				fields.emplace_back(MapObjectField{ "default", MapObjectFieldString{"", false}, MapObjectFieldInt{ extraValues.at(0).integer, true } });
				fields.emplace_back(MapObjectField{ "index", MapObjectFieldString{"", false}, MapObjectFieldInt{ extraValues.at(1).integer, true } });
			}
			else if (this->manager->palette->selectedOrigin == "level")
			{
				fields.emplace_back(MapObjectField{ "group", MapObjectFieldString{"", false}, MapObjectFieldInt{ extraValues.at(0).integer, true } });
				fields.emplace_back(MapObjectField{ "index", MapObjectFieldString{"", false}, MapObjectFieldInt{ extraValues.at(1).integer, true } });
				fields.emplace_back(MapObjectField{ "target-index", MapObjectFieldString{"", false}, MapObjectFieldInt{ extraValues.at(2).integer, true } });
				fields.emplace_back(MapObjectField{ "width", MapObjectFieldString{"", false}, MapObjectFieldInt{ extraValues.at(3).integer, true } });
				fields.emplace_back(MapObjectField{ "height", MapObjectFieldString{"", false}, MapObjectFieldInt{ extraValues.at(4).integer, true } });
				fields.emplace_back(MapObjectField{ "map", MapObjectFieldString{extraValues.at(5).string, true} });
			}
			else if (this->manager->palette->selectedOrigin == "generator")
			{
				std::string unitTypePrefix = "characters/", unitTypeField = extraValues.at(5).string;
				boost::erase_all(unitTypeField, unitTypePrefix);
				unitTypeField = unitTypePrefix + unitTypeField;
				fields.emplace_back(MapObjectField{ "alliance", MapObjectFieldString{ extraValues.at(0).string, true } });
				fields.emplace_back(MapObjectField{ "index", MapObjectFieldString{"", false}, MapObjectFieldInt{ extraValues.at(1).integer, true } });
				fields.emplace_back(MapObjectField{ "target-x", MapObjectFieldString{"", false}, MapObjectFieldInt{ extraValues.at(2).integer, true } });
				fields.emplace_back(MapObjectField{ "target-y", MapObjectFieldString{"", false}, MapObjectFieldInt{ extraValues.at(3).integer, true } });
				fields.emplace_back(MapObjectField{ "cooldown", MapObjectFieldString{"", false}, MapObjectFieldInt{ extraValues.at(4).integer, true } });
				fields.emplace_back(MapObjectField{ "unit-type", MapObjectFieldString{ unitTypeField, true } });
			}
			else if (this->manager->palette->selectedOrigin == "wall")
			{
				fields.emplace_back(MapObjectField{ "width", MapObjectFieldString{"", false}, MapObjectFieldInt{ extraValues.at(0).integer, true } });
				fields.emplace_back(MapObjectField{ "height", MapObjectFieldString{"", false}, MapObjectFieldInt{ extraValues.at(1).integer, true } });
				fields.emplace_back(MapObjectField{ "index", MapObjectFieldString{"", false}, MapObjectFieldInt{ extraValues.at(2).integer, true } });
			}
			else if (this->manager->palette->selectedOrigin == "region")
			{
				fields.emplace_back(MapObjectField{ "width", MapObjectFieldString{"", false}, MapObjectFieldInt{ extraValues.at(0).integer, true } });
				fields.emplace_back(MapObjectField{ "height", MapObjectFieldString{"", false}, MapObjectFieldInt{ extraValues.at(1).integer, true } });
				fields.emplace_back(MapObjectField{ "index", MapObjectFieldString{"", false}, MapObjectFieldInt{ extraValues.at(2).integer, true } });
			}
			else if (this->manager->palette->selectedOrigin == "teleporter")
			{
				fields.emplace_back(MapObjectField{ "width", MapObjectFieldString{"", false}, MapObjectFieldInt{ extraValues.at(0).integer, true } });
				fields.emplace_back(MapObjectField{ "height", MapObjectFieldString{"", false}, MapObjectFieldInt{ extraValues.at(1).integer, true } });
				fields.emplace_back(MapObjectField{ "index", MapObjectFieldString{"", false}, MapObjectFieldInt{ extraValues.at(2).integer, true } });
				fields.emplace_back(MapObjectField{ "target-index", MapObjectFieldString{"", false}, MapObjectFieldInt{ extraValues.at(3).integer, true } });
			}
			else if (this->manager->palette->selectedOrigin == "slider")
			{
				fields.emplace_back(MapObjectField{ "width", MapObjectFieldString{"", false}, MapObjectFieldInt{ extraValues.at(0).integer, true } });
				fields.emplace_back(MapObjectField{ "height", MapObjectFieldString{"", false}, MapObjectFieldInt{ extraValues.at(1).integer, true } });
				fields.emplace_back(MapObjectField{ "index", MapObjectFieldString{"", false}, MapObjectFieldInt{ extraValues.at(2).integer, true } });
				fields.emplace_back(MapObjectField{ "speed-x", MapObjectFieldString{"", false}, MapObjectFieldInt{ extraValues.at(3).integer, true } });
				fields.emplace_back(MapObjectField{ "speed-y", MapObjectFieldString{"", false}, MapObjectFieldInt{ extraValues.at(4).integer, true } });
				fields.emplace_back(MapObjectField{ "invert-x", MapObjectFieldString{ "", false}, MapObjectFieldInt{ 0, false }, MapObjectFieldFloat{ 0.f, false }, MapObjectFieldBool{ extraValues.at(5).boolean, true } });
				fields.emplace_back(MapObjectField{ "invert-y", MapObjectFieldString{ "", false}, MapObjectFieldInt{ 0, false }, MapObjectFieldFloat{ 0.f, false }, MapObjectFieldBool{ extraValues.at(6).boolean, true } });
			}
			else if (this->manager->palette->selectedOrigin == "crusher")
			{
				fields.emplace_back(MapObjectField{ "width", MapObjectFieldString{"", false}, MapObjectFieldInt{ extraValues.at(0).integer, true } });
				fields.emplace_back(MapObjectField{ "height", MapObjectFieldString{"", false}, MapObjectFieldInt{ extraValues.at(1).integer, true } });
				fields.emplace_back(MapObjectField{ "index", MapObjectFieldString{"", false}, MapObjectFieldInt{ extraValues.at(2).integer, true } });
				fields.emplace_back(MapObjectField{ "damage", MapObjectFieldString{"", false}, MapObjectFieldInt{ extraValues.at(3).integer, true } });
			}
			else if (this->manager->palette->selectedOrigin == "connector")
			{
				fields.emplace_back(MapObjectField{ "width", MapObjectFieldString{"", false}, MapObjectFieldInt{ extraValues.at(0).integer, true } });
				fields.emplace_back(MapObjectField{ "height", MapObjectFieldString{"", false}, MapObjectFieldInt{ extraValues.at(1).integer, true } });
				fields.emplace_back(MapObjectField{ "direction", MapObjectFieldString{"", false}, MapObjectFieldInt{ extraValues.at(2).integer, true } });
			}
			else if (this->manager->palette->selectedOrigin == "exit")
			{
				fields.emplace_back(MapObjectField{ "width", MapObjectFieldString{"", false}, MapObjectFieldInt{ extraValues.at(0).integer, true } });
				fields.emplace_back(MapObjectField{ "height", MapObjectFieldString{"", false}, MapObjectFieldInt{ extraValues.at(1).integer, true } });
			}
			else if (this->manager->palette->selectedOrigin == "guardian")
			{
				fields.emplace_back(MapObjectField{ "width", MapObjectFieldString{"", false}, MapObjectFieldInt{ extraValues.at(0).integer, true } });
				fields.emplace_back(MapObjectField{ "height", MapObjectFieldString{"", false}, MapObjectFieldInt{ extraValues.at(1).integer, true } });
			}
			else if (this->manager->palette->selectedOrigin == "waygate")
			{
				fields.emplace_back(MapObjectField{ "width", MapObjectFieldString{"", false}, MapObjectFieldInt{ extraValues.at(0).integer, true } });
				fields.emplace_back(MapObjectField{ "height", MapObjectFieldString{"", false}, MapObjectFieldInt{ extraValues.at(1).integer, true } });
				fields.emplace_back(MapObjectField{ "index", MapObjectFieldString{"", false}, MapObjectFieldInt{ extraValues.at(2).integer, true } });
			}
			break;
		}
	}
	return fields;
}

bool Hud::spawnClick(sf::Vector2f cursor)
{
	if (this->matrixTriggered || this->itemSelectedMove)
		return false;
	if (this->matrixPosSpawn) {
		this->matrixPosSpawn = false;
		return false;
	}

	PaletteStatus status = this->manager->palette->status;
	if (status == PaletteStatus::psInsert && this->mouseRightButton)
		status = PaletteStatus::psDelete;

	switch (status)
	{
		case (PaletteStatus::psInsert):
		{
			if (this->manager->palette->selectedItem == "" || !this->shapeHover->visible)
				return false;

			MapObjectType objectType = MapObjectType::motTerrain;
			std::string paletteTypeField = "";
			std::string texture = this->manager->palette->selectedItem;
			std::list<MapObjectField> fields = this->getExtraEditValuesByType();
			sf::Vector2f tilesetPosition(this->shapeHover->shape->getPosition());
			sf::Vector2f tilesetOrigin(this->shapeHover->shape->getOrigin());
			sf::Vector2f hoverCenter(
				this->shapeHover->shape->getPosition().x + this->shapeHover->shape->getGlobalBounds().width / 2.f,
				this->shapeHover->shape->getPosition().y + this->shapeHover->shape->getGlobalBounds().height / 2.f);
			int priorityValue = this->priority;
			std::vector<ImguiEditValue> extraValues = this->getExtraEditsValue();

			switch (this->manager->palette->type)
			{
				case (PaletteType::ptTerrain) :
				{
					objectType = MapObjectType::motTerrain;
					paletteTypeField = this->manager->constant.gamePath + "/data/textures/terrain/";
					if (extraValues.at(1).boolean)
					{
						std::shared_ptr<Model> model = std::make_shared<Model>(this->manager,
							tilesetPosition,
							this->manager->constant.gamePath + "/data/textures/terrain/" + texture, priorityValue, false, "", this->manager->palette->selectedOrigin);
						model->sprite->setScale(this->scale, this->scale);
						model->setPosition(sf::Vector2f(model->getGlobalBounds().width, model->getGlobalBounds().height));
						this->manager->map->data.textureBackground = MapObjectUnit{ objectType, sf::Vector2f(model->getGlobalBounds().width, model->getGlobalBounds().height), 0.f, model, fields };
						this->setExtraEditsValue({ "true", "false"});
						this->manager->palette->clearPaletteItem();
						this->removeBgVisible = true;
						this->bgTexture = "terrain/" + texture;
						this->manager->hud->showMessage("Terrain background added!");
						return true;
					}
					break;
				}
				case (PaletteType::ptProp):
				{
					objectType = MapObjectType::motProp;
					paletteTypeField = this->manager->constant.gamePath + "/data/textures/prop/";
					break;
				}
				case (PaletteType::ptEnvironment):
				{
					objectType = MapObjectType::motEnvironment;
					paletteTypeField = this->manager->constant.gamePath + "/data/textures/environment/";
					break;
				}
				case (PaletteType::ptUnit):
				{
					objectType = MapObjectType::motUnit;
					paletteTypeField = "";
					texture = this->manager->palette->selectedTexture;
					priorityValue = 0;
					break;
				}
				case (PaletteType::ptMerchant):
				{
					objectType = MapObjectType::motMerchant;
					paletteTypeField = "";
					texture = this->manager->palette->selectedTexture;
					priorityValue = 0;
					break;
				}
				case (PaletteType::ptItem):
				{
					objectType = MapObjectType::motItem;
					paletteTypeField = "";
					texture = this->manager->palette->selectedTexture;
					priorityValue = 0;
					break;
				}
				case (PaletteType::ptPortal):
				{
					objectType = MapObjectType::motPortal;
					paletteTypeField = "";
					texture = "";
					priorityValue = this->manager->map->getObjectPriority(objectType);
					std::shared_ptr<Model> model = std::make_shared<Model>(this->manager, tilesetPosition, "", priorityValue, false, "", this->manager->palette->selectedOrigin);

					if (this->manager->palette->selectedOrigin == "spawner")
						this->manager->palette->loadPaletteShape(model, this->manager->palette->selectedOrigin);
					else if (this->manager->palette->selectedOrigin == "level")
						this->manager->palette->loadPaletteShape(model, this->manager->palette->selectedOrigin, sf::Vector2f(extraValues.at(3).integer, extraValues.at(4).integer));
					else if (this->manager->palette->selectedOrigin == "generator")
						this->manager->palette->loadPaletteShape(model, this->manager->palette->selectedOrigin);
					else if (this->manager->palette->selectedOrigin == "wall")
						this->manager->palette->loadPaletteShape(model, this->manager->palette->selectedOrigin, sf::Vector2f(extraValues.at(0).integer, extraValues.at(1).integer));
					else if (this->manager->palette->selectedOrigin == "region")
						this->manager->palette->loadPaletteShape(model, this->manager->palette->selectedOrigin, sf::Vector2f(extraValues.at(0).integer, extraValues.at(1).integer));
					else if (this->manager->palette->selectedOrigin == "teleporter")
						this->manager->palette->loadPaletteShape(model, this->manager->palette->selectedOrigin, sf::Vector2f(extraValues.at(0).integer, extraValues.at(1).integer));
					else if (this->manager->palette->selectedOrigin == "slider")
						this->manager->palette->loadPaletteShape(model, this->manager->palette->selectedOrigin, sf::Vector2f(extraValues.at(0).integer, extraValues.at(1).integer));
					else if (this->manager->palette->selectedOrigin == "crusher")
						this->manager->palette->loadPaletteShape(model, this->manager->palette->selectedOrigin, sf::Vector2f(extraValues.at(0).integer, extraValues.at(1).integer));
					else if (this->manager->palette->selectedOrigin == "connector")
						this->manager->palette->loadPaletteShape(model, this->manager->palette->selectedOrigin, sf::Vector2f(extraValues.at(0).integer, extraValues.at(1).integer));
					else if (this->manager->palette->selectedOrigin == "exit")
						this->manager->palette->loadPaletteShape(model, this->manager->palette->selectedOrigin, sf::Vector2f(extraValues.at(0).integer, extraValues.at(1).integer));
					else if (this->manager->palette->selectedOrigin == "guardian")
						this->manager->palette->loadPaletteShape(model, this->manager->palette->selectedOrigin, sf::Vector2f(extraValues.at(0).integer, extraValues.at(1).integer));
					else if (this->manager->palette->selectedOrigin == "waygate")
						this->manager->palette->loadPaletteShape(model, this->manager->palette->selectedOrigin, sf::Vector2f(extraValues.at(0).integer, extraValues.at(1).integer));

					model->setOrigin(tilesetOrigin);
					this->manager->addView(std::static_pointer_cast<ViewElement>(model));
					MapObjectUnit portalUnit{ objectType, model->getPosition(), 0.f, model, fields };
					this->manager->map->addObjectUnit(portalUnit);
					this->historySpawnBuffer.emplace_back(portalUnit);
					this->manager->palette->clearPaletteItem();
					this->recordHistory(HistoryActionType::hatSpawn, "Spawned portal: " + this->manager->palette->selectedOrigin);
					return true;
					break;
				}
			}

			priorityValue += this->manager->map->getObjectPriority(objectType);

			if (this->mousePressed && this->spawnPress)
				for (auto& object : this->manager->map->objects)
					if (object.type == objectType && object.model->sprite->getGlobalBounds().contains(hoverCenter) &&
						"textures/" + object.model->texture->filename == paletteTypeField + "/" + texture)
						return false;

			switch (this->manager->palette->formShape)
			{
				case (PaletteFormShape::fsNone):
				{
					for (int x = 0; x < (int)sqrt((float)this->brushSizeList.at(this->brushSize)); x++)
						for (int y = 0; y < (int)sqrt((float)this->brushSizeList.at(this->brushSize)); y++)
							this->spawnItem(tilesetPosition, paletteTypeField, texture, priorityValue, tilesetOrigin, x, y, objectType, fields);
					break;
				}
				case (PaletteFormShape::fsSquare):
				{
					if (this->formShapeSize < 2) break;
					sf::Vector2f lastPosition(
						tilesetPosition.x - ((this->formShapeSize - 1) / 2.f) * this->hoverShapeSize.x,
						tilesetPosition.y + ((this->formShapeSize - 1) / 2.f) * this->hoverShapeSize.y);
					int xAxis[4] = {0, 1, 0, -1};
					int yAxis[4] = {-1, 0, 1, 0};
					for (int x = 0; x < 4; x++)
						for (int y = 0; y < this->formShapeSize - 1; y++) {
							std::shared_ptr<Model> model = this->spawnItem(lastPosition, paletteTypeField, texture, priorityValue, tilesetOrigin, 0, 0, objectType, fields);
							lastPosition = sf::Vector2f(lastPosition.x + (model->getGlobalBounds().width * xAxis[x]), lastPosition.y + (model->getGlobalBounds().height * yAxis[x]));
						}
					break;
				}
				case (PaletteFormShape::fsCircle):
				{
					if (this->formShapeSize < 2) break;
					sf::Vector2f lastPosition(
						tilesetPosition.x - (this->formShapeSize * 1.33f) * this->hoverShapeSize.y,
						tilesetPosition.y + ((this->formShapeSize - 1) / 2.f) * this->hoverShapeSize.y);
					int xAxis[8] = {0, 1, 1, 1, 0, -1, -1, -1};
					int yAxis[8] = {-1, -1, 0, 1, 1, 1, 0, -1};
					for (int x = 0; x < 8; x++)
						for (int y = 0; y < this->formShapeSize - 1; y++) {
							std::shared_ptr<Model> model = this->spawnItem(lastPosition, paletteTypeField, texture, priorityValue, tilesetOrigin, 0, 0, objectType, fields);
							lastPosition = sf::Vector2f(lastPosition.x + (model->getGlobalBounds().width * xAxis[x]), lastPosition.y + (model->getGlobalBounds().height * yAxis[x]));
						}
					break;
				}
			}
			// Record spawned items in history after the form shape block
			if (this->historySpawnBuffer.size() > 0 && !this->matrixSpawnInProgress)
			{
				std::string desc = "Spawned " + boost::lexical_cast<std::string>(this->historySpawnBuffer.size()) + " item(s)";
				this->recordHistory(HistoryActionType::hatSpawn, desc);
			}
			this->resetExtraEditsValue();
			break;
		}

		case (PaletteStatus::psDelete):
		{
			if (!this->checkMapClick(cursor))
				return false;
			MapObjectUnit objectSelected{ MapObjectType::motTerrain , sf::Vector2f(0.f, 0.f), 0.f, nullptr, {} };
			for (auto& object : this->manager->map->objects)
				if (object.model->getGlobalBounds().contains(cursor)) {
					if (objectSelected.model != nullptr && objectSelected.model->priority < object.model->priority)
						continue;
					objectSelected = object;
				}
			if (objectSelected.model) {
				this->historySpawnBuffer.emplace_back(objectSelected);
				std::vector<MapObjectUnit> objectList = this->updateBitmaskRemove(objectSelected);
				this->manager->map->removeObjectUnit(objectSelected);
				this->updateBitmaskList(objectList);
				objectList.clear();
				this->recordHistory(HistoryActionType::hatDelete, "Deleted 1 item");
			}
			break;
		}
		default: return false;
	}
	return true;
}

std::shared_ptr<Model> Hud::spawnItem(sf::Vector2f tilesetPosition, std::string paletteTypeField, std::string texture, int priorityValue,
	sf::Vector2f tilesetOrigin, int x, int y, MapObjectType objectType, std::list<MapObjectField> fields)
{
	std::string filename = paletteTypeField + texture;
	std::shared_ptr<Model> model = std::make_shared<Model>(this->manager, tilesetPosition, filename, priorityValue, false, "", this->manager->palette->selectedOrigin);
	model->autoPriority = this->manager->map->getObjectAutoPriority(objectType);
	model->sprite->setOrigin(tilesetOrigin);
	model->sprite->move(model->sprite->getGlobalBounds().width * x, model->sprite->getGlobalBounds().height * y);
	model->sprite->setRotation(this->rotation);
	model->sprite->setScale(this->scale, this->scale);
	this->manager->addView(std::static_pointer_cast<ViewElement>(model));
	MapObjectUnit newUnit{ objectType, model->sprite->getPosition(), 0.f, model, fields };
	this->manager->map->addObjectUnit(newUnit);
	this->historySpawnBuffer.emplace_back(newUnit);

	if (model->texture->bitmask)
	{
		std::vector<sf::Vector2f> bitmaskPositionList = {
			sf::Vector2f(model->sprite->getGlobalBounds().left + (model->sprite->getGlobalBounds().width * 0.5f), model->sprite->getGlobalBounds().top - (model->sprite->getGlobalBounds().height * 0.5f)),
			sf::Vector2f(model->sprite->getGlobalBounds().left - (model->sprite->getGlobalBounds().width * 0.5f), model->sprite->getGlobalBounds().top + (model->sprite->getGlobalBounds().height * 0.5f)),
			sf::Vector2f(model->sprite->getGlobalBounds().left + (model->sprite->getGlobalBounds().width * 1.5f), model->sprite->getGlobalBounds().top + (model->sprite->getGlobalBounds().height * 0.5f)),
			sf::Vector2f(model->sprite->getGlobalBounds().left + (model->sprite->getGlobalBounds().width * 0.5f), model->sprite->getGlobalBounds().top + (model->sprite->getGlobalBounds().height * 1.5f)) };
		int index = 1, bitmaskSum = 0;
		std::vector<MapObjectUnit> bitmapObjects = {};
		for (auto& bitmaskPosition : bitmaskPositionList)
		{
			for (auto& object : this->manager->map->objects)
				if (object.type == MapObjectType::motTerrain && object.model && object.model->filename == filename && object.model->sprite->getGlobalBounds().contains(bitmaskPosition))
				{
					bitmapObjects.emplace_back(object);
					bitmaskSum += index;
					break;
				}
			index *= 2;
		}
		model->animation = "stand:" + boost::lexical_cast<std::string>(bitmaskSum);
		model->loadSprite(model->filename, model->position);
		for (auto& object : bitmapObjects)
			this->updateBitmask(object);
	}
	return model;
}

bool Hud::updateBitmask(MapObjectUnit object)
{
	if (object.type != MapObjectType::motTerrain)
		return false;
	std::vector<sf::Vector2f> bitmaskPositionList = {
		sf::Vector2f(object.model->sprite->getGlobalBounds().left + (object.model->sprite->getGlobalBounds().width * 0.5f), object.model->sprite->getGlobalBounds().top - (object.model->sprite->getGlobalBounds().height * 0.5f)),
		sf::Vector2f(object.model->sprite->getGlobalBounds().left - (object.model->sprite->getGlobalBounds().width * 0.5f), object.model->sprite->getGlobalBounds().top + (object.model->sprite->getGlobalBounds().height * 0.5f)),
		sf::Vector2f(object.model->sprite->getGlobalBounds().left + (object.model->sprite->getGlobalBounds().width * 1.5f), object.model->sprite->getGlobalBounds().top + (object.model->sprite->getGlobalBounds().height * 0.5f)),
		sf::Vector2f(object.model->sprite->getGlobalBounds().left + (object.model->sprite->getGlobalBounds().width * 0.5f), object.model->sprite->getGlobalBounds().top + (object.model->sprite->getGlobalBounds().height * 1.5f)) };
	int index = 1, bitmaskSum = 0;
	for (auto& bitmaskPosition : bitmaskPositionList)
	{
		for (auto& subObject : this->manager->map->objects)
			if (subObject.type == MapObjectType::motTerrain && subObject.model && subObject.model->filename == object.model->filename && subObject.model->sprite->getGlobalBounds().contains(bitmaskPosition))
			{
				bitmaskSum += index;
				break;
			}
		index *= 2;
	}
	object.model->animation = "stand:" + boost::lexical_cast<std::string>(bitmaskSum);
	object.model->loadSprite(object.model->filename, object.model->position);
	return true;
}

bool Hud::updateBitmaskList(std::vector<MapObjectUnit> list)
{
	for (auto& object : list)
		this->updateBitmask(object);
	return true;
}

std::vector<MapObjectUnit> Hud::updateBitmaskRemove(MapObjectUnit object)
{
	if (object.type != MapObjectType::motTerrain || !object.model->texture->bitmask)
		return {};
	std::vector<sf::Vector2f> bitmaskPositionList = {
		sf::Vector2f(object.model->sprite->getGlobalBounds().left + (object.model->sprite->getGlobalBounds().width * 0.5f), object.model->sprite->getGlobalBounds().top - (object.model->sprite->getGlobalBounds().height * 0.5f)),
		sf::Vector2f(object.model->sprite->getGlobalBounds().left - (object.model->sprite->getGlobalBounds().width * 0.5f), object.model->sprite->getGlobalBounds().top + (object.model->sprite->getGlobalBounds().height * 0.5f)),
		sf::Vector2f(object.model->sprite->getGlobalBounds().left + (object.model->sprite->getGlobalBounds().width * 1.5f), object.model->sprite->getGlobalBounds().top + (object.model->sprite->getGlobalBounds().height * 0.5f)),
		sf::Vector2f(object.model->sprite->getGlobalBounds().left + (object.model->sprite->getGlobalBounds().width * 0.5f), object.model->sprite->getGlobalBounds().top + (object.model->sprite->getGlobalBounds().height * 1.5f)) };
	std::vector<MapObjectUnit> bitmapObjects = {};
	for (auto& bitmaskPosition : bitmaskPositionList)
		for (auto& subObject : this->manager->map->objects)
			if (subObject.type == MapObjectType::motTerrain && subObject.model && object.model->filename == subObject.model->filename && subObject.model->sprite->getGlobalBounds().contains(bitmaskPosition))
			{
				bitmapObjects.emplace_back(subObject);
				break;
			}
	return bitmapObjects;
}

bool Hud::updateExtraEditsValue(std::vector<std::string> caption, std::vector<EditType> type, std::vector<std::string> value, std::vector<int> maxValue, std::vector<std::string> origin)
{
	this->extraFieldCaptions = caption;
	this->extraFieldTypes = type;
	this->extraFieldMaxValues = maxValue;
	this->extraFieldOrigins = origin;
	this->gettingExtraValues = true;
	for (int index = 0; index < 7; index++)
		memset(imguiExtraFields[index], 0, sizeof(imguiExtraFields[index]));
	for (int index = 0; index < (int)value.size() && index < 7; index++)
		strncpy(imguiExtraFields[index], value[index].c_str(), sizeof(imguiExtraFields[index]) - 1);
	return true;
}

bool Hud::setExtraEditsValue(std::vector<std::string> value)
{
	for (int index = 0; index < (int)value.size() && index < 7; index++)
		strncpy(imguiExtraFields[index], value[index].c_str(), sizeof(imguiExtraFields[index]) - 1);
	return true;
}

bool Hud::setExtraEditValue(std::string value, int index)
{
	if (index >= 0 && index < 7)
		strncpy(imguiExtraFields[index], value.c_str(), sizeof(imguiExtraFields[index]) - 1);
	return true;
}

bool Hud::resetExtraEditsValue()
{
	if (!this->manager->palette)
		return false;
	switch (this->manager->palette->type)
	{
		case (PaletteType::ptTerrain):
			this->setExtraEditsValue({ "true", "false" });
			break;
		case (PaletteType::ptProp):
			this->setExtraEditsValue({ "" });
			break;
		case (PaletteType::ptEnvironment):
			this->setExtraEditValue("", 1);
			break;
		case (PaletteType::ptUnit):
			this->setExtraEditsValue({ "0", "false", "enemy", "", "" });
			break;
	}
	return true;
}

std::vector<ImguiEditValue> Hud::getExtraEditsValue()
{
	std::vector<ImguiEditValue> values = {};
	for (int index = 0; index < 7; index++)
	{
		std::string val(imguiExtraFields[index]);
		ImguiEditValue editValue;
		editValue.string = val;
		editValue.integer = atoi(val.c_str());
		editValue.boolean = (val == "true" || val == "1");
		editValue.active = (val.length() > 0);
		values.emplace_back(editValue);
	}
	return values;
}

bool Hud::getCheckEditing()
{
	return ImGui::IsAnyItemActive();
}

bool Hud::unloadLists()
{
	for (auto& model : this->models)
		this->manager->removeView(std::static_pointer_cast<ViewElement>(model));
	this->models.clear();
	for (auto& gridIndex : this->grid)
		this->manager->removeView(std::static_pointer_cast<ViewElement>(gridIndex));
	this->grid.clear();
	return true;
}

bool Hud::loadGrid()
{
	for (auto& gridIndex : this->grid)
		this->manager->removeView(std::static_pointer_cast<ViewElement>(gridIndex));
	this->grid.clear();

	sf::Vector2f distance((float)this->gridSizeList.at(this->gridSize), (float)this->gridSizeList.at(this->gridSize));
	float gridSizeValue = this->gridSizeList.at(this->gridSize) / 32.f;

	for (int x = 0; x < (int)(40 / gridSizeValue); x++)
	{
		std::shared_ptr<Model> line = std::make_shared<Model>(this->manager, sf::Vector2f(0.f, x * distance.y), "", 3);
		line->loadShape(sf::Vector2f(4000.f, 1.f), sf::Color(0, 255, 0, 100));
		this->manager->addView(std::static_pointer_cast<ViewElement>(line));
		this->grid.emplace_back(line);
	}

	for (int y = 0; y < (int)(96 / gridSizeValue); y++)
	{
		std::shared_ptr<Model> line = std::make_shared<Model>(this->manager, sf::Vector2f(y * distance.x, 0.f), "", 3);
		line->loadShape(sf::Vector2f(1.f, 1800.f), sf::Color(0, 255, 0, 100));
		this->manager->addView(std::static_pointer_cast<ViewElement>(line));
		this->grid.emplace_back(line);
	}

	for (auto& gridIndex : this->grid)
		gridIndex->visible = this->gridVisible;
	return true;
}

bool Hud::loadModels()
{
	std::shared_ptr<Model> messageBox = std::make_shared<Model>(this->manager, sf::Vector2f(0.f, 900.f));
	messageBox->loadShape(sf::Vector2f(30.f, 1.f), sf::Color(100, 100, 100, 255));
	this->manager->addView(std::static_pointer_cast<ViewElement>(messageBox));
	this->models.emplace_back(messageBox);
	messageBox->visible = false;

	this->shapeMinimap = std::make_shared<Model>(this->manager,
		sf::Vector2f(this->manager->constant.minimapSize.left * this->manager->window->getSize().x,
			this->manager->constant.minimapSize.top * this->manager->window->getSize().y -
			(this->manager->constant.minimapSize.top * 60.f)), "", 0, true);
	this->shapeMinimap->loadShape(sf::Vector2f(1.f, 1.f), sf::Color(0, 0, 0, 255));
	this->manager->addView(std::static_pointer_cast<ViewElement>(this->shapeMinimap));
	return true;
}

bool Hud::loadLists()
{
	this->unloadLists();
	this->loadGrid();
	this->loadModels();
	return true;
}

bool Hud::clearHistory()
{
	this->undoStack.clear();
	this->redoStack.clear();
	this->historySpawnBuffer.clear();
	return true;
}

bool Hud::recordHistory(HistoryActionType type, std::string description)
{
	if (this->historySpawnBuffer.empty() && type == HistoryActionType::hatSpawn)
		return false;

	HistoryEntry entry;
	entry.type = type;
	entry.objects = this->historySpawnBuffer;
	entry.description = description;

	this->historySpawnBuffer.clear();

	// New action clears redo stack
	this->redoStack.clear();

	this->undoStack.emplace_back(entry);

	// Limit undo stack size
	if ((int)this->undoStack.size() > historyMaxSize)
		this->undoStack.erase(this->undoStack.begin());

	return true;
}

bool Hud::undoAction()
{
	if (this->undoStack.empty())
		return false;

	HistoryEntry entry = this->undoStack.back();
	this->undoStack.pop_back();

	if (entry.objects.empty())
		return false;

	if (entry.type == HistoryActionType::hatSpawn)
	{
		// Remove all spawned items
		for (auto& obj : entry.objects)
		{
			std::vector<MapObjectUnit> objectList = this->updateBitmaskRemove(obj);
			this->manager->map->removeObjectUnit(obj);
			this->updateBitmaskList(objectList);
		}
		this->showMessage("Undo: " + entry.description, 2.f);
	}
	else if (entry.type == HistoryActionType::hatDelete)
	{
		// Add all deleted items back
		for (auto& obj : entry.objects)
		{
			this->manager->addView(std::static_pointer_cast<ViewElement>(obj.model));
			this->manager->map->addObjectUnit(obj);
		}
		this->showMessage("Undo: " + entry.description, 2.f);
	}

	this->redoStack.emplace_back(entry);
	return true;
}

bool Hud::redoAction()
{
	if (this->redoStack.empty())
		return false;

	HistoryEntry entry = this->redoStack.back();
	this->redoStack.pop_back();

	if (entry.objects.empty())
		return false;

	if (entry.type == HistoryActionType::hatSpawn)
	{
		// Re-add all spawned items
		for (auto& obj : entry.objects)
		{
			this->manager->addView(std::static_pointer_cast<ViewElement>(obj.model));
			this->manager->map->addObjectUnit(obj);
		}
		this->showMessage("Redo: " + entry.description, 2.f);
	}
	else if (entry.type == HistoryActionType::hatDelete)
	{
		// Remove all items that were re-added by undo
		for (auto& obj : entry.objects)
		{
			std::vector<MapObjectUnit> objectList = this->updateBitmaskRemove(obj);
			this->manager->map->removeObjectUnit(obj);
			this->updateBitmaskList(objectList);
		}
		this->showMessage("Redo: " + entry.description, 2.f);
	}

	this->undoStack.emplace_back(entry);
	return true;
}

	// IMGUI RENDERING

bool Hud::imguiRender()
{
	this->imguiRenderMenuBar();
	this->imguiRenderToolPanel();
	this->imguiRenderPalettePanel();
	this->imguiRenderNotification();
	this->imguiRenderPreferencesWindow();
	return true;
}

void Hud::imguiRenderMenuBar()
{
	if (ImGui::BeginMainMenuBar())
	{
		if (ImGui::BeginMenu("File"))
		{
			if (ImGui::MenuItem("New Map")) { this->manager->map->newMap(); }
			if (ImGui::MenuItem("Open...", "Ctrl+O")) { this->manager->map->loadMap(); }
			if (ImGui::MenuItem("Save", "Ctrl+S")) { this->manager->map->saveMap(); }
			if (ImGui::MenuItem("Save As...")) { this->manager->map->saveMapAs(); }
			ImGui::Separator();
			if (ImGui::MenuItem("Reload Map")) { this->manager->map->reloadMap(); }
			if (ImGui::MenuItem("Reload Config")) { this->manager->loadConstants(); }
			if (ImGui::MenuItem("Create Trigger File")) { this->manager->map->createTriggerFile(); }
			ImGui::Separator();
			ImGui::Separator();
			if (ImGui::MenuItem("Map Preferences...")) { this->showPreferencesWindow = true; }
			if (ImGui::MenuItem("Help")) { this->help(); }
			ImGui::EndMenu();
		}

		// Quick-access file operation buttons on the menu bar
		ImGui::SameLine(); ImGui::Spacing(); ImGui::SameLine();
		if (ImGui::Button("New")) { this->manager->map->newMap(); } ImGui::SameLine();
		if (ImGui::Button("Open")) { this->manager->map->loadMap(); } ImGui::SameLine();
		if (ImGui::Button("Save")) { this->manager->map->saveMap(); } ImGui::SameLine();
		if (ImGui::Button("Save As")) { this->manager->map->saveMapAs(); } ImGui::SameLine();
		if (ImGui::Button("Reload")) { this->manager->map->reloadMap(); } ImGui::SameLine();
		if (ImGui::Button("Reload Cfg")) { this->manager->loadConstants(); } ImGui::SameLine();
		if (ImGui::Button("Trigger")) { this->manager->map->createTriggerFile(); } ImGui::SameLine();
		if (ImGui::Button("Help")) { this->help(); }

		ImGui::EndMainMenuBar();
	}
}

void Hud::imguiRenderToolPanel()
{
	ImGui::SetNextWindowPos(ImVec2(0, ImGui::GetFrameHeight()), ImGuiCond_FirstUseEver);
	ImGui::SetNextWindowSize(ImVec2(400, 0), ImGuiCond_FirstUseEver);
	ImGui::SetNextWindowBgAlpha(0.85f);

	ImGuiWindowFlags flags = ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoBringToFrontOnFocus;

	ImGui::Begin("Tools", NULL, flags);
	ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(4, 4));

	// Grid & Brush
	ImGui::Separator();
	ImGui::Text("Grid:"); ImGui::SameLine();
	if (ImGui::Button("<##grid")) { this->changeGridSize(-1); } ImGui::SameLine();
	ImGui::TextUnformatted((boost::lexical_cast<std::string>(this->gridSizeList.at(this->gridSize)) + "px").c_str()); ImGui::SameLine();
	if (ImGui::Button(">##grid")) { this->changeGridSize(1); } ImGui::SameLine();
	ImGui::Spacing(); ImGui::SameLine();
	if (ImGui::Button(this->gridVisible ? "G: ON" : "G: OFF")) { this->toggleGridVisibility(); } ImGui::SameLine();
	ImGui::Spacing(); ImGui::SameLine();
	ImGui::Text("Brush:"); ImGui::SameLine();
	if (ImGui::Button("<##brush")) { this->changeBrushSize(-1); } ImGui::SameLine();
	ImGui::TextUnformatted((boost::lexical_cast<std::string>(this->brushSizeList.at(this->brushSize)) + "x").c_str()); ImGui::SameLine();
	if (ImGui::Button(">##brush")) { this->changeBrushSize(1); }

	// Tool buttons
	ImGui::Separator();
	auto renderToggleBtn = [&](const char* label, bool& state) {
		if (state) {
			ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.2f, 0.6f, 0.6f, 0.8f));
			ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.3f, 0.7f, 0.7f, 1.0f));
		}
		if (ImGui::Button(label)) { state = !state; }
		if (state) ImGui::PopStyleColor(2);
	};

	renderToggleBtn("C", this->itemSelect); ImGui::SameLine();
	ImGui::Text("Clear"); ImGui::SameLine(); ImGui::Spacing(); ImGui::SameLine();
	if (ImGui::Button("[E]")) { this->manager->palette->erasePaletteItem(); }
	ImGui::SameLine(); ImGui::Text("Erase"); ImGui::SameLine(); ImGui::Spacing(); ImGui::SameLine();
	renderToggleBtn("G", this->gridVisible);
	ImGui::SameLine(); ImGui::Text("Grid"); ImGui::SameLine(); ImGui::Spacing(); ImGui::SameLine();
	renderToggleBtn("P", this->spawnPress);
	ImGui::SameLine(); ImGui::Text("Spawn"); ImGui::SameLine(); ImGui::Spacing(); ImGui::SameLine();
	renderToggleBtn("S", this->centerShape);
	ImGui::SameLine(); ImGui::Text("Center"); ImGui::SameLine(); ImGui::Spacing(); ImGui::SameLine();
	renderToggleBtn("M", this->matrixActivated);
	ImGui::SameLine(); ImGui::Text("Matrix"); ImGui::SameLine(); ImGui::Spacing(); ImGui::SameLine();
	renderToggleBtn("^", this->dragCursor);
	ImGui::SameLine(); ImGui::Text("Drag"); ImGui::SameLine(); ImGui::Spacing(); ImGui::SameLine();
	renderToggleBtn("W", this->wallActivated);
	ImGui::SameLine(); ImGui::Text("Wall"); ImGui::SameLine(); ImGui::Spacing(); ImGui::SameLine();
	renderToggleBtn("A", this->shapeMapArea->visible);
	ImGui::SameLine(); ImGui::Text("Area"); ImGui::SameLine(); ImGui::Spacing(); ImGui::SameLine();
	renderToggleBtn("D", this->gridSpawn);
	ImGui::SameLine(); ImGui::Text("Grid Off"); ImGui::SameLine(); ImGui::Spacing(); ImGui::SameLine();

	if (ImGui::Button("[I]")) { this->manager->palette->clearPaletteItem(); this->itemSelect = true; }
	ImGui::SameLine(); ImGui::Text("Select"); ImGui::SameLine(); ImGui::Spacing(); ImGui::SameLine();
	renderToggleBtn("->", this->itemSelectedMove);
	ImGui::SameLine(); ImGui::Text("Move"); ImGui::SameLine(); ImGui::Spacing(); ImGui::SameLine();
	if (ImGui::Button("B")) { this->updateMapBounds(); }
	ImGui::SameLine(); ImGui::Text("Bounds");

	// Form shape
	ImGui::Separator();
	ImGui::Text("Form:"); ImGui::SameLine();
	if (this->formShapeSelected == "square") {
		ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.2f, 0.6f, 0.6f, 0.8f));
	}
	if (ImGui::Button("Square")) { this->formShapeClick("square"); }
	if (this->formShapeSelected == "square") ImGui::PopStyleColor();
	ImGui::SameLine();
	if (this->formShapeSelected == "circle") {
		ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.2f, 0.6f, 0.6f, 0.8f));
	}
	if (ImGui::Button("Circle")) { this->formShapeClick("circle"); }
	if (this->formShapeSelected == "circle") ImGui::PopStyleColor();
	ImGui::SameLine();
	if (this->formShapeSelected == "none") {
		ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.2f, 0.6f, 0.6f, 0.8f));
	}
	if (ImGui::Button("None")) { this->formShapeClick("none"); }
	if (this->formShapeSelected == "none") ImGui::PopStyleColor();
	ImGui::SameLine(0, 20);
	ImGui::Text("Form Size:"); ImGui::SameLine();
	ImGui::PushItemWidth(60);
	ImGui::InputInt("##formSize", &this->formShapeSize, 0, 0);
	if (this->formShapeSize < 0) this->formShapeSize = 0;
	if (this->formShapeSize > 255) this->formShapeSize = 255;
	ImGui::PopItemWidth();

	// Object properties
	ImGui::Separator();
	ImGui::Text("Rotation:"); ImGui::SameLine();
	ImGui::PushItemWidth(60);
	if (ImGui::InputInt("##rot", &this->rotation, 0, 0)) {
		if (this->rotation < 0) this->rotation = 0;
		if (this->rotation > 360) this->rotation = 360;
	}
	ImGui::SameLine(0, 15);
	ImGui::Text("Scale%%:"); ImGui::SameLine();
	int scalePct = (int)(this->scale * 100);
	if (ImGui::InputInt("##scale", &scalePct, 0, 0)) {
		if (scalePct < 0) scalePct = 0;
		if (scalePct > 1000) scalePct = 1000;
		this->scale = scalePct / 100.f;
	}
	ImGui::SameLine(0, 15);
	ImGui::Text("Priority:"); ImGui::SameLine();
	if (ImGui::InputInt("##prio", &this->priority, 0, 0)) {
		if (this->priority < 0) this->priority = 0;
		if (this->priority > 100) this->priority = 100;
	}
	ImGui::PopItemWidth();

	// Extra fields
	if (this->gettingExtraValues && this->extraFieldCaptions.size() > 0)
	{
		ImGui::Separator();
		ImGui::Text("Properties:");
		for (int index = 0; index < (int)this->extraFieldCaptions.size() && index < 7; index++)
		{
			ImGui::Text("%s:", this->extraFieldCaptions[index].c_str()); ImGui::SameLine();
			switch (this->extraFieldTypes[index])
			{
				case EditType::etString:
					ImGui::PushItemWidth(120);
					ImGui::InputText(("##ef" + boost::lexical_cast<std::string>(index)).c_str(), imguiExtraFields[index], sizeof(imguiExtraFields[index]));
					ImGui::PopItemWidth();
					break;
				case EditType::etInteger:
				{
					int val = atoi(imguiExtraFields[index]);
					ImGui::PushItemWidth(80);
					if (ImGui::InputInt(("##ef" + boost::lexical_cast<std::string>(index)).c_str(), &val, 0, 0))
					{
						if (this->extraFieldMaxValues[index] > 0 && val > this->extraFieldMaxValues[index])
							val = this->extraFieldMaxValues[index];
						if (val < 0) val = 0;
						strncpy(imguiExtraFields[index], boost::lexical_cast<std::string>(val).c_str(), sizeof(imguiExtraFields[index]) - 1);
					}
					ImGui::PopItemWidth();
					break;
				}
				case EditType::etBoolean:
				{
					bool val = (std::string(imguiExtraFields[index]) == "true");
					if (ImGui::Checkbox(("##ef" + boost::lexical_cast<std::string>(index)).c_str(), &val))
						strncpy(imguiExtraFields[index], val ? "true" : "false", sizeof(imguiExtraFields[index]) - 1);
					ImGui::SameLine();
					ImGui::TextUnformatted(val ? "true" : "false");
					break;
				}
			}
		}
	}

	ImGui::PopStyleVar();
	ImGui::End();
}

void Hud::imguiRenderPalettePanel()
{
	ImGui::SetNextWindowPos(ImVec2(ImGui::GetIO().DisplaySize.x - 340, ImGui::GetFrameHeight()), ImGuiCond_FirstUseEver);
	ImGui::SetNextWindowSize(ImVec2(340, ImGui::GetIO().DisplaySize.y - ImGui::GetFrameHeight() * 2), ImGuiCond_FirstUseEver);
	ImGui::SetNextWindowBgAlpha(0.85f);

	ImGuiWindowFlags flags = ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoBringToFrontOnFocus;

	ImGui::Begin("Palette", NULL, flags);

	sf::Vector2f mousePos = this->manager->getMousePosition();
	ImGui::Text("(%d, %d)", (int)mousePos.x, (int)mousePos.y);

	switch (this->manager->palette->status)
	{
		case PaletteStatus::psInsert:
			ImGui::TextColored(ImVec4(0,1,0,1), "INSERTING"); break;
		case PaletteStatus::psDelete:
			ImGui::TextColored(ImVec4(1,0,0,1), "DELETING"); break;
		default:
			ImGui::Text(this->itemSelect ? "SELECT" : "- - -");
	}

	ImGui::Separator();
	ImGui::Text("Palette Types:");
	auto renderPaletteBtn = [&](const char* label, PaletteType type) {
		bool selected = (this->manager->palette->type == type);
		if (selected) ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.2f, 0.5f, 0.8f, 0.8f));
		if (ImGui::Button(label)) { this->manager->palette->selectPalette(type); }
		if (selected) ImGui::PopStyleColor();
	};

	renderPaletteBtn("Units", PaletteType::ptUnit); ImGui::SameLine();
	renderPaletteBtn("Merchants", PaletteType::ptMerchant); ImGui::SameLine();
	renderPaletteBtn("Items", PaletteType::ptItem);
	renderPaletteBtn("Props", PaletteType::ptProp); ImGui::SameLine();
	renderPaletteBtn("Environments", PaletteType::ptEnvironment); ImGui::SameLine();
	renderPaletteBtn("Terrain", PaletteType::ptTerrain);
	renderPaletteBtn("Portals", PaletteType::ptPortal);

	ImGui::Separator();
	if (this->removeBgVisible) {
		ImGui::Text("BG: %s", this->bgTexture.c_str());
		if (ImGui::Button("Remove BG")) { this->removeBackground(); }
	}

	ImGui::Separator();
	ImGui::Text("Page:"); ImGui::SameLine();
	if (ImGui::Button("<##page")) {
		if (this->manager->palette->pageIndex > 0) {
			this->manager->palette->pageIndex--;
			this->manager->palette->selectPalette(this->manager->palette->type);
		}
	}
	ImGui::SameLine();
	ImGui::Text(" %d ", this->manager->palette->pageIndex + 1);
	ImGui::SameLine();
	if (ImGui::Button(">##page")) {
		this->manager->palette->pageIndex++;
		this->manager->palette->selectPalette(this->manager->palette->type);
	}
	ImGui::SameLine();
	ImGui::Text("  Selected: %s", this->manager->palette->selectedItem.c_str());

	ImGui::Separator();
	this->imguiRenderPaletteItems();

	ImGui::End();
}

void Hud::imguiRenderNotification()
{
	if (this->notificationText.empty())
		return;

	this->notificationTimer += ImGui::GetIO().DeltaTime;

	if (this->notificationTimer >= this->notificationDuration)
	{
		this->notificationText.clear();
		return;
	}

	float alpha = 1.0f;
	if (this->notificationTimer > this->notificationDuration - 0.5f)
		alpha = (this->notificationDuration - this->notificationTimer) / 0.5f;

	ImGuiViewport* viewport = ImGui::GetMainViewport();
	float windowWidth = viewport->Size.x;
	float textWidth = ImGui::CalcTextSize(this->notificationText.c_str()).x;

	ImGui::SetNextWindowPos(ImVec2(windowWidth * 0.5f, viewport->Size.y - 60.0f), ImGuiCond_Always, ImVec2(0.5f, 1.0f));
	ImGui::SetNextWindowBgAlpha(alpha * 0.85f);

	ImGuiWindowFlags flags = ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove |
		ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_AlwaysAutoResize |
		ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoInputs;

	ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.0f, 1.0f, 0.0f, alpha));
	ImGui::Begin("##notification", NULL, flags);
	ImGui::TextUnformatted(this->notificationText.c_str());
	ImGui::End();
	ImGui::PopStyleColor();
}

void Hud::imguiRenderPaletteItems()
{
	if (this->manager->palette->paletteItems.empty())
	{
		ImGui::TextColored(ImVec4(0.5f, 0.5f, 0.5f, 1.0f), "No items loaded");
		return;
	}

	int itemsPerRow = 4;
	int itemIndex = 0;

	ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(6, 6));

	for (auto& item : this->manager->palette->paletteItems)
	{
		bool isSelected = (item.filename == this->manager->palette->selectedItem);

		// Highlight selected item with a colored border
		if (isSelected)
		{
			ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.2f, 0.6f, 0.9f, 1.0f));
			ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.3f, 0.7f, 1.0f, 1.0f));
		}

		bool clicked = false;

		if (item.model->sprite)
		{
			// Display sprite-based items as image buttons
			clicked = ImGui::ImageButton(*item.model->sprite, ImVec2(64, 64));
		}
		else if (item.model->shape)
		{
			// For portal/other items without sprites, show a colored button
			sf::Color shapeColor = item.model->shape->getFillColor();
			ImVec4 col(shapeColor.r / 255.f, shapeColor.g / 255.f, shapeColor.b / 255.f, shapeColor.a / 255.f);
			ImGui::PushStyleColor(ImGuiCol_Button, col);
			clicked = ImGui::Button(item.filename.c_str(), ImVec2(64, 64));
			ImGui::PopStyleColor();
		}
		else
		{
			// Fallback: show as text button
			clicked = ImGui::Button(item.filename.c_str(), ImVec2(64, 64));
		}

		if (isSelected)
			ImGui::PopStyleColor(2);

		// Handle click to select this palette item
		if (clicked)
		{
			this->manager->palette->clearPaletteItem();
			this->manager->palette->selectPaletteItem(itemIndex);
		}

		// Layout: itemsPerRow items per row
		if ((itemIndex + 1) % itemsPerRow != 0 && (itemIndex + 1) < (int)this->manager->palette->paletteItems.size())
			ImGui::SameLine();

		itemIndex++;
	}

	ImGui::PopStyleVar();
}

void Hud::imguiRenderPreferencesWindow()
{
	if (!this->showPreferencesWindow)
		return;

	ImGui::SetNextWindowSize(ImVec2(400, 450), ImGuiCond_FirstUseEver);
	ImGui::SetNextWindowPos(
		ImVec2(ImGui::GetIO().DisplaySize.x * 0.5f, ImGui::GetIO().DisplaySize.y * 0.5f),
		ImGuiCond_FirstUseEver,
		ImVec2(0.5f, 0.5f)
	);

	ImGui::Begin("Map Preferences", &this->showPreferencesWindow, ImGuiWindowFlags_NoCollapse);

	// Sync buffer values from map data
	strncpy(imguiMapName, this->manager->map->data.name.c_str(), sizeof(imguiMapName) - 1);
	strncpy(imguiMapMusic, this->manager->map->data.music.c_str(), sizeof(imguiMapMusic) - 1);
	strncpy(imguiMapVersion, this->manager->map->data.version.c_str(), sizeof(imguiMapVersion) - 1);
	strncpy(imguiWeatherName, this->manager->map->data.weatherName.c_str(), sizeof(imguiWeatherName) - 1);
	strncpy(imguiParticles, this->manager->map->data.particles.c_str(), sizeof(imguiParticles) - 1);

	// -- Map Identity section --
	ImGui::Separator();
	ImGui::Text("--- Map Identity ---");

	ImGui::Text("Name");
	ImGui::PushItemWidth(280);
	if (ImGui::InputText("##prefMapName", imguiMapName, sizeof(imguiMapName)))
		this->manager->map->data.name = imguiMapName;
	ImGui::PopItemWidth();

	ImGui::Text("Version");
	ImGui::PushItemWidth(120);
	if (ImGui::InputText("##prefMapVersion", imguiMapVersion, sizeof(imguiMapVersion)))
		this->manager->map->data.version = imguiMapVersion;
	ImGui::PopItemWidth();

	ImGui::Text("Music");
	ImGui::PushItemWidth(280);
	if (ImGui::InputText("##prefMapMusic", imguiMapMusic, sizeof(imguiMapMusic)))
		this->manager->map->data.music = imguiMapMusic;
	ImGui::PopItemWidth();

	// -- Map Dimensions section --
	ImGui::Separator();
	ImGui::Text("--- Map Dimensions ---");

	int mapSizeX = this->manager->map->data.size.x;
	int mapSizeY = this->manager->map->data.size.y;

	ImGui::Text("Width");
	ImGui::PushItemWidth(120);
	if (ImGui::InputInt("##prefMapSx", &mapSizeX, 0, 0))
		this->manager->map->data.size.x = std::max(0, std::min(99999, mapSizeX));
	ImGui::PopItemWidth();

	ImGui::Text("Height");
	ImGui::PushItemWidth(120);
	if (ImGui::InputInt("##prefMapSy", &mapSizeY, 0, 0))
		this->manager->map->data.size.y = std::max(0, std::min(99999, mapSizeY));
	ImGui::PopItemWidth();

	// -- Weather section --
	ImGui::Separator();
	ImGui::Text("--- Weather & Particles ---");

	int wChance = (int)this->manager->map->data.weatherChance;
	ImGui::Text("Weather Chance (%%)");
	ImGui::PushItemWidth(120);
	if (ImGui::InputInt("##prefWChance", &wChance, 0, 0))
		this->manager->map->data.weatherChance = std::max(0, std::min(100, wChance));
	ImGui::PopItemWidth();

	ImGui::Text("Weather Name");
	ImGui::PushItemWidth(280);
	if (ImGui::InputText("##prefWName", imguiWeatherName, sizeof(imguiWeatherName)))
		this->manager->map->data.weatherName = imguiWeatherName;
	ImGui::PopItemWidth();

	ImGui::Text("Particles");
	ImGui::PushItemWidth(280);
	if (ImGui::InputText("##prefParticles", imguiParticles, sizeof(imguiParticles)))
		this->manager->map->data.particles = imguiParticles;
	ImGui::PopItemWidth();

	ImGui::End();
}

void Hud::imguiRenderPaletteSelector() {}
void Hud::imguiRenderPropertiesPanel() {}
void Hud::imguiRenderEditsPanel() {}
