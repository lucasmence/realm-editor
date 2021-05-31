#include <boost/lexical_cast.hpp>
#include <boost/algorithm/string.hpp>
#include <math.h>
#include "hud.hpp"
#include "../manager.hpp"
#include "../library/position.hpp"

Hud::Hud(Manager* manager)
{
	this->manager = manager;
	this->gridSizeList = { 32, 64, 128, 256 };
	this->brushSizeList = { 1, 4, 16 };
	this->gridSize = 1;
	this->brushSize = 0;
	this->rotation = 0;
	this->priority = 0;
	this->scale = 1.f;
	this->spawnPress = false;
	this->mousePressed = false;
	this->centerShape = false;
	this->mouseRightButton = false;
	this->matrixActivated = false;
	this->matrixTriggered = false;
	this->matrixPosSpawn = false;
	this->hoverShapeSize = sf::Vector2f(0.f, 0.f);
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
	
	this->loadLists();
}

Hud::~Hud()
{
	this->manager->removeView(std::static_pointer_cast<ViewElement>(this->shapeHover));
	this->manager->removeView(std::static_pointer_cast<ViewElement>(this->shapeMapArea));
	this->manager->removeView(std::static_pointer_cast<ViewElement>(this->shapeMatrix));
	this->unloadLists();
}

bool Hud::update(sf::Vector2f cursor)
{
	this->updateCursor(cursor);
	this->updateLabels(cursor);
	this->updateButtonsColor(cursor);
	this->updateEditsColor(cursor);
	this->updateEditValues();
	this->updateHoverShapeSize();
	this->updateHoverMapSize();
	this->updateMousePressed(cursor);

	return true;
}

bool Hud::updateClick(sf::Vector2f cursor, bool rightButton)
{
	this->mousePressed = true;
	this->mouseRightButton = rightButton;
	this->buttonsClick(cursor);
	this->matrixActivate(cursor);
	this->spawnClick(cursor);
	this->editsClick(cursor);

	return true;
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
	
	sf::Vector2f positionCenter(0.f, 0.f);
	if (this->centerShape)
		positionCenter = sf::Vector2f(this->shapeHover->shape->getGlobalBounds().width / 2.f, this->shapeHover->shape->getGlobalBounds().height / 2.f);

	this->shapeHover->visible = true;
	this->shapeHover->shape->setPosition(position::getGridPosition(sf::Vector2f(this->gridSizeList.at(this->gridSize) / 2.f + (positionCenter.x),
																				this->gridSizeList.at(this->gridSize) / 2.f + (positionCenter.y)),
																				cursor));

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
			text->setString("- - -");
			text->setFillColor(sf::Color(255, 255, 255, 255));
			break;
		}
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

bool Hud::toggleMatrixTriggered(std::shared_ptr<Button> button)
{
	button->selected = !button->selected;
	this->matrixActivated = button->selected;
	return true;
}

