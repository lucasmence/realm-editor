#include <boost/lexical_cast.hpp>
#include <boost/algorithm/string.hpp>
#include <math.h>
#include "hud.hpp"
#include "../manager.hpp"
#include "../library/position.hpp"
#include "../library//script.hpp"

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
	this->hoverShapeSize = sf::Vector2f(0.f, 0.f);
	this->mousePressPosition = sf::Vector2f(0.f, 0.f);
	this->messageBox = MessageBox{ nullptr, nullptr };

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
	this->manager->removeView(std::static_pointer_cast<ViewElement>(this->shapeTooltip));
	this->manager->removeView(std::static_pointer_cast<ViewElement>(this->shapeMinimap));
	this->manager->removeView(std::static_pointer_cast<ViewElement>(this->labelTooltip));
	this->itemModelSelected = nullptr;
	this->unloadLists();
}

bool Hud::update(sf::Vector2f cursor)
{
	this->resetTooltip();
	this->updateCursor(cursor);
	this->updateLabels(cursor);
	this->updateButtonsColor(cursor);
	this->updateEditsColor(cursor);
	this->updateEditValues();
	this->updateHoverShapeSize();
	this->updateHoverMapSize();
	this->updateMousePressed(cursor);
	this->updateHoverGeneral();

	return true;
}

bool Hud::updateClick(sf::Vector2f cursor, bool rightButton)
{
	this->mousePressPosition = cursor;
	this->mousePressed = true;
	this->mouseRightButton = rightButton;
	this->selectedItemUpdate();
	this->updateItemSelectedMove(cursor);
	this->buttonsClick(cursor);
	this->matrixActivate(cursor);
	this->spawnClick(cursor);
	this->editsClick(cursor);
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

bool Hud::updateLabels(sf::Vector2f cursor)
{
	for (auto& label : this->labels)
		if (label->name == "lblCoordinates" && this->manager->hasFocus)
			label->text->setString("(" + boost::lexical_cast<std::string>((int)cursor.x) + ", " + boost::lexical_cast<std::string>((int)cursor.y) + ")");
		else if (label->name == "lblPalettePage")
			label->text->setString(boost::lexical_cast<std::string>(this->manager->palette->pageIndex + 1));
		else if (label->name == "lblPaletteItem")
			label->text->setString(this->manager->palette->selectedItem);
		else if (label->name == "lblPaletteStatus")
			this->updateLabelPaletteStatus(label->text);
		else if (label->name == "lblGridSize")
			label->text->setString(boost::lexical_cast<std::string>(this->gridSizeList.at(this->gridSize))+"px");
		else if (label->name == "lblBrushSize")
			label->text->setString(boost::lexical_cast<std::string>(this->brushSizeList.at(this->brushSize)) + "x");

	return true;
}

bool Hud::updateLabelPaletteStatus(std::shared_ptr<sf::Text> text)
{
	switch (this->manager->palette->status)
	{
		case (PaletteStatus::psInsert):
		{
			text->setString("I N S E R T I N G");
			text->setFillColor(sf::Color(0,255,0,255));
			break;
		}
		case (PaletteStatus::psDelete):
		{
			text->setString("D E L E T I N G");
			text->setFillColor(sf::Color(255, 0, 0, 255));
			break;
		}
		default: 
		{
			if (this->itemSelect)
				text->setString("S E L E C T");
			else
				text->setString("- - -");
			text->setFillColor(sf::Color(255, 255, 255, 255));
			break;
		}
	}
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

bool Hud::setTooltip(std::string hint, sf::Vector2f cursor)
{
	if (hint == "")
		return false;

	this->labelTooltip->text->setString(hint);
	this->labelTooltip->visible = true;
	this->shapeTooltip->visible = true;

	this->labelTooltip->setPosition(sf::Vector2f(cursor.x, cursor.y - 15.f));

	std::static_pointer_cast<sf::RectangleShape>(this->shapeTooltip->shape)->setSize(sf::Vector2f(this->labelTooltip->text->getGlobalBounds().width + 10.f, 
																								  this->labelTooltip->text->getGlobalBounds().height + 10.f));
	this->shapeTooltip->setPosition(sf::Vector2f(cursor.x - 5.f, cursor.y - 15.f));
	return true;
}

bool Hud::resetTooltip()
{
	this->labelTooltip->text->setString("");
	this->labelTooltip->visible = false;
	this->shapeTooltip->visible = false;
	return false;
}

bool Hud::help()
{
	this->showMessage("Accessing external page...");
	script::openUrl("https://boltcraft.github.io/realm-editor/");
	return true;
}

bool Hud::changeGridSize(int order)
{
	this->gridSize += order;

	if (this->gridSize < 0)
		this->gridSize = 0;
	else if (this->gridSize >= this->gridSizeList.size())
		this->gridSize = this->gridSizeList.size() - 1;

	return this->loadGrid();
}

bool Hud::changeBrushSize(int order)
{
	this->brushSize += order;

	if (this->brushSize < 0)
		this->brushSize = 0;
	else if (this->brushSize >= this->brushSizeList.size())
		this->brushSize = this->brushSizeList.size() - 1;

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
	std::static_pointer_cast<sf::RectangleShape>(this->shapeMinimap->shape)->setSize(sf::Vector2f(this->manager->minimapViewArea.width / 2.f, this->manager->minimapViewArea.height / 2.f));
	return true;
}

bool Hud::updateHoverShapeSize()
{
	std::static_pointer_cast<sf::RectangleShape>(this->shapeHover->shape)->setSize(sf::Vector2f(this->hoverShapeSize.x *
																							   (sqrt(this->brushSizeList.at(this->brushSize))),
																							   this->hoverShapeSize.y *
																							   (sqrt(this->brushSizeList.at(this->brushSize)))));
	this->shapeHover->shape->setOrigin(sf::Vector2f(0.f * this->shapeHover->shape->getGlobalBounds().width / 2.f,
                                                    0.f * this->shapeHover->shape->getGlobalBounds().height / 2.f));
	this->shapeHover->shape->setRotation(this->rotation);
	this->shapeHover->shape->setScale(this->scale, this->scale);
	return true;
}

bool Hud::toggleSpawnPress(std::shared_ptr<Button> button)
{
	this->spawnPress = !this->spawnPress;
	button->selected = this->spawnPress;
	return true;
}

bool Hud::toggleCenterShape(std::shared_ptr<Button> button)
{
	this->centerShape = !this->centerShape;
	button->selected = this->centerShape;
	return true;
}

bool Hud::toggleGridVisibility()
{
	for (auto& gridIndex : this->grid)
		gridIndex->visible = !gridIndex->visible;
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
				if (object.type == MapObjectType::motPortal)
					positionExtra.x = -object.model->getGlobalBounds().width;
				else
					positionExtra.x = 0.f;

			}
				
			if (positionLowerest.y > object.model->getPosition().y || positionLowerest.y == -1.f)
			{
				positionLowerest.y = object.model->getPosition().y;
				if (object.type == MapObjectType::motPortal)
					positionExtra.y = -object.model->getGlobalBounds().height;
				else
					positionExtra.y = 0.f;
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
				if (object.type == MapObjectType::motTerrain)
					positionExtra.x = object.model->getGlobalBounds().width;
				else
					positionExtra.x = 0.f;
			}
				
			if (positionHighest.y < object.model->getPosition().y)
			{
				positionHighest.y = object.model->getPosition().y;
				if (object.type == MapObjectType::motTerrain)
					positionExtra.y = object.model->getGlobalBounds().height;
				else
					positionExtra.y = 0.f;
			}
				
		}

	this->setEditValue("edtMapSizeX", boost::lexical_cast<std::string>(positionHighest.x + positionExtra.x));
	this->setEditValue("edtMapSizeY", boost::lexical_cast<std::string>(positionHighest.y + positionExtra.y));
	return true;
}

bool Hud::toggleMatrixTriggered(std::shared_ptr<Button> button)
{
	button->selected = !button->selected;
	this->matrixActivated = button->selected;
	return true;
}

bool Hud::toggleWallTriggered(std::shared_ptr<Button> button)
{
	button->selected = !button->selected;
	this->wallActivated = button->selected;
	return true;
}

bool Hud::toggleMapAreaSize(std::shared_ptr<Button> button)
{
	button->selected = !button->selected;
	this->shapeMapArea->visible = button->selected;
	return true;
}

bool Hud::toggleGridSpawn(std::shared_ptr<Button> button)
{
	button->selected = !button->selected;
	this->gridSpawn = !button->selected;
	return true;
}

bool Hud::toggleItemSelectedMove(std::shared_ptr<Button> button)
{
	if (!this->itemSelected)
		button->selected = false;
	else
		button->selected = !button->selected;
	this->itemSelectedMove = button->selected;
	return true;
}

bool Hud::toggleDragCursor(std::shared_ptr<Button> button)
{
	button->selected = !button->selected;
	this->dragCursor = button->selected;
	if (this->dragCursor)
		this->manager->palette->clearPaletteItem();
	return false;
}

