#include <boost/lexical_cast.hpp>
#include <boost/algorithm/string.hpp>
#include "hud.hpp"
#include "../manager.hpp"
#include "../library/position.hpp"

Hud::Hud(Manager* manager)
{
	this->manager = manager;
	this->shapeHover = std::make_shared<Model>(this->manager, sf::Vector2f(0.f, 300.f), "", 1, false);
	this->shapeHover->loadShape(sf::Vector2f(1.f, 1.f), sf::Color(150, 200, 150, 100));
	this->shapeHover->visible = false;
	this->manager->addView(std::static_pointer_cast<ViewElement>(this->shapeHover));
	this->loadLists();
}

Hud::~Hud()
{
	this->manager->removeView(std::static_pointer_cast<ViewElement>(this->shapeHover));
	this->unloadLists();
}

bool Hud::update(sf::Vector2f cursor)
{
	this->updateCursor(cursor);
	this->updateLabels(cursor);
	this->updateButtonsColor(cursor);

	return true;
}

bool Hud::updateClick(sf::Vector2f cursor)
{
	this->buttonsClick(cursor);
	this->spawnClick(cursor);
	return true;
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
	this->shapeHover->shape->setPosition(position::getGridPosition(sf::Vector2f(32.f, 32.f), cursor));

	return true;
}

bool Hud::updateLabels(sf::Vector2f cursor)
{
	for (auto& label : this->labels)
		if (label->name == "lblCoordinates" && this->manager->hasFocus)
			label->text->setString("(" + boost::lexical_cast<std::string>((int)cursor.x) + ", " + boost::lexical_cast<std::string>((int)cursor.y) +")");
		else if (label->name == "lblPalettePage")
			label->text->setString(boost::lexical_cast<std::string>(this->manager->palette->pageIndex+1));
		else if (label->name == "lblPaletteItem")
			label->text->setString(this->manager->palette->selectedItem);
	return true;
}

bool Hud::spawnClick(sf::Vector2f cursor)
{
	if (this->manager->palette->selectedItem == "" || !this->shapeHover->visible)
		return false;

	std::shared_ptr<Model> model = std::make_shared<Model>(this->manager, this->shapeHover->shape->getPosition(), "textures/terrain/" + this->manager->palette->selectedItem, 5, false);
	model->sprite->setOrigin(this->shapeHover->shape->getOrigin());
	this->manager->addView(std::static_pointer_cast<ViewElement>(model));

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
				this->manager->palette->selectPalette();
			}
			else if (button->name == "btnPaletteNext")
			{
				this->manager->palette->pageIndex++;
				this->manager->palette->selectPalette();
			}
			else if (button->name == "btnClear")
				this->manager->palette->clearPaletteItem();

			return true;
			break;
		}
	return false;
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