bool Hud::toggleMapAreaSize(std::shared_ptr<Button> button)
{
	button->selected = !button->selected;
	this->shapeMapArea->visible = button->selected;
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
	if (this->matrixTriggered || this->spawnPress || !this->matrixActivated || this->manager->palette->status != PaletteStatus::psInsert)
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

bool Hud::spawnClick(sf::Vector2f cursor)
{
	if (this->matrixTriggered)
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
			std::list<MapObjectField> fields = {};

			int priorityValue = this->priority;

			switch (this->manager->palette->type)
			{
				case (PaletteType::ptTerrain) :
				{
					objectType = MapObjectType::motTerrain;
					paletteTypeField += "terrain";

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
					paletteTypeField += "environment";

					break;
				}
				case (PaletteType::ptUnit):
				{
					objectType = MapObjectType::motUnit;
					paletteTypeField = "";
					texture = this->manager->palette->selectedTexture;
					std::vector<EditValue> extraValues = this->getExtraEditsValue();

					MapObjectField fieldObject;
					fieldObject.field = "alliance";
					fieldObject.valueString = MapObjectFieldString{ extraValues.at(0).string, true};
					fields.emplace_back(fieldObject);

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
			}

			priorityValue += this->manager->map->getObjectPriority(objectType);

			sf::Vector2f tilesetPosition(this->shapeHover->shape->getPosition());
			sf::Vector2f tilesetOrigin(this->shapeHover->shape->getOrigin());

			sf::Vector2f hoverCenter(this->shapeHover->shape->getPosition().x + this->shapeHover->shape->getGlobalBounds().width / 2.f,
									 this->shapeHover->shape->getPosition().y + this->shapeHover->shape->getGlobalBounds().height / 2.f);

			if (this->mousePressed && this->spawnPress)
				for (auto& object : this->manager->map->objects)
					if (object.type == objectType && object.model->sprite->getGlobalBounds().contains(hoverCenter) &&
						object.model->texture->filename == paletteTypeField + "/" + texture)
						return false;			

			for (int x = 0; x < sqrt(this->brushSizeList.at(this->brushSize)); x++)
				for (int y = 0; y < sqrt(this->brushSizeList.at(this->brushSize)); y++)
				{
					std::shared_ptr<Model> model = std::make_shared<Model>(this->manager, 
																		   tilesetPosition, 
																		   paletteTypeField + "/" + texture, priorityValue, false, "", this->manager->palette->selectedOrigin);
					model->sprite->setOrigin(tilesetOrigin);
					model->sprite->move(model->sprite->getGlobalBounds().width * x, model->sprite->getGlobalBounds().height * y);
					model->sprite->setRotation(this->rotation);
					model->sprite->setScale(this->scale, this->scale);
					this->manager->addView(std::static_pointer_cast<ViewElement>(model));
					this->manager->map->addObjectUnit(MapObjectUnit{ objectType, model->sprite->getPosition(), 0.f, model, fields });
				}

			break;
		}

		case (PaletteStatus::psDelete):
		{
			MapObjectUnit objectSelected{ MapObjectType::motTerrain , sf::Vector2f(0.f, 0.f), 0.f, nullptr, {} };
			for (auto& object : this->manager->map->objects)
				if (object.model->sprite->getGlobalBounds().contains(cursor))
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

bool Hud::buttonsClick(sf::Vector2f cursor)
{
	for (auto& button : this->buttons)
		if (button->shape->shape->getGlobalBounds().contains(cursor))
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
		if (button->shape->shape->getGlobalBounds().contains(cursor))
		{
			button->shape->shape->setFillColor(sf::Color(200, 200, 50, 100));
			button->label->text->setFillColor(sf::Color(200, 200, 50, 255));
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
		if (edit->shape->shape->getGlobalBounds().contains(cursor))
		{
			edit->shape->shape->setFillColor(sf::Color(50, 200, 50, 100));
			edit->label->text->setFillColor(sf::Color(50, 200, 50, 255));
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
	if (editName == "" || value == "")
		return false;

	for (auto& edit : this->edits)
		if (edit->name == editName)
		{
			edit->setValue(value);
			return true;
		}

	return false;
}

bool Hud::updateExtraEditsValue(std::vector<std::string> caption, std::vector<EditType> type, std::vector<std::string> value, std::vector<int> maxValue)
{
	if (caption.size() == 0)
	{
		for (int index = 0; index < 5; index++)
		{
			for (auto& label : this->labels)
				if (label->name == "lblExtraField-" + boost::lexical_cast<std::string>(index))
					label->visible = false;

			for (auto& edit : this->edits)
				if (edit->name == "edtExtraField-" + boost::lexical_cast<std::string>(index))
					edit->setVisible(false);
		}

		return false;	
	}

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

						switch (edit->type)
						{
							case (EditType::etString):
							{
								edit->maxLength = maxValue.at(index);
								break;
							}
							case (EditType::etInteger):
							{
								edit->integerMaxValue = maxValue.at(index);
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
						extraSpace = sf::Vector2f(45.f, 0.f);

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

	for (int index = 0; index < 5; index++)
		for (auto& edit : this->edits)
			if (edit->name == "edtExtraField-" + boost::lexical_cast<std::string>(index) && edit->label->visible)
				values.emplace_back(edit->getValue());

	return values;
}

bool Hud::updateEditValues()
{
	for (auto& edit : this->edits)
		if (edit->name == "edtRotation")
			this->rotation = edit->getValue().integer;
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

	for (int x = 0; x < (int(40 / (this->gridSize + 1))); x++)
	{
		std::shared_ptr<Model> line = std::make_shared<Model>(this->manager, sf::Vector2f(0.f, x * distance.y), "", 3);

		line->loadShape(sf::Vector2f(2000.f, 1.f), sf::Color(0, 255, 0, 100));
		this->manager->addView(std::static_pointer_cast<ViewElement>(line));
		this->grid.emplace_back(line);
	}

	for (int y = 0; y < (int(64 / (this->gridSize + 1))); y++)
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

	return true;
}
bool Hud::loadButtons()
{
	std::shared_ptr<Button> btnNew = std::make_shared<Button>(this->manager, "[New]", sf::Vector2f(15.f, 10.f), "btnNew", 20);
	std::shared_ptr<Button> btnOpen = std::make_shared<Button>(this->manager, "[Open]", sf::Vector2f(0.f, 0.f), "btnOpen", 20, btnNew, sf::Vector2i(1, 0));
	std::shared_ptr<Button> btnSave = std::make_shared<Button>(this->manager, "[Save]", sf::Vector2f(0.f, 0.f), "btnSave", 20, btnOpen, sf::Vector2i(1, 0));
	std::shared_ptr<Button> btnSaveAs = std::make_shared<Button>(this->manager, "[Save As]", sf::Vector2f(0.f, 0.f), "btnSaveAs", 20, btnSave, sf::Vector2i(1, 0));
	std::shared_ptr<Button> btnHelp = std::make_shared<Button>(this->manager, "[Help]", sf::Vector2f(0.f, 0.f), "btnHelp", 20, btnSaveAs, sf::Vector2i(1, 0));

	std::shared_ptr<Button> btnClear = std::make_shared<Button>(this->manager, "[C]", sf::Vector2f(1325.f, 65.f), "btnClear", 20);
	std::shared_ptr<Button> btnErase = std::make_shared<Button>(this->manager, "[E]", sf::Vector2f(0.f, 0.f), "btnErase", 20, btnClear, sf::Vector2i(-1, 0));
	std::shared_ptr<Button> btnGridVisibilityToggle = std::make_shared<Button>(this->manager, "[G]", sf::Vector2f(0.f, 0.f), "btnGridVisibilityToggle", 20, btnClear, sf::Vector2i(1, 0));
	std::shared_ptr<Button> btnSpawnPress = std::make_shared<Button>(this->manager, "[P]", sf::Vector2f(0.f, 0.f), "btnSpawnPress", 20, btnErase, sf::Vector2i(-1, 0));
	std::shared_ptr<Button> btnCenterShape = std::make_shared<Button>(this->manager, "[S]", sf::Vector2f(0.f, 0.f), "btnCenterShape", 20, btnSpawnPress, sf::Vector2i(-1, 0));
	std::shared_ptr<Button> btnMatrix = std::make_shared<Button>(this->manager, "[M]", sf::Vector2f(0.f, 0.f), "btnMatrix", 20, btnCenterShape, sf::Vector2i(-1, 0));
	std::shared_ptr<Button> btnMapAreaSize = std::make_shared<Button>(this->manager, "[A]", sf::Vector2f(0.f, 0.f), "btnMapAreaSize", 20, btnGridVisibilityToggle, sf::Vector2i(1, 0));
	
	std::shared_ptr<Button> btnUnit = std::make_shared<Button>(this->manager, "[Units]", sf::Vector2f(1650.f, 200.f), "btnUnit", 20);
	std::shared_ptr<Button> btnMerchant = std::make_shared<Button>(this->manager, "[Merchants]", sf::Vector2f(0.f, 0.f), "btnMerchant", 20, btnUnit, sf::Vector2i(1, 0));
	std::shared_ptr<Button> btnProp = std::make_shared<Button>(this->manager, "[Props]", sf::Vector2f(0.f, 0.f), "btnProp", 20, btnUnit, sf::Vector2i(0, 1));
	std::shared_ptr<Button> btnEnvironment = std::make_shared<Button>(this->manager, "[Environments]", sf::Vector2f(0.f, 0.f), "btnEnvironment", 20, btnProp, sf::Vector2i(1, 0));
	std::shared_ptr<Button> btnTerrain = std::make_shared<Button>(this->manager, "[Terrain]", sf::Vector2f(0.f, 0.f), "btnTerrain", 20, btnProp, sf::Vector2i(0, 1));
	std::shared_ptr<Button> btnPortal = std::make_shared<Button>(this->manager, "[Portals]", sf::Vector2f(0.f, 0.f), "btnPortal", 20, btnTerrain, sf::Vector2i(1, 0));

	std::shared_ptr<Button> btnPalettePrevious = std::make_shared<Button>(this->manager, "[<]", sf::Vector2f(0.f, 15.f), "btnPalettePrevious", 20, btnTerrain, sf::Vector2i(0, 1));
	std::shared_ptr<Button> btnPaletteBack = std::make_shared<Button>(this->manager, "[>]", sf::Vector2f(175.f, 0.f), "btnPaletteNext", 20, btnPalettePrevious, sf::Vector2i(1, 0));
	std::shared_ptr<Label> lblPalettePage = std::make_shared<Label>(this->manager, "1", 20, sf::Vector2f(85.f, 0.f), 1, sf::Color(255, 255, 255, 255), "lblPalettePage");
	lblPalettePage->setPosition(position::getSidePosition(btnPalettePrevious->shape->shape->getGlobalBounds(), 
														  lblPalettePage->text->getGlobalBounds(), 
														  lblPalettePage->text->getPosition(), sf::Vector2i(1, 0)));
	this->manager->addView(std::static_pointer_cast<ViewElement>(lblPalettePage));
	this->labels.emplace_back(lblPalettePage);

	std::shared_ptr<Button> btnGridSizeDecrease = std::make_shared<Button>(this->manager, "[<]", sf::Vector2f(0.f, 700.f), "btnGridSizeDecrease", 20, btnTerrain, sf::Vector2i(0, 1));
	std::shared_ptr<Button> btnGridSizeIncrease = std::make_shared<Button>(this->manager, "[>]", sf::Vector2f(175.f, 0.f), "btnGridSizeIncrease", 20, btnGridSizeDecrease, sf::Vector2i(1, 0));
	std::shared_ptr<Label> lblGridSize = std::make_shared<Label>(this->manager, "64px", 20, sf::Vector2f(65.f, 0.f), 1, sf::Color(255, 255, 255, 255), "lblGridSize");
	lblGridSize->setPosition(position::getSidePosition(btnGridSizeDecrease->shape->shape->getGlobalBounds(),
													   lblGridSize->text->getGlobalBounds(),
													   lblGridSize->text->getPosition(), sf::Vector2i(1, 0)));
	this->manager->addView(std::static_pointer_cast<ViewElement>(lblGridSize));
	this->labels.emplace_back(lblGridSize);

	std::shared_ptr<Button> btnBrushSizeDecrease = std::make_shared<Button>(this->manager, "[<]", sf::Vector2f(0.f, -50.f), "btnBrushSizeDecrease", 20, btnGridSizeDecrease, sf::Vector2i(0, 1));
	std::shared_ptr<Button> btnBrushSizeIncrease = std::make_shared<Button>(this->manager, "[>]", sf::Vector2f(175.f, 0.f), "btnBrushSizeIncrease", 20, btnBrushSizeDecrease, sf::Vector2i(1, 0));
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

	std::shared_ptr<Label> lblMapSize = std::make_shared<Label>(this->manager, "Size: ", 20, sf::Vector2f(-50.f, -85.f), 1, sf::Color(255, 255, 255, 255), "lblMapSize");
	lblMapSize->setPosition(position::getSidePosition(btnCenterShape->shape->shape->getGlobalBounds(),
													  lblMapSize->text->getGlobalBounds(),
													  lblMapSize->text->getPosition(), sf::Vector2i(0, 1)));
	this->manager->addView(std::static_pointer_cast<ViewElement>(lblMapSize));
	this->labels.emplace_back(lblMapSize);

	std::shared_ptr<Edit> edtMapSizeX = std::make_shared<Edit>(this->manager, EditType::etInteger, "", sf::Vector2f(0.f, -5.f), "edtMapSizeX", 20,
															   lblMapSize->text->getGlobalBounds(), sf::Vector2i(1, 0));
	edtMapSizeX->setValue("1000");
	edtMapSizeX->integerMaxValue = 99999;
	std::shared_ptr<Edit> edtMapSizeY = std::make_shared<Edit>(this->manager, EditType::etInteger, "", sf::Vector2f(10.f, 0.f), "edtMapSizeY", 20,
															   edtMapSizeX->shape->shape->getGlobalBounds(), sf::Vector2i(1, 0));
	edtMapSizeY->setValue("1000");
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
	edtMapMusic->setValue("village");
	edtMapMusic->maxLength = 32;

	std::shared_ptr<Edit> edtMapVersion = std::make_shared<Edit>(this->manager, EditType::etString, "", sf::Vector2f(425.f, 0.f), "edtMapVersion", 15,
																 edtMapName->shape->shape->getGlobalBounds(), sf::Vector2i(1, 0));
	edtMapVersion->setValue("1.00");
	edtMapVersion->maxLength = 16;

	sf::FloatRect extraFieldRegion = btnNew->shape->shape->getGlobalBounds();
	sf::Vector2f extraSpace(0.f, 45.f);
	sf::Vector2i extraSide(0, 1);

	for (int index = 0; index < 5; index++)
	{
		std::shared_ptr<Label> labelIndex = std::make_shared<Label>(this->manager, "-", 15, extraSpace, 1, sf::Color(255, 255, 255, 255),
																	"lblExtraField-" + boost::lexical_cast<std::string>(index));
		labelIndex->setPosition(position::getSidePosition(extraFieldRegion,
														  labelIndex->text->getGlobalBounds(),
														  labelIndex->text->getPosition(), extraSide));
		this->manager->addView(std::static_pointer_cast<ViewElement>(labelIndex));
		this->labels.emplace_back(labelIndex);

		std::shared_ptr<Edit> edtIndex = std::make_shared<Edit>(this->manager, EditType::etString, "", sf::Vector2f(0.f, -5.f), "edtExtraField-" + boost::lexical_cast<std::string>(index), 15,
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
	this->buttons.emplace_back(btnSave);
	this->buttons.emplace_back(btnSaveAs);
	this->buttons.emplace_back(btnHelp);
	this->buttons.emplace_back(btnClear);
	this->buttons.emplace_back(btnErase);
	this->buttons.emplace_back(btnGridVisibilityToggle);
	this->buttons.emplace_back(btnSpawnPress);
	this->buttons.emplace_back(btnCenterShape);
	this->buttons.emplace_back(btnMapAreaSize);
	this->buttons.emplace_back(btnUnit);
	this->buttons.emplace_back(btnMerchant);
	this->buttons.emplace_back(btnProp);
	this->buttons.emplace_back(btnEnvironment);
	this->buttons.emplace_back(btnTerrain);
	this->buttons.emplace_back(btnPortal);
	this->buttons.emplace_back(btnPalettePrevious);
	this->buttons.emplace_back(btnPaletteBack);
	this->buttons.emplace_back(btnGridSizeDecrease);
	this->buttons.emplace_back(btnGridSizeIncrease);
	this->buttons.emplace_back(btnBrushSizeDecrease);
	this->buttons.emplace_back(btnBrushSizeIncrease);
	this->buttons.emplace_back(btnMatrix);
	this->edits.emplace_back(edtRotation);
	this->edits.emplace_back(edtScale);
	this->edits.emplace_back(edtMapSizeX);
	this->edits.emplace_back(edtMapSizeY);
	this->edits.emplace_back(edtMapName);
	this->edits.emplace_back(edtMapMusic);
	this->edits.emplace_back(edtMapVersion);
	this->edits.emplace_back(edtPriority);

	return true;
}
bool Hud::loadLabels()
{
	std::shared_ptr<Label> lblCoordinates = std::make_shared<Label>(this->manager, "0, 0", 20, sf::Vector2f(1630.f, 70.f), 1, sf::Color(255, 255, 255, 255), "lblCoordinates");
	std::shared_ptr<Label> lblPaletteItem = std::make_shared<Label>(this->manager, "0, 0", 20, sf::Vector2f(0.f, 0.f), 1, sf::Color(255, 255, 255, 255), "lblPaletteItem");
	std::shared_ptr<Label> lblVersion = std::make_shared<Label>(this->manager, "0.02", 20, sf::Vector2f(1870.f, 0.f), 1, sf::Color(255, 255, 255, 255), "lblVersion");
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