bool Hud::formShapeClick(std::shared_ptr<Button> button)
{
	if (button->name == "btnFormShapeSquare")
	{
		button->selected = true;
		this->getButton("btnFormShapeCircle")->selected = false;
		this->manager->palette->formShape = PaletteFormShape::fsSquare;
	}

	if (button->name == "btnFormShapeCircle")
	{
		button->selected = true;
		this->getButton("btnFormShapeSquare")->selected = false;
		this->manager->palette->formShape = PaletteFormShape::fsCircle;
	}

	if (button->name == "btnFormShapeNone")
	{
		this->getButton("btnFormShapeCircle")->selected = false;
		this->getButton("btnFormShapeSquare")->selected = false;
		this->manager->palette->formShape = PaletteFormShape::fsNone;
	}

	return true;
}

bool Hud::removeBackground(std::shared_ptr<Button> button, const bool message)
{
	this->manager->map->data.textureBackground.model = nullptr;
	button->setVisible(false);
	std::shared_ptr<Label> label = this->manager->hud->getLabel("lblBackground");
	label->text->setString("");
	label->visible = false;
	if (message)
		this->manager->hud->showMessage("Terrain background removed!");
	return true;
}

bool Hud::enableItemSelect(std::shared_ptr<Button> button)
{
	this->manager->palette->clearPaletteItem();
	this->itemSelect = true;
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
		if (object.model == this->itemModelSelected)
		{
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
		if (object.model == this->itemModelSelected)
		{
			objectSelected = object;
			break;
		}

	if (!objectSelected.model)
		return false;

	this->manager->palette->clearPaletteItem();
	this->manager->map->removeObjectUnit(objectSelected);
	return true;
}

bool Hud::selectItem(sf::Vector2f cursor)
{
	if (!this->itemSelect || !this->checkMapClick(cursor))
		return false;

	MapObjectUnit objectSelected{ MapObjectType::motTerrain , sf::Vector2f(0.f, 0.f), 0.f, nullptr, {} };
	for (auto& object : this->manager->map->objects)
		if (object.model->getGlobalBounds().contains(cursor))
		{
			if (objectSelected.model != nullptr)
				if (objectSelected.model->priority < object.model->priority)
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
	std::static_pointer_cast<sf::RectangleShape>(this->shapeItemSelected->shape)->setSize(sf::Vector2f(objectSelected.model->getGlobalBounds().width, 
																									   objectSelected.model->getGlobalBounds().height));
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
		{
			if (field.valueBool.value)
				value = "true";
			else
				value = "false";
		}

		for (int index = 0; index < 7; index++)
			for (auto& edit : this->edits)
				if (edit->name == "edtExtraField-" + boost::lexical_cast<std::string>(index))
					if (edit->origin == field.field)
					{
						edit->setValue(value);
						break;
					}			
	}

	return false;
}

bool Hud::getPaletteType(PaletteType& paletteType, MapObjectType type)
{
	switch (type)
	{
		case (MapObjectType::motTerrain):
		{
			paletteType = PaletteType::ptTerrain;
			break;
		}
		case (MapObjectType::motProp):
		{
			paletteType = PaletteType::ptProp;
			break;
		}
		case (MapObjectType::motEnvironment):
		{
			paletteType = PaletteType::ptEnvironment;
			break;
		}
		case (MapObjectType::motUnit):
		{
			paletteType = PaletteType::ptUnit;
			break;
		}
		case (MapObjectType::motMerchant):
		{
			paletteType = PaletteType::ptMerchant;
			break;
		}
		case (MapObjectType::motItem):
		{
			paletteType = PaletteType::ptItem;
			break;
		}
		case (MapObjectType::motPortal):
		{
			paletteType = PaletteType::ptPortal;
			break;
		}
	}

	return true;
}

bool Hud::showMessage(std::string text, float time)
{
	this->messageBox.label->reset();
	this->messageBox.border->reset();
	this->messageBox.label->text->setString(text);
	this->messageBox.label->setPosition(sf::Vector2f(0.f, 900.f));
	this->messageBox.border->setPosition(sf::Vector2f(0.f, 905.f));
	std::static_pointer_cast<sf::RectangleShape>(this->messageBox.border->shape)->setSize(sf::Vector2f(this->messageBox.label->text->getGlobalBounds().width + 10.f,
																									   this->messageBox.label->text->getGlobalBounds().height + 10.f));
	this->messageBox.label->setPosition(position::getCenterPosition(sf::Vector2f(1920.f, 1080.f), this->messageBox.label->text->getGlobalBounds(), sf::Vector2i(1, 0)));
	this->messageBox.border->setPosition(position::getCenterPosition(sf::Vector2f(1920.f, 1080.f), this->messageBox.border->shape->getGlobalBounds(), sf::Vector2i(1, 0)));
	this->messageBox.label->timeMax = time;
	this->messageBox.border->timeMax = time;

	this->manager->update();

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

	if (initialPosition.x < 0.f)
	{
		initialPosition.x = cursor.x;
		finalPosition.x = this->shapeMatrix->shape->getPosition().x;
	}	
	else 
	{
		initialPosition.x = this->shapeMatrix->shape->getPosition().x;
		finalPosition.x = cursor.x;
	}
		
	if (initialPosition.y < 0.f)
	{
		initialPosition.y = cursor.y;
		finalPosition.y = this->shapeMatrix->shape->getPosition().y;
	}
	else
	{
		initialPosition.y = this->shapeMatrix->shape->getPosition().y;
		finalPosition.y = cursor.y;
	}

	sf::Vector2f realSize(finalPosition.x - initialPosition.x, finalPosition.y - initialPosition.y);

	if (this->wallActivated)
	{
		if (!this->checkMapClick(cursor))
			return false;

		this->matrixPosSpawn = false;

		PaletteType previousPaletteType = this->manager->palette->type;
		this->manager->palette->selectPalette(PaletteType::ptPortal);
		this->manager->palette->status = PaletteStatus::psInsert;
		this->manager->palette->selectedOrigin = "wall";
		this->manager->palette->selectedItem = "wall";
		this->shapeHover->shape->setPosition(initialPosition);
		this->shapeHover->visible = true;

		this->manager->hud->updateExtraEditsValue(
			{ "Width", "Height" },
			{ EditType::etInteger, EditType::etInteger },
			{ boost::lexical_cast<std::string>(int(this->shapeMatrix->shape->getGlobalBounds().width)), boost::lexical_cast<std::string>(int(this->shapeMatrix->shape->getGlobalBounds().height)) },
			{ 99999, 99999 },
			{ "width", "height" });

		this->setExtraEditsValue({ boost::lexical_cast<std::string>(int(this->shapeMatrix->shape->getGlobalBounds().width)),
								   boost::lexical_cast<std::string>(int(this->shapeMatrix->shape->getGlobalBounds().height)) });

		this->spawnClick(initialPosition);
		this->shapeHover->visible = false;
		this->matrixPosSpawn = true;

		return true;
	}

	sf::Vector2f textureSize(this->shapeHover->shape->getGlobalBounds().width / this->shapeHover->shape->getScale().x, 
							 this->shapeHover->shape->getGlobalBounds().height / this->shapeHover->shape->getScale().y);

	int lines = div(realSize.x, (int)textureSize.x).quot;
	int columns = div(realSize.y, (int)textureSize.y).quot;

	sf::Vector2f position = initialPosition;

	for (int x = 0; x < lines; x++)
	{
		position = sf::Vector2f(initialPosition.x + textureSize.x * x, initialPosition.y);
		this->spawnClick(position);
		for (int y = 0; y < columns; y++)
		{
			position = sf::Vector2f(initialPosition.x + textureSize.x * x, initialPosition.y + textureSize.y * y);
			this->shapeHover->shape->setPosition(position);
			this->spawnClick(position);
		}	
	}	

	this->matrixPosSpawn = true;
	
	return true;
}

std::list<MapObjectField> Hud::getExtraEditValuesByType()
{
	std::list<MapObjectField> fields = {};
	std::vector<EditValue> extraValues = this->getExtraEditsValue();

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
			else if (this->manager->palette->selectedOrigin == "trap")
			{
				fields.emplace_back(MapObjectField{ "width", MapObjectFieldString{"", false}, MapObjectFieldInt{ extraValues.at(0).integer, true } });
				fields.emplace_back(MapObjectField{ "height", MapObjectFieldString{"", false}, MapObjectFieldInt{ extraValues.at(1).integer, true } });	
				fields.emplace_back(MapObjectField{ "spell", MapObjectFieldString{ extraValues.at(2).string, true} });
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
			break;
		}
	}

	return fields;
}