bool Hud::unloadLists()
{
	for (auto& button : this->buttons)
		button->clear();
	this->buttons.clear();
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

	sf::Vector2f distance(64.f, 64.f);

	for (int x = 0; x < 20; x++)
	{
		std::shared_ptr<Model> line = std::make_shared<Model>(this->manager, sf::Vector2f(0.f, x * distance.y));

		line->loadShape(sf::Vector2f(2000.f, 1.f), sf::Color(0, 255, 0, 100));
		this->manager->addView(std::static_pointer_cast<ViewElement>(line));
		this->grid.emplace_back(line);
	}

	for (int y = 0; y < 32; y++)
	{
		std::shared_ptr<Model> line = std::make_shared<Model>(this->manager, sf::Vector2f(y * distance.x, 0.f));

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

	header->loadShape(sf::Vector2f(2020.f, 200.f), sf::Color(100, 100, 100, 255));
	palette->loadShape(sf::Vector2f(400.f, 1000.f), sf::Color(100, 100, 100, 255));

	this->manager->addView(std::static_pointer_cast<ViewElement>(header));
	this->manager->addView(std::static_pointer_cast<ViewElement>(palette));

	this->models.emplace_back(header);
	this->models.emplace_back(palette);

	return true;
}
bool Hud::loadButtons()
{
	std::shared_ptr<Button> btnNew = std::make_shared<Button>(this->manager, "[New]", sf::Vector2f(25.f, 15.f), "btnNew", 20);
	std::shared_ptr<Button> btnOpen = std::make_shared<Button>(this->manager, "[Open]", sf::Vector2f(0.f, 0.f), "btnOpen", 20, btnNew, sf::Vector2i(1, 0));
	std::shared_ptr<Button> btnSave = std::make_shared<Button>(this->manager, "[Save]", sf::Vector2f(0.f, 0.f), "btnSave", 20, btnOpen, sf::Vector2i(1, 0));

	std::shared_ptr<Button> btnClear = std::make_shared<Button>(this->manager, "[C]", sf::Vector2f(1400.f, 55.f), "btnClear", 20);

	std::shared_ptr<Button> btnUnit = std::make_shared<Button>(this->manager, "[Units]", sf::Vector2f(1650.f, 250.f), "btnUnit", 20);
	std::shared_ptr<Button> btnMerchant = std::make_shared<Button>(this->manager, "[Merchants]", sf::Vector2f(0.f, 0.f), "btnMerchant", 20, btnUnit, sf::Vector2i(1, 0));
	std::shared_ptr<Button> btnProp = std::make_shared<Button>(this->manager, "[Props]", sf::Vector2f(0.f, 0.f), "btnProp", 20, btnUnit, sf::Vector2i(0, 1));
	std::shared_ptr<Button> btnEnvironment = std::make_shared<Button>(this->manager, "[Environments]", sf::Vector2f(0.f, 0.f), "btnEnvironment", 20, btnProp, sf::Vector2i(1, 0));
	std::shared_ptr<Button> btnTerrain = std::make_shared<Button>(this->manager, "[Terrain]", sf::Vector2f(0.f, 0.f), "btnTerrain", 20, btnProp, sf::Vector2i(0, 1));
	std::shared_ptr<Button> btnPortal = std::make_shared<Button>(this->manager, "[Portals]", sf::Vector2f(0.f, 0.f), "btnPortal", 20, btnTerrain, sf::Vector2i(1, 0));

	std::shared_ptr<Button> btnPalettePrevious = std::make_shared<Button>(this->manager, "[<]", sf::Vector2f(0.f, 700.f), "btnPalettePrevious", 20, btnTerrain, sf::Vector2i(0, 1));
	std::shared_ptr<Button> btnPaletteBack = std::make_shared<Button>(this->manager, "[>]", sf::Vector2f(175.f, 0.f), "btnPaletteNext", 20, btnPalettePrevious, sf::Vector2i(1, 0));
	std::shared_ptr<Label> lblPalettePage = std::make_shared<Label>(this->manager, "1", 20, sf::Vector2f(85.f, 0.f), 1, sf::Color(255, 255, 255, 255), "lblPalettePage");
	lblPalettePage->setPosition(position::getSidePosition(btnPalettePrevious->shape->shape->getGlobalBounds(), 
														  lblPalettePage->text->getGlobalBounds(), 
														  lblPalettePage->text->getPosition(), sf::Vector2i(1, 0)));
	this->manager->addView(std::static_pointer_cast<ViewElement>(lblPalettePage));
	this->labels.emplace_back(lblPalettePage);

	this->buttons.emplace_back(btnNew);
	this->buttons.emplace_back(btnOpen);
	this->buttons.emplace_back(btnSave);
	this->buttons.emplace_back(btnClear);
	this->buttons.emplace_back(btnUnit);
	this->buttons.emplace_back(btnMerchant);
	this->buttons.emplace_back(btnProp);
	this->buttons.emplace_back(btnEnvironment);
	this->buttons.emplace_back(btnTerrain);
	this->buttons.emplace_back(btnPortal);
	this->buttons.emplace_back(btnPalettePrevious);
	this->buttons.emplace_back(btnPaletteBack);

	return true;
}
bool Hud::loadLabels()
{
	std::shared_ptr<Label> lblCoordinates = std::make_shared<Label>(this->manager, "0, 0", 20, sf::Vector2f(1630.f, 80.f), 1, sf::Color(255, 255, 255, 255), "lblCoordinates");
	std::shared_ptr<Label> lblPaletteItem = std::make_shared<Label>(this->manager, "0, 0", 20, sf::Vector2f(0.f, 0.f), 1, sf::Color(255, 255, 255, 255), "lblPaletteItem");

	lblPaletteItem->setPosition(position::getSidePosition(lblCoordinates->text->getGlobalBounds(),
														  lblPaletteItem->text->getGlobalBounds(), 
														  lblPaletteItem->text->getPosition(), sf::Vector2i(0, 1)));
	
	this->manager->addView(std::static_pointer_cast<ViewElement>(lblCoordinates));
	this->manager->addView(std::static_pointer_cast<ViewElement>(lblPaletteItem));
	
	this->labels.emplace_back(lblCoordinates);	
	this->labels.emplace_back(lblPaletteItem);

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