bool Hud::spawnClick(sf::Vector2f cursor)
{
	if (this->matrixTriggered || this->itemSelectedMove)
		return false;

	if (this->matrixPosSpawn)
	{
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
			std::string paletteTypeField = "textures/";
			std::string texture = this->manager->palette->selectedItem;
			std::list<MapObjectField> fields = this->getExtraEditValuesByType();

			sf::Vector2f tilesetPosition(this->shapeHover->shape->getPosition());
			sf::Vector2f tilesetOrigin(this->shapeHover->shape->getOrigin());

			sf::Vector2f hoverCenter(this->shapeHover->shape->getPosition().x + this->shapeHover->shape->getGlobalBounds().width / 2.f,
									 this->shapeHover->shape->getPosition().y + this->shapeHover->shape->getGlobalBounds().height / 2.f);

			int priorityValue = this->priority;

			std::vector<EditValue> extraValues = this->getExtraEditsValue();

			switch (this->manager->palette->type)
			{
				case (PaletteType::ptTerrain) :
				{
					objectType = MapObjectType::motTerrain;
					paletteTypeField += "terrain";
					
					if (extraValues.at(1).boolean)
					{
						std::shared_ptr<Model> model = std::make_shared<Model>(this->manager, 
																			   tilesetPosition, 
																			   paletteTypeField + "/" + texture, priorityValue, false, "", this->manager->palette->selectedOrigin);
						model->sprite->setScale(this->scale, this->scale);
						model->setPosition(sf::Vector2f(model->getGlobalBounds().width,
														model->getGlobalBounds().height));
						this->manager->map->data.textureBackground = MapObjectUnit{ objectType, sf::Vector2f(model->getGlobalBounds().width, 
																											 model->getGlobalBounds().height), 0.f, model, fields };

						this->setExtraEditsValue({ "true", "false"});
						this->manager->palette->clearPaletteItem();
						this->getButton("btnRemoveBackground")->setVisible(true);
						std::shared_ptr<Label> label = this->manager->hud->getLabel("lblBackground");
						label->text->setString("terrain/" + texture);
						label->visible = true;

						this->manager->hud->showMessage("Terrain background added!");

						return true;
					}

					break;
				}
				case (PaletteType::ptProp):
				{
					objectType = MapObjectType::motProp;
					paletteTypeField += "prop";
					break;
				}
				case (PaletteType::ptEnvironment):
				{
					objectType = MapObjectType::motEnvironment;

					std::size_t found = texture.find("/");
					if (found == std::string::npos)
						paletteTypeField += "environment";
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

					std::shared_ptr<Model> model = std::make_shared<Model>(this->manager, 
																		   tilesetPosition, 
																		   "", priorityValue, false, "", this->manager->palette->selectedOrigin);

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
					else if (this->manager->palette->selectedOrigin == "trap")
						this->manager->palette->loadPaletteShape(model, this->manager->palette->selectedOrigin, sf::Vector2f(extraValues.at(0).integer, extraValues.at(1).integer));
					else if (this->manager->palette->selectedOrigin == "exit")
						this->manager->palette->loadPaletteShape(model, this->manager->palette->selectedOrigin, sf::Vector2f(extraValues.at(0).integer, extraValues.at(1).integer));
					else if (this->manager->palette->selectedOrigin == "guardian")
						this->manager->palette->loadPaletteShape(model, this->manager->palette->selectedOrigin, sf::Vector2f(extraValues.at(0).integer, extraValues.at(1).integer));

					model->setOrigin(tilesetOrigin);
					this->manager->addView(std::static_pointer_cast<ViewElement>(model));
					this->manager->map->addObjectUnit(MapObjectUnit{ objectType, model->getPosition(), 0.f, model, fields });

					this->manager->palette->clearPaletteItem();

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
					for (int x = 0; x < sqrt(this->brushSizeList.at(this->brushSize)); x++)
						for (int y = 0; y < sqrt(this->brushSizeList.at(this->brushSize)); y++)
							this->spawnItem(tilesetPosition, paletteTypeField, texture, priorityValue, tilesetOrigin, x, y, objectType, fields);

					break;
				}
				case (PaletteFormShape::fsSquare):
				{
					if (this->formShapeSize < 2)
						break;

					sf::Vector2f lastPosition(tilesetPosition.x - ((this->formShapeSize - 1) / 2.f) * this->hoverShapeSize.x, 
											  tilesetPosition.y + ((this->formShapeSize - 1) / 2.f) * this->hoverShapeSize.y);

					int xAxis[4] = {0, 1, 0, -1};
					int yAxis[4] = {-1, 0, 1, 0};

					for (int x = 0; x < 4; x++)
						for (int y = 0; y < this->formShapeSize - 1; y++)
						{
							std::shared_ptr<Model> model = this->spawnItem(lastPosition, paletteTypeField, texture, priorityValue, tilesetOrigin, 0, 0, objectType, fields);
							lastPosition = sf::Vector2f(lastPosition.x + (model->getGlobalBounds().width * xAxis[x]), lastPosition.y + (model->getGlobalBounds().height * yAxis[x]));
						}			

					break;
				}
				case (PaletteFormShape::fsCircle):
				{
					if (this->formShapeSize < 2)
						break;

					sf::Vector2f lastPosition(tilesetPosition.x - (this->formShapeSize * 1.33f) * this->hoverShapeSize.y,
											  tilesetPosition.y + ((this->formShapeSize - 1) / 2.f) * this->hoverShapeSize.y);

					int xAxis[8] = {0, 1, 1, 1, 0, -1, -1, -1};
					int yAxis[8] = {-1, -1, 0, 1, 1, 1, 0, -1};

					for (int x = 0; x < 8; x++)
						for (int y = 0; y < this->formShapeSize - 1; y++)
						{
							std::shared_ptr<Model> model = this->spawnItem(lastPosition, paletteTypeField, texture, priorityValue, tilesetOrigin, 0, 0, objectType, fields);
							lastPosition = sf::Vector2f(lastPosition.x + (model->getGlobalBounds().width * xAxis[x]), lastPosition.y + (model->getGlobalBounds().height * yAxis[x]));
						}			

					break;
				}
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
				if (object.model->getGlobalBounds().contains(cursor))
				{
					if (objectSelected.model != nullptr)
						if (objectSelected.model->priority < object.model->priority)
							continue;

					objectSelected = object;
				}	

			if (objectSelected.model)
				this->manager->map->removeObjectUnit(objectSelected);

			break;
		}

		default: return false;
	}

	return true;
}

std::shared_ptr<Model> Hud::spawnItem(sf::Vector2f tilesetPosition, std::string paletteTypeField, std::string texture, int priorityValue,
									  sf::Vector2f tilesetOrigin, int x, int y, MapObjectType objectType, std::list<MapObjectField> fields)
{
	std::shared_ptr<Model> model = std::make_shared<Model>(this->manager, 
														   tilesetPosition, 
														   paletteTypeField + "/" + texture, priorityValue, false, "", this->manager->palette->selectedOrigin);
	
	model->autoPriority = this->manager->map->getObjectAutoPriority(objectType);
	model->sprite->setOrigin(tilesetOrigin);
	model->sprite->move(model->sprite->getGlobalBounds().width * x, model->sprite->getGlobalBounds().height * y);
	model->sprite->setRotation(this->rotation);
	model->sprite->setScale(this->scale, this->scale);
	
	this->manager->addView(std::static_pointer_cast<ViewElement>(model));
	this->manager->map->addObjectUnit(MapObjectUnit{ objectType, model->sprite->getPosition(), 0.f, model, fields });

	return model;
}

bool Hud::buttonsClick(sf::Vector2f cursor)
{
	for (auto& button : this->buttons)
		if (button->shape->shape->getGlobalBounds().contains(cursor) && button->getVisible())
		{
			if (button->name == "btnPalettePrevious")
			{
				this->manager->palette->pageIndex--;
				this->manager->palette->selectPalette(this->manager->palette->type);
			}
			else if (button->name == "btnPaletteNext")
			{
				this->manager->palette->pageIndex++;
				this->manager->palette->selectPalette(this->manager->palette->type);
			}
			else if (button->name == "btnClear")
				this->manager->palette->clearPaletteItem();
			else if (button->name == "btnErase")
				this->manager->palette->erasePaletteItem();
			else if (button->name == "btnSave")
				this->manager->map->saveMap();
			else if (button->name == "btnSaveAs")
				this->manager->map->saveMapAs();
			else if (button->name == "btnOpen")
				this->manager->map->loadMap();
			else if (button->name == "btnNew")
				this->manager->map->newMap();
			else if (button->name == "btnReload")
				this->manager->map->reloadMap();
			else if (button->name == "btnReloadConfig")
				this->manager->loadConstants();
			else if (button->name == "btnTrigger")
				this->manager->map->createTriggerFile();
			else if (button->name == "btnHelp")
				this->help();
			else if (button->name == "btnGridSizeIncrease")
				this->changeGridSize(1);
			else if (button->name == "btnGridSizeDecrease")
				this->changeGridSize(-1);
			else if (button->name == "btnGridVisibilityToggle")
				this->toggleGridVisibility();
			else if (button->name == "btnBrushSizeIncrease")
				this->changeBrushSize(1);
			else if (button->name == "btnBrushSizeDecrease")
				this->changeBrushSize(-1);
			else if (button->name == "btnSpawnPress")
				this->toggleSpawnPress(button);
			else if (button->name == "btnCenterShape")
				this->toggleCenterShape(button);
			else if (button->name == "btnMapAreaSize")
				this->toggleMapAreaSize(button);
			else if (button->name == "btnMatrix")
				this->toggleMatrixTriggered(button);
			else if (button->name == "btnWall")
				this->toggleWallTriggered(button);
			else if (button->name == "btnGridSpawn")
				this->toggleGridSpawn(button);
			else if (button->name == "btnDragCursor")
				this->toggleDragCursor(button);
			else if (button->name == "btnSelectItem")
				this->enableItemSelect(button);
			else if (button->name == "btnSelectItemMove")
				this->toggleItemSelectedMove(button);
			else if (button->name == "btnUpdateMapBounds")
				this->updateMapBounds();
			else if (button->name == "btnFormShapeSquare" || button->name == "btnFormShapeCircle" || button->name == "btnFormShapeNone")
				this->formShapeClick(button);
			else if (button->name == "btnTerrain")
				this->manager->palette->selectPalette(PaletteType::ptTerrain);
			else if (button->name == "btnProp")
				this->manager->palette->selectPalette(PaletteType::ptProp);
			else if (button->name == "btnEnvironment")
				this->manager->palette->selectPalette(PaletteType::ptEnvironment);
			else if (button->name == "btnUnit")
				this->manager->palette->selectPalette(PaletteType::ptUnit);
			else if (button->name == "btnMerchant")
				this->manager->palette->selectPalette(PaletteType::ptMerchant);
			else if (button->name == "btnPortal")
				this->manager->palette->selectPalette(PaletteType::ptPortal);
			else if (button->name == "btnItem")
				this->manager->palette->selectPalette(PaletteType::ptItem);
			else if (button->name == "btnRemoveBackground")
				this->removeBackground(button);

			return true;
			break;
		}
	return false;
}

bool Hud::editsClick(sf::Vector2f cursor)
{
	bool selected = true;
	for (auto& edit : this->edits)
		if (edit->shape->shape->getGlobalBounds().contains(cursor))
		{
			edit->selected = true;
			selected = true;
		}

	if (selected)
		for (auto& edit : this->edits)
			if (!edit->shape->shape->getGlobalBounds().contains(cursor))
				edit->selected = false;

	return true;
}

bool Hud::updateButtonsColor(sf::Vector2f cursor)
{
	for (auto& button : this->buttons)
		if (button->shape->shape->getGlobalBounds().contains(cursor) && button->getVisible())
		{
			button->shape->shape->setFillColor(sf::Color(200, 200, 50, 100));
			button->label->text->setFillColor(sf::Color(200, 200, 50, 255));

			this->setTooltip(button->hint, cursor);
			
		}
		else if (button->selected)
		{
			button->shape->shape->setFillColor(sf::Color(150, 200, 200, 100));
			button->label->text->setFillColor(sf::Color(100, 255, 255, 255));
		}
		else
		{
			button->shape->shape->setFillColor(sf::Color(150, 150, 150, 100));
			button->label->text->setFillColor(sf::Color(255, 255, 255, 255));
		}

	return true;
}

bool Hud::updateEditsColor(sf::Vector2f cursor)
{
	for (auto& edit : this->edits)
		if (edit->shape->shape->getGlobalBounds().contains(cursor) && edit->shape->visible)
		{
			edit->shape->shape->setFillColor(sf::Color(50, 200, 50, 100));
			edit->label->text->setFillColor(sf::Color(50, 200, 50, 255));

			this->setTooltip(edit->hint, cursor);
		}
		else if (edit->selected)
		{
			edit->shape->shape->setFillColor(sf::Color(150, 200, 200, 100));
			edit->label->text->setFillColor(sf::Color(100, 255, 255, 255));
		}
		else
		{
			edit->shape->shape->setFillColor(sf::Color(150, 150, 150, 100));
			edit->label->text->setFillColor(sf::Color(255, 255, 255, 255));
		}

	return true;
}

bool Hud::setEditValue(std::string editName, std::string value)
{
	if (editName == "")
		return false;

	for (auto& edit : this->edits)
		if (edit->name == editName)
		{
			edit->setValue(value);
			return true;
		}

	return false;
}

bool Hud::setExtraEditsValue(std::vector<std::string> value)
{
	for (int index = 0; index < value.size(); index++)
		for (auto& edit : this->edits)
			if (edit->name == "edtExtraField-" + boost::lexical_cast<std::string>(index))
				edit->setValue(value.at(index));
	return true;
}

bool Hud::setExtraEditValue(std::string value, int index)
{
	for (auto& edit : this->edits)
		if (edit->name == "edtExtraField-" + boost::lexical_cast<std::string>(index))
			edit->setValue(value);
	return true;
}

bool Hud::resetExtraEditsValue()
{
	if (!this->manager->palette)
		return false;

	switch (this->manager->palette->type)
	{
		case (PaletteType::ptTerrain):
		{
			this->setExtraEditsValue({ "true", "false" });
			break;
		}
		case (PaletteType::ptProp):
		{
			this->setExtraEditsValue({ "" });
			break;
		}
		case (PaletteType::ptEnvironment):
		{
			this->setExtraEditValue("", 1);
			break;
		}
		case (PaletteType::ptUnit):
		{
			this->setExtraEditsValue({ "0", "false", "enemy", "", "" });
			break;
		}
	}

	return true;
}

bool Hud::updateExtraEditsValue(std::vector<std::string> caption, std::vector<EditType> type, std::vector<std::string> value, std::vector<int> maxValue, std::vector<std::string> origin)
{
	for (int index = 0; index < 7; index++)
	{
		for (auto& label : this->labels)
			if (label->name == "lblExtraField-" + boost::lexical_cast<std::string>(index))
				label->visible = false;

		for (auto& edit : this->edits)
			if (edit->name == "edtExtraField-" + boost::lexical_cast<std::string>(index))
				edit->setVisible(false);
	}

	if (caption.size() <= 0)
		return false;

	sf::FloatRect extraFieldRegion = sf::FloatRect(0.f, 0.f, 0.f, 0.f);
	sf::Vector2f extraSpace(45.f, 0.f);
	sf::Vector2i extraSide(1, 0);

	for (int index = 0; index < caption.size(); index++)
	{
		for (auto& label : this->labels)
			if (label->name == "lblExtraField-" + boost::lexical_cast<std::string>(index))
			{
				label->text->setString(caption.at(index));
				label->visible = true;

				if (index > 0)
					label->setPosition(position::getSidePosition(extraFieldRegion,
																 sf::FloatRect(extraSpace.x, extraSpace.y,
																			   label->text->getGlobalBounds().width,
																			   label->text->getGlobalBounds().height),
																 extraSpace, extraSide));

				extraFieldRegion = sf::FloatRect(label->position.x, label->position.y, label->text->getGlobalBounds().width, label->text->getGlobalBounds().height);

				for (auto& edit : this->edits)
					if (edit->name == "edtExtraField-" + boost::lexical_cast<std::string>(index))
					{
						edit->type = type.at(index);
						edit->setValue(value.at(index));
						edit->origin = origin.at(index);

						int extraWidth = 0.f;

						switch (edit->type)
						{
							case (EditType::etString):
							{
								edit->maxLength = maxValue.at(index);
								extraWidth = edit->maxLength;
								break;
							}
							case (EditType::etInteger):
							{
								edit->integerMaxValue = maxValue.at(index);
								extraWidth = boost::lexical_cast<std::string>(edit->integerMaxValue).size();
								break;
							}
							case (EditType::etBoolean):
							{
								edit->maxLength = 5;
								edit->integerMaxValue = 1;
								extraWidth = edit->maxLength;
								break;
							}
						}

						edit->setVisible(true);

						extraSpace = sf::Vector2f(0.f, 0.f);

						sf::Vector2f positionSide = position::getSidePosition(extraFieldRegion, sf::FloatRect(extraSpace.x, extraSpace.y, 
																											  edit->shape->shape->getGlobalBounds().width, 
																											  edit->shape->shape->getGlobalBounds().height), 
																			  extraSpace, extraSide);

						edit->shape->setPosition(positionSide);
						edit->label->setPosition(sf::Vector2f(positionSide.x + 5.f, positionSide.y));

						extraFieldRegion = sf::FloatRect(edit->shape->position.x, label->position.y,
														 edit->shape->shape->getGlobalBounds().width, edit->shape->shape->getGlobalBounds().height);
						extraSpace = sf::Vector2f(0.f + (extraWidth * 8.f), 0.f);

						break;
					}

				break;
			}
	}

	return true;
}
std::vector<EditValue> Hud::getExtraEditsValue()
{
	std::vector<EditValue> values = {};

	for (int index = 0; index < 7; index++)
		for (auto& edit : this->edits)
			if (edit->name == "edtExtraField-" + boost::lexical_cast<std::string>(index) && edit->label->visible)
				values.emplace_back(edit->getValue());

	return values;
}

std::shared_ptr<Button> Hud::getButton(std::string name)
{
	for (auto& button : this->buttons)
		if (button->name == name)
			return button;

	return nullptr;
}

std::shared_ptr<Label> Hud::getLabel(std::string name)
{
	for (auto& label : this->labels)
		if (label->name == name)
			return label;

	return nullptr;
}

bool Hud::updateEditValues()
{
	for (auto& edit : this->edits)
		if (edit->name == "edtRotation")
			this->rotation = edit->getValue().integer;
		else if (edit->name == "edtFormShapeSize")
			this->formShapeSize = edit->getValue().integer;
		else if (edit->name == "edtScale")
			this->scale = edit->getValue().integer / 100.f;
		else if (edit->name == "edtPriority")
			this->priority = edit->getValue().integer;
		else if (edit->name == "edtMapSizeX")
			this->manager->map->data.size.x = edit->getValue().integer;
		else if (edit->name == "edtMapSizeY")
			this->manager->map->data.size.y = edit->getValue().integer;
		else if (edit->name == "edtMapName")
			this->manager->map->data.name = edit->getValue().string;
		else if (edit->name == "edtMapMusic")
			this->manager->map->data.music = edit->getValue().string;
		else if (edit->name == "edtMapVersion")
			this->manager->map->data.version = edit->getValue().string;
		else if (edit->name == "edtWeatherChance")
			this->manager->map->data.weatherChance = edit->getValue().integer;
		else if (edit->name == "edtWeatherName")
			this->manager->map->data.weatherName = edit->getValue().string;
		else if (edit->name == "edtParticles")
			this->manager->map->data.particles = edit->getValue().string;
	return true;
}

bool Hud::updateEdit(char text)
{
	for (auto& edit : this->edits)
		if (edit->selected)
		{
			std::string value = "";
			value += text;
			if (value == "\b" && edit->getValue().string.length() > 0)
			{
				edit->value.string.pop_back();
				edit->setValue(edit->getValue().string);
			}	
			else
				edit->setValue(edit->getValue().string+text);

			return true;
		}
	return false;
}

bool Hud::getCheckEditing()
{
	for (auto& edit : this->edits)
		if (edit->selected)
			return true;
		
	return false;
}

bool Hud::unloadLists()
{
	for (auto& button : this->buttons)
		button->clear();
	this->buttons.clear();
	for (auto& edit : this->edits)
		edit->clear();
	this->edits.clear();
	for (auto& label : this->labels)
		this->manager->removeView(std::static_pointer_cast<ViewElement>(label));
	this->labels.clear();
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

	sf::Vector2f distance(this->gridSizeList.at(this->gridSize), this->gridSizeList.at(this->gridSize));

	float gridSizeValue = this->gridSizeList.at(this->gridSize) / 32.f;

	for (int x = 0; x < (int(40 / gridSizeValue)); x++)
	{
		std::shared_ptr<Model> line = std::make_shared<Model>(this->manager, sf::Vector2f(0.f, x * distance.y), "", 3);

		line->loadShape(sf::Vector2f(2000.f, 1.f), sf::Color(0, 255, 0, 100));
		this->manager->addView(std::static_pointer_cast<ViewElement>(line));
		this->grid.emplace_back(line);
	}

	for (int y = 0; y < (int(64 / gridSizeValue)); y++)
	{
		std::shared_ptr<Model> line = std::make_shared<Model>(this->manager, sf::Vector2f(y * distance.x, 0.f), "", 3);

		line->loadShape(sf::Vector2f(1.f, 1220.f), sf::Color(0, 255, 0, 100));
		this->manager->addView(std::static_pointer_cast<ViewElement>(line));
		this->grid.emplace_back(line);
	}

	return true;
}

bool Hud::loadModels()
{
	std::shared_ptr<Model> header = std::make_shared<Model>(this->manager, sf::Vector2f(0.f, -100.f));
	std::shared_ptr<Model> palette = std::make_shared<Model>(this->manager, sf::Vector2f(1620.f, 100.f));
	std::shared_ptr<Model> messageBox = std::make_shared<Model>(this->manager, sf::Vector2f(0.f, 900.f));

	header->loadShape(sf::Vector2f(2020.f, 200.f), sf::Color(100, 100, 100, 255));
	palette->loadShape(sf::Vector2f(400.f, 1000.f), sf::Color(100, 100, 100, 255));
	messageBox->loadShape(sf::Vector2f(30.f, 1.f), sf::Color(100, 100, 100, 255));

	this->manager->addView(std::static_pointer_cast<ViewElement>(header));
	this->manager->addView(std::static_pointer_cast<ViewElement>(palette));
	this->manager->addView(std::static_pointer_cast<ViewElement>(messageBox));

	this->models.emplace_back(header);
	this->models.emplace_back(palette);
	this->models.emplace_back(messageBox);

	messageBox->visible = false;
	this->messageBox.border = messageBox;

	this->shapeMinimap = std::make_shared<Model>(this->manager, sf::Vector2f(this->manager->constant.minimapSize.left * this->manager->window->getSize().x,
																			 this->manager->constant.minimapSize.top * this->manager->window->getSize().y - 
																			 (this->manager->constant.minimapSize.top * 60.f)), "", 0, true);
	this->shapeMinimap->loadShape(sf::Vector2f(1.f, 1.f), sf::Color(0, 0, 0, 255));
	this->manager->addView(std::static_pointer_cast<ViewElement>(this->shapeMinimap));

	return true;
}
bool Hud::loadButtons()
{
	std::shared_ptr<Button> btnNew = std::make_shared<Button>(this->manager, "[New]", sf::Vector2f(15.f, 10.f), "btnNew", 20, nullptr, sf::Vector2i(0, 0), "Create a new map");
	std::shared_ptr<Button> btnOpen = std::make_shared<Button>(this->manager, "[Open]", sf::Vector2f(0.f, 0.f), "btnOpen", 20, btnNew, sf::Vector2i(1, 0), "Select a file to open an existing map");
	std::shared_ptr<Button> btnReload = std::make_shared<Button>(this->manager, "[Reload]", sf::Vector2f(0.f, 0.f), "btnReload", 20, btnOpen, sf::Vector2i(1, 0), "Reload the current loaded map");
	std::shared_ptr<Button> btnSave = std::make_shared<Button>(this->manager, "[Save]", sf::Vector2f(0.f, 0.f), "btnSave", 20, btnReload, sf::Vector2i(1, 0), "Save the current map");
	std::shared_ptr<Button> btnSaveAs = std::make_shared<Button>(this->manager, "[Save As]", sf::Vector2f(0.f, 0.f), "btnSaveAs", 20, btnSave, sf::Vector2i(1, 0), "Save the current map always verifying the path");
	std::shared_ptr<Button> btnReloadConfig = std::make_shared<Button>(this->manager, "[Reload Config]", sf::Vector2f(0.f, 0.f), "btnReloadConfig", 20, btnSaveAs, sf::Vector2i(1, 0), "Reload editor configuration file");
	std::shared_ptr<Button> btnTrigger = std::make_shared<Button>(this->manager, "[Trigger]", sf::Vector2f(0.f, 0.f), "btnTrigger", 20, btnReloadConfig, sf::Vector2i(1, 0), "Auto-create the trigger file");
	std::shared_ptr<Button> btnHelp = std::make_shared<Button>(this->manager, "[Help]", sf::Vector2f(0.f, 0.f), "btnHelp", 20, btnTrigger, sf::Vector2i(1, 0), "Open the help section (external)");

	std::shared_ptr<Button> btnClear = std::make_shared<Button>(this->manager, "[C]", sf::Vector2f(1275.f, 65.f), "btnClear", 18, nullptr, sf::Vector2i(0, 0), "Clear the selection");
	std::shared_ptr<Button> btnErase = std::make_shared<Button>(this->manager, "[E]", sf::Vector2f(0.f, 0.f), "btnErase", 18, btnClear, sf::Vector2i(-1, 0), "Enable erase-on-click mode");
	std::shared_ptr<Button> btnGridVisibilityToggle = std::make_shared<Button>(this->manager, "[G]", sf::Vector2f(0.f, 0.f), "btnGridVisibilityToggle", 18, btnClear, sf::Vector2i(1, 0), "Toggle grid visible ON/OFF");
	std::shared_ptr<Button> btnSpawnPress = std::make_shared<Button>(this->manager, "[P]", sf::Vector2f(0.f, 0.f), "btnSpawnPress", 18, btnErase, sf::Vector2i(-1, 0), "Toggle spawn-press mode");
	std::shared_ptr<Button> btnCenterShape = std::make_shared<Button>(this->manager, "[S]", sf::Vector2f(0.f, 0.f), "btnCenterShape", 18, btnSpawnPress, sf::Vector2i(-1, 0), "Makes the spawn shape at center of cursor");
	std::shared_ptr<Button> btnMatrix = std::make_shared<Button>(this->manager, "[M]", sf::Vector2f(0.f, 0.f), "btnMatrix", 18, btnCenterShape, sf::Vector2i(-1, 0), "Toggle matrix spawn mode");
	std::shared_ptr<Button> btnDragCursor = std::make_shared<Button>(this->manager, "[^]", sf::Vector2f(0.f, 0.f), "btnDragCursor", 18, btnMatrix, sf::Vector2i(-1, 0), "Toggle drag cursor to move the map");
	std::shared_ptr<Button> btnWall = std::make_shared<Button>(this->manager, "[W]", sf::Vector2f(0.f, 0.f), "btnWall", 18, btnDragCursor, sf::Vector2i(-1, 0), "Toggle generate wall with matrix spawn");
	std::shared_ptr<Button> btnMapAreaSize = std::make_shared<Button>(this->manager, "[A]", sf::Vector2f(0.f, 0.f), "btnMapAreaSize", 18, btnGridVisibilityToggle, sf::Vector2i(1, 0), "Toggle map area visible ON/OFF");
	std::shared_ptr<Button> btnGridSpawn = std::make_shared<Button>(this->manager, "[D]", sf::Vector2f(0.f, 0.f), "btnGridSpawn", 18, btnMapAreaSize, sf::Vector2i(1, 0), "Toggle disable grid-spawn ON/OFF");
	std::shared_ptr<Button> btnSelectItem = std::make_shared<Button>(this->manager, "[I]", sf::Vector2f(0.f, 0.f), "btnSelectItem", 18, btnGridSpawn, sf::Vector2i(1, 0), "Enable the item selection");
	std::shared_ptr<Button> btnSelectItemMove = std::make_shared<Button>(this->manager, "[->]", sf::Vector2f(0.f, 0.f), "btnSelectItemMove", 18, btnSelectItem, sf::Vector2i(1, 0), "Toggle item selected move");
	std::shared_ptr<Button> btnUpdateMapBounds = std::make_shared<Button>(this->manager, "[B]", sf::Vector2f(0.f, 0.f), "btnUpdateMapBounds", 18, btnSelectItemMove, sf::Vector2i(1, 0), "Update map bounds");
	std::shared_ptr<Button> btnFormShapeSquare = std::make_shared<Button>(this->manager, "[Q]", sf::Vector2f(0.f, 0.f), "btnFormShapeSquare", 18, btnUpdateMapBounds, sf::Vector2i(1, 0), "Set form shape to square");
	std::shared_ptr<Button> btnFormShapeCircle = std::make_shared<Button>(this->manager, "[R]", sf::Vector2f(0.f, 0.f), "btnFormShapeCircle", 18, btnFormShapeSquare, sf::Vector2i(1, 0), "Set form shape to circle");
	std::shared_ptr<Button> btnFormShapeNone = std::make_shared<Button>(this->manager, "[O]", sf::Vector2f(0.f, 0.f), "btnFormShapeNone", 18, btnFormShapeCircle, sf::Vector2i(1, 0), "Set form shape to none");
	
	std::shared_ptr<Button> btnUnit = std::make_shared<Button>(this->manager, "[Units]", sf::Vector2f(1650.f, 200.f), "btnUnit", 20, nullptr, sf::Vector2i(0, 0));
	std::shared_ptr<Button> btnMerchant = std::make_shared<Button>(this->manager, "[Merchants]", sf::Vector2f(0.f, 0.f), "btnMerchant", 20, btnUnit, sf::Vector2i(1, 0));
	std::shared_ptr<Button> btnProp = std::make_shared<Button>(this->manager, "[Props]", sf::Vector2f(0.f, 0.f), "btnProp", 20, btnUnit, sf::Vector2i(0, 1));
	std::shared_ptr<Button> btnEnvironment = std::make_shared<Button>(this->manager, "[Environments]", sf::Vector2f(0.f, 0.f), "btnEnvironment", 20, btnProp, sf::Vector2i(1, 0));
	std::shared_ptr<Button> btnTerrain = std::make_shared<Button>(this->manager, "[Terrain]", sf::Vector2f(0.f, 0.f), "btnTerrain", 18, btnProp, sf::Vector2i(0, 1));
	std::shared_ptr<Button> btnPortal = std::make_shared<Button>(this->manager, "[Portals]", sf::Vector2f(0.f, 0.f), "btnPortal", 18, btnTerrain, sf::Vector2i(1, 0));
	std::shared_ptr<Button> btnItem = std::make_shared<Button>(this->manager, "[Item]", sf::Vector2f(0.f, 0.f), "btnItem", 18, btnPortal, sf::Vector2i(1, 0));

	std::shared_ptr<Button> btnRemoveBackground = std::make_shared<Button>(this->manager, "[Remove BG]", sf::Vector2f(0.f, 70.f), "btnRemoveBackground", 15, btnUnit, sf::Vector2i(0, -1), "Remove the \ncurrent terrain \nbackground");
	btnRemoveBackground->setVisible(false);

	std::shared_ptr<Label> lblBackground = std::make_shared<Label>(this->manager, "-", 15, sf::Vector2f(0.f, 0.f), 1, sf::Color(255, 255, 255, 255), "lblBackground");
	lblBackground->setPosition(position::getSidePosition(btnRemoveBackground->shape->shape->getGlobalBounds(),
														 lblBackground->text->getGlobalBounds(),
														 lblBackground->text->getPosition(), sf::Vector2i(1, 0)));
	lblBackground->visible = false;
	this->manager->addView(std::static_pointer_cast<ViewElement>(lblBackground));
	this->labels.emplace_back(lblBackground);

	std::shared_ptr<Label> lblWeatherChance = std::make_shared<Label>(this->manager, "Weather Chance: ", 15, sf::Vector2f(0.f, 22.f), 1, sf::Color(255, 255, 255, 255), "lblWeatherChance");
	lblWeatherChance->setPosition(position::getSidePosition(btnRemoveBackground->shape->shape->getGlobalBounds(),
														    lblWeatherChance->text->getGlobalBounds(),
														    lblWeatherChance->text->getPosition(), sf::Vector2i(0, 1)));
	this->manager->addView(std::static_pointer_cast<ViewElement>(lblWeatherChance));
	this->labels.emplace_back(lblWeatherChance);

	std::shared_ptr<Edit> edtWeatherChance = std::make_shared<Edit>(this->manager, EditType::etInteger, "100", sf::Vector2f(0.f, 0.f), "edtWeatherChance", 15,
															  lblWeatherChance->text->getGlobalBounds(), sf::Vector2i(1, 0));
	edtWeatherChance->setValue("100");
	edtWeatherChance->integerMaxValue = 100;

	std::shared_ptr<Label> lblWeatherName = std::make_shared<Label>(this->manager, "Weather Name: ", 15, sf::Vector2f(0.f, 3.f), 1, sf::Color(255, 255, 255, 255), "lblWeatherName");
	lblWeatherName->setPosition(position::getSidePosition(lblWeatherChance->text->getGlobalBounds(),
														  lblWeatherName->text->getGlobalBounds(),
														  lblWeatherName->text->getPosition(), sf::Vector2i(0, 1)));
	this->manager->addView(std::static_pointer_cast<ViewElement>(lblWeatherName));
	this->labels.emplace_back(lblWeatherName);

	std::shared_ptr<Edit> edtWeatherName = std::make_shared<Edit>(this->manager, EditType::etString, "", sf::Vector2f(0.f, 0.f), "edtWeatherName", 15,
																  lblWeatherName->text->getGlobalBounds(), sf::Vector2i(1, 0));
	edtWeatherName->setValue("");
	edtWeatherName->maxLength = 64;

	std::shared_ptr<Label> lblParticles = std::make_shared<Label>(this->manager, "Particles: ", 15, sf::Vector2f(0.f, 9.f), 1, sf::Color(255, 255, 255, 255), "lblParticlesName");
	lblParticles->setPosition(position::getSidePosition(lblWeatherChance->text->getGlobalBounds(),
														  lblParticles->text->getGlobalBounds(),
														  lblParticles->text->getPosition(), sf::Vector2i(0, -1)));
	this->manager->addView(std::static_pointer_cast<ViewElement>(lblParticles));
	this->labels.emplace_back(lblParticles);

	std::shared_ptr<Edit> edtParticles = std::make_shared<Edit>(this->manager, EditType::etString, "", sf::Vector2f(0.f, -3.f), "edtParticles", 15,
																  lblParticles->text->getGlobalBounds(), sf::Vector2i(1, 0));
	edtParticles->setValue("woods");
	edtParticles->maxLength = 64;

	std::shared_ptr<Button> btnPalettePrevious = std::make_shared<Button>(this->manager, "[<]", sf::Vector2f(0.f, 15.f), "btnPalettePrevious", 20, btnTerrain, sf::Vector2i(0, 1), "Previous page");
	std::shared_ptr<Button> btnPaletteBack = std::make_shared<Button>(this->manager, "[>]", sf::Vector2f(175.f, 0.f), "btnPaletteNext", 20, btnPalettePrevious, sf::Vector2i(1, 0), "Next page");
	std::shared_ptr<Label> lblPalettePage = std::make_shared<Label>(this->manager, "1", 20, sf::Vector2f(85.f, 0.f), 1, sf::Color(255, 255, 255, 255), "lblPalettePage");
	lblPalettePage->setPosition(position::getSidePosition(btnPalettePrevious->shape->shape->getGlobalBounds(), 
														  lblPalettePage->text->getGlobalBounds(), 
														  lblPalettePage->text->getPosition(), sf::Vector2i(1, 0)));
	this->manager->addView(std::static_pointer_cast<ViewElement>(lblPalettePage));
	this->labels.emplace_back(lblPalettePage);

	std::shared_ptr<Button> btnGridSizeDecrease = std::make_shared<Button>(this->manager, "[<]", sf::Vector2f(0.f, 700.f), "btnGridSizeDecrease", 20, btnTerrain, sf::Vector2i(0, 1), "Decrease \ngrid size");
	std::shared_ptr<Button> btnGridSizeIncrease = std::make_shared<Button>(this->manager, "[>]", sf::Vector2f(175.f, 0.f), "btnGridSizeIncrease", 20, btnGridSizeDecrease, sf::Vector2i(1, 0), "Increase \ngrid size");
	std::shared_ptr<Label> lblGridSize = std::make_shared<Label>(this->manager, "64px", 20, sf::Vector2f(65.f, 0.f), 1, sf::Color(255, 255, 255, 255), "lblGridSize");
	lblGridSize->setPosition(position::getSidePosition(btnGridSizeDecrease->shape->shape->getGlobalBounds(),
													   lblGridSize->text->getGlobalBounds(),
													   lblGridSize->text->getPosition(), sf::Vector2i(1, 0)));
	this->manager->addView(std::static_pointer_cast<ViewElement>(lblGridSize));
	this->labels.emplace_back(lblGridSize);

	std::shared_ptr<Button> btnBrushSizeDecrease = std::make_shared<Button>(this->manager, "[<]", sf::Vector2f(0.f, -50.f), "btnBrushSizeDecrease", 20, btnGridSizeDecrease, sf::Vector2i(0, 1), "Decrease \nbrush size");
	std::shared_ptr<Button> btnBrushSizeIncrease = std::make_shared<Button>(this->manager, "[>]", sf::Vector2f(175.f, 0.f), "btnBrushSizeIncrease", 20, btnBrushSizeDecrease, sf::Vector2i(1, 0), "Increase \nbrush size");
	std::shared_ptr<Label> lblBrushSize = std::make_shared<Label>(this->manager, "1x", 20, sf::Vector2f(75.f, 0.f), 1, sf::Color(255, 255, 255, 255), "lblBrushSize");
	lblBrushSize->setPosition(position::getSidePosition(btnBrushSizeDecrease->shape->shape->getGlobalBounds(),
													   lblBrushSize->text->getGlobalBounds(),
													   lblBrushSize->text->getPosition(), sf::Vector2i(1, 0)));
	this->manager->addView(std::static_pointer_cast<ViewElement>(lblBrushSize));
	this->labels.emplace_back(lblBrushSize);

	std::shared_ptr<Label> lblRotation = std::make_shared<Label>(this->manager, "Rotation: ", 20, sf::Vector2f(0.f, -50.f), 1, sf::Color(255, 255, 255, 255), "lblRotation");
	lblRotation->setPosition(position::getSidePosition(btnBrushSizeDecrease->shape->shape->getGlobalBounds(),
													   lblRotation->text->getGlobalBounds(),
													   lblRotation->text->getPosition(), sf::Vector2i(0, 1)));
	this->manager->addView(std::static_pointer_cast<ViewElement>(lblRotation));
	this->labels.emplace_back(lblRotation);

	std::shared_ptr<Edit> edtRotation = std::make_shared<Edit>(this->manager, EditType::etInteger, "", sf::Vector2f(0.f, -5.f), "edtRotation", 20, 
															   lblRotation->text->getGlobalBounds(), sf::Vector2i(1, 0));
	edtRotation->integerMaxValue = 360;
	edtRotation->setValue("0");

	std::shared_ptr<Label> lblScale = std::make_shared<Label>(this->manager, "Scale: ", 20, sf::Vector2f(0.f, -50.f), 1, sf::Color(255, 255, 255, 255), "lblScale");
	lblScale->setPosition(position::getSidePosition(lblRotation->text->getGlobalBounds(),
													lblScale->text->getGlobalBounds(),
													lblScale->text->getPosition(), sf::Vector2i(0, 1)));
	this->manager->addView(std::static_pointer_cast<ViewElement>(lblScale));
	this->labels.emplace_back(lblScale);

	std::shared_ptr<Edit> edtScale = std::make_shared<Edit>(this->manager, EditType::etInteger, "", sf::Vector2f(0.f, -5.f), "edtScale", 20,
														   lblScale->text->getGlobalBounds(), sf::Vector2i(1, 0));
	edtScale->integerMaxValue = 1000;
	edtScale->setValue("100");

	std::shared_ptr<Label> lblPriority = std::make_shared<Label>(this->manager, "Priority: ", 20, sf::Vector2f(0.f, -50.f), 1, sf::Color(255, 255, 255, 255), "lblPriority");
	lblPriority->setPosition(position::getSidePosition(lblScale->text->getGlobalBounds(),
													   lblPriority->text->getGlobalBounds(),
													   lblPriority->text->getPosition(), sf::Vector2i(0, 1)));
	this->manager->addView(std::static_pointer_cast<ViewElement>(lblPriority));
	this->labels.emplace_back(lblPriority);

	std::shared_ptr<Edit> edtPriority = std::make_shared<Edit>(this->manager, EditType::etInteger, "", sf::Vector2f(0.f, -5.f), "edtPriority", 20,
															   lblPriority->text->getGlobalBounds(), sf::Vector2i(1, 0));
	edtPriority->integerMaxValue = 100;
	edtPriority->setValue("0");

	std::shared_ptr<Label> lblFormShapeSize = std::make_shared<Label>(this->manager, "Form Shape Size: ", 20, sf::Vector2f(0.f, -50.f), 1, sf::Color(255, 255, 255, 255), "lblFormShapeSize");
	lblFormShapeSize->setPosition(position::getSidePosition(lblPriority->text->getGlobalBounds(),
													   lblFormShapeSize->text->getGlobalBounds(),
													   lblFormShapeSize->text->getPosition(), sf::Vector2i(0, 1)));
	this->manager->addView(std::static_pointer_cast<ViewElement>(lblFormShapeSize));
	this->labels.emplace_back(lblFormShapeSize);

	std::shared_ptr<Edit> edtFormShapeSize = std::make_shared<Edit>(this->manager, EditType::etInteger, "", sf::Vector2f(0.f, -5.f), "edtFormShapeSize", 20,
																	lblFormShapeSize->text->getGlobalBounds(), sf::Vector2i(1, 0));
	edtFormShapeSize->integerMaxValue = 255;
	edtFormShapeSize->setValue("3");

	std::shared_ptr<Label> lblMapSize = std::make_shared<Label>(this->manager, "Size: ", 20, sf::Vector2f(-50.f, -85.f), 1, sf::Color(255, 255, 255, 255), "lblMapSize");
	lblMapSize->setPosition(position::getSidePosition(btnCenterShape->shape->shape->getGlobalBounds(),
													  lblMapSize->text->getGlobalBounds(),
													  lblMapSize->text->getPosition(), sf::Vector2i(0, 1)));
	this->manager->addView(std::static_pointer_cast<ViewElement>(lblMapSize));
	this->labels.emplace_back(lblMapSize);

	std::shared_ptr<Edit> edtMapSizeX = std::make_shared<Edit>(this->manager, EditType::etInteger, "", sf::Vector2f(0.f, -5.f), "edtMapSizeX", 20,
															   lblMapSize->text->getGlobalBounds(), sf::Vector2i(1, 0));
	edtMapSizeX->setValue("3000");
	edtMapSizeX->integerMaxValue = 99999;
	std::shared_ptr<Edit> edtMapSizeY = std::make_shared<Edit>(this->manager, EditType::etInteger, "", sf::Vector2f(10.f, 0.f), "edtMapSizeY", 20,
															   edtMapSizeX->shape->shape->getGlobalBounds(), sf::Vector2i(1, 0));
	edtMapSizeY->setValue("3000");
	edtMapSizeY->integerMaxValue = 99999;

	std::shared_ptr<Label> lblMapName = std::make_shared<Label>(this->manager, "Name: ", 20, sf::Vector2f(0.f, 0.f), 1, sf::Color(255, 255, 255, 255), "lblMapName");
	lblMapName->setPosition(position::getSidePosition(lblMapSize->text->getGlobalBounds(),
													  lblMapName->text->getGlobalBounds(),
													  lblMapName->text->getPosition(), sf::Vector2i(0, 1)));
	this->manager->addView(std::static_pointer_cast<ViewElement>(lblMapName));
	this->labels.emplace_back(lblMapName);

	std::shared_ptr<Edit> edtMapName = std::make_shared<Edit>(this->manager, EditType::etString, "", sf::Vector2f(0.f, 0.f), "edtMapName", 15,
															  lblMapName->text->getGlobalBounds(), sf::Vector2i(1, 0));
	edtMapName->setValue("another_map");
	edtMapName->maxLength = 64;

	std::shared_ptr<Label> lblMapMusic = std::make_shared<Label>(this->manager, "Music: ", 20, sf::Vector2f(10.f, 0.f), 1, sf::Color(255, 255, 255, 255), "lblMapMusic");
	lblMapMusic->setPosition(position::getSidePosition(edtMapSizeY->shape->shape->getGlobalBounds(),
													   lblMapMusic->text->getGlobalBounds(),
													   lblMapMusic->text->getPosition(), sf::Vector2i(1, 0)));
	this->manager->addView(std::static_pointer_cast<ViewElement>(lblMapMusic));
	this->labels.emplace_back(lblMapMusic);

	std::shared_ptr<Edit> edtMapMusic = std::make_shared<Edit>(this->manager, EditType::etString, "", sf::Vector2f(0.f, 0.f), "edtMapMusic", 15,
															   lblMapMusic->text->getGlobalBounds(), sf::Vector2i(1, 0));
	edtMapMusic->setValue("woods");
	edtMapMusic->maxLength = 32;

	std::shared_ptr<Edit> edtMapVersion = std::make_shared<Edit>(this->manager, EditType::etString, "", sf::Vector2f(425.f, 0.f), "edtMapVersion", 15,
																 edtMapName->shape->shape->getGlobalBounds(), sf::Vector2i(1, 0));
	edtMapVersion->setValue("1.00");
	edtMapVersion->maxLength = 16;

	sf::FloatRect extraFieldRegion = btnNew->shape->shape->getGlobalBounds();
	sf::Vector2f extraSpace(0.f, 45.f);
	sf::Vector2i extraSide(0, 1);

	for (int index = 0; index < 7; index++)
	{
		std::shared_ptr<Label> labelIndex = std::make_shared<Label>(this->manager, "-", 14, extraSpace, 1, sf::Color(255, 255, 255, 255),
																	"lblExtraField-" + boost::lexical_cast<std::string>(index));
		labelIndex->setPosition(position::getSidePosition(extraFieldRegion,
														  labelIndex->text->getGlobalBounds(),
														  labelIndex->text->getPosition(), extraSide));
		this->manager->addView(std::static_pointer_cast<ViewElement>(labelIndex));
		this->labels.emplace_back(labelIndex);

		std::shared_ptr<Edit> edtIndex = std::make_shared<Edit>(this->manager, EditType::etString, "", sf::Vector2f(0.f, -5.f), "edtExtraField-" + boost::lexical_cast<std::string>(index), 14,
																labelIndex->text->getGlobalBounds(), sf::Vector2i(1, 0));
		edtIndex->setValue("");

		extraFieldRegion = labelIndex->text->getGlobalBounds();
		extraSide = sf::Vector2i(1, 0);
		extraSpace = sf::Vector2f(edtIndex->shape->shape->getGlobalBounds().width + 10.f, -13.f);

		this->edits.emplace_back(edtIndex);

		labelIndex->visible = false;
		edtIndex->setVisible(false);
	}

	this->buttons.emplace_back(btnNew);
	this->buttons.emplace_back(btnOpen);
	this->buttons.emplace_back(btnReload);
	this->buttons.emplace_back(btnSave);
	this->buttons.emplace_back(btnSaveAs);
	this->buttons.emplace_back(btnReloadConfig);
	this->buttons.emplace_back(btnTrigger);
	this->buttons.emplace_back(btnHelp);
	this->buttons.emplace_back(btnClear);
	this->buttons.emplace_back(btnErase);
	this->buttons.emplace_back(btnGridVisibilityToggle);
	this->buttons.emplace_back(btnSpawnPress);
	this->buttons.emplace_back(btnCenterShape);
	this->buttons.emplace_back(btnMapAreaSize);
	this->buttons.emplace_back(btnGridSpawn);
	this->buttons.emplace_back(btnSelectItem);
	this->buttons.emplace_back(btnSelectItemMove);
	this->buttons.emplace_back(btnUpdateMapBounds);
	this->buttons.emplace_back(btnFormShapeSquare);
	this->buttons.emplace_back(btnFormShapeCircle);
	this->buttons.emplace_back(btnFormShapeNone);	
	this->buttons.emplace_back(btnUnit);
	this->buttons.emplace_back(btnMerchant);
	this->buttons.emplace_back(btnProp);
	this->buttons.emplace_back(btnEnvironment);
	this->buttons.emplace_back(btnTerrain);
	this->buttons.emplace_back(btnPortal);
	this->buttons.emplace_back(btnItem);
	this->buttons.emplace_back(btnRemoveBackground);
	this->buttons.emplace_back(btnPalettePrevious);
	this->buttons.emplace_back(btnPaletteBack);
	this->buttons.emplace_back(btnGridSizeDecrease);
	this->buttons.emplace_back(btnGridSizeIncrease);
	this->buttons.emplace_back(btnBrushSizeDecrease);
	this->buttons.emplace_back(btnBrushSizeIncrease);
	this->buttons.emplace_back(btnMatrix);
	this->buttons.emplace_back(btnDragCursor);
	this->buttons.emplace_back(btnWall);
	this->edits.emplace_back(edtWeatherChance);
	this->edits.emplace_back(edtWeatherName);
	this->edits.emplace_back(edtParticles);
	this->edits.emplace_back(edtRotation);
	this->edits.emplace_back(edtScale);
	this->edits.emplace_back(edtMapSizeX);
	this->edits.emplace_back(edtMapSizeY);
	this->edits.emplace_back(edtMapName);
	this->edits.emplace_back(edtMapMusic);
	this->edits.emplace_back(edtMapVersion);
	this->edits.emplace_back(edtPriority);
	this->edits.emplace_back(edtFormShapeSize);

	return true;
}
bool Hud::loadLabels()
{
	std::shared_ptr<Label> lblCoordinates = std::make_shared<Label>(this->manager, "0, 0", 20, sf::Vector2f(1630.f, 70.f), 1, sf::Color(255, 255, 255, 255), "lblCoordinates");
	std::shared_ptr<Label> lblPaletteItem = std::make_shared<Label>(this->manager, "-", 15, sf::Vector2f(0.f, 0.f), 1, sf::Color(255, 255, 255, 255), "lblPaletteItem");
	std::shared_ptr<Label> lblVersion = std::make_shared<Label>(this->manager, "By Mence v1.07", 20, sf::Vector2f(1760.f, 0.f), 1, sf::Color(255, 255, 255, 255), "lblVersion");
	std::shared_ptr<Label> lblPaletteStatus = std::make_shared<Label>(this->manager, "- - -", 30, sf::Vector2f(0.f, 960.f), 1, sf::Color(255, 255, 255, 255), "lblPaletteStatus");
	std::shared_ptr<Label> lblMessageBox = std::make_shared<Label>(this->manager, "", 20, sf::Vector2f(0.f, 900.f), 1, sf::Color(255, 255, 255, 255), "lblMessageBox");
	

	lblPaletteItem->setPosition(position::getSidePosition(lblCoordinates->text->getGlobalBounds(),
														  lblPaletteItem->text->getGlobalBounds(), 
														  lblPaletteItem->text->getPosition(), sf::Vector2i(0, 1)));
	
	this->manager->addView(std::static_pointer_cast<ViewElement>(lblCoordinates));
	this->manager->addView(std::static_pointer_cast<ViewElement>(lblPaletteItem));
	this->manager->addView(std::static_pointer_cast<ViewElement>(lblVersion));
	this->manager->addView(std::static_pointer_cast<ViewElement>(lblPaletteStatus));
	this->manager->addView(std::static_pointer_cast<ViewElement>(lblMessageBox));
	
	this->labels.emplace_back(lblCoordinates);	
	this->labels.emplace_back(lblPaletteItem);
	this->labels.emplace_back(lblVersion);
	this->labels.emplace_back(lblPaletteStatus);
	this->labels.emplace_back(lblMessageBox);

	lblMessageBox->visible = false;
	this->messageBox.label = lblMessageBox;

	this->shapeTooltip = std::make_shared<Model>(this->manager, sf::Vector2f(0.f, 0.f), "", 1, false);
	this->shapeTooltip->loadShape(sf::Vector2f(1.f, 1.f), sf::Color(0, 0, 0, 255));
	this->manager->addView(std::static_pointer_cast<ViewElement>(this->shapeTooltip));
	this->shapeTooltip->visible = false;

	this->labelTooltip = std::make_shared<Label>(this->manager, "", 15, sf::Vector2f(0.f, 0.f), 1, sf::Color(255, 255, 0, 255), "lblTooltip");
	this->labelTooltip->canvasBound = false;
	this->manager->addView(std::static_pointer_cast<ViewElement>(labelTooltip));
	this->labels.emplace_back(labelTooltip);
	this->labelTooltip->visible = false;

	return true;
}

bool Hud::loadLists()
{
	this->unloadLists();

	this->loadGrid();
	this->loadModels();
	this->loadButtons();
	this->loadLabels();	

	return true;
}