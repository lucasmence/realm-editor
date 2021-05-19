#include <iostream>
#include <string>
#include <fstream>
#include <sstream>
#include <type_traits>
#include <boost/filesystem.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/lexical_cast.hpp>
#include "palette.hpp"
#include "../manager.hpp"
#include "../library/json.hpp"

Palette::Palette(Manager* manager)
{
    this->status = PaletteStatus::psNone;
    this->pageIndex = 0;
    this->selectedItem = "";
	this->type = PaletteType::ptTerrain;
	this->manager = manager;
    this->loadPalettes();
}

Palette::~Palette()
{
    
}

bool Palette::unloadPalettes()
{
    this->terrain.clear();
    return true;
}
bool Palette::loadPalettes()
{
    this->unloadPalettes();
    this->terrain = this->loadFileLists("textures/terrain");

    this->selectPalette();

    return true;
}

bool Palette::clearPaletteItems()
{
    for (auto& item : this->paletteItems)
        this->manager->removeView(item.model);
    this->paletteItems.clear();

    if (this->pageIndex < 0)
        this->pageIndex = 0;

    return true;
}

bool Palette::selectPalette()
{
    this->clearPaletteItems();
    
    int x = 0, y = 0, xLimit = 4, pageLimit = 32, index = 0, count = pageLimit * this->pageIndex, countLimit = pageLimit * (this->pageIndex + 1);
    sf::Vector2f initialPosition(1650.f, 300.f);

    switch (this->type)
    {
        case (PaletteType::ptTerrain):
        {
            for (auto& filename : this->terrain)
            {
                index++;
                if (index < count)
                    continue;

                std::shared_ptr<Model> model = std::make_shared<Model>(this->manager, sf::Vector2f(initialPosition.x + x * 64.f, initialPosition.y + y * 64.f), "textures/terrain/" + filename);
                model->sprite->setScale(sf::Vector2f(64.f / model->sprite->getGlobalBounds().width, 64.f / model->sprite->getGlobalBounds().height));
                this->manager->addView(std::static_pointer_cast<ViewElement>(model));
                this->paletteItems.emplace_back(PaletteItem{model, filename});
                x++;
                count++;
                if (x >= xLimit)
                {
                    y++;
                    x = 0;
                }
                if (count >= countLimit)
                    break;
            }
            break;
        }
    }

    if (this->pageIndex > 0 && this->paletteItems.size() <= 0)
    {
        this->pageIndex--;
        this->selectPalette();
    }

    return true;
}

bool Palette::erasePaletteItem()
{
    this->clearPaletteItem();
    this->status = PaletteStatus::psDelete;
    return true;
}

bool Palette::clearPaletteItem()
{
    this->status = PaletteStatus::psNone;
    this->selectedItem = "";
    this->manager->hud->shapeHover->visible = false;
    for (auto& item : this->paletteItems)
            item.model->sprite->setColor(sf::Color(255, 255, 255, 255));

    return true;
}

bool Palette::selectPaletteItem(sf::Vector2f cursor)
{
    std::string filename = "";
    for (auto& item : this->paletteItems)
        if (item.model->sprite->getGlobalBounds().contains(cursor))
        {
            item.model->sprite->setColor(sf::Color(255,0,0,255));
            filename = item.filename;
            this->selectedItem = item.filename;
            std::static_pointer_cast<sf::RectangleShape>(this->manager->hud->shapeHover->shape)->setSize(sf::Vector2f(item.model->sprite->getGlobalBounds().width / item.model->sprite->getScale().x,
                                                                                                                      item.model->sprite->getGlobalBounds().height / item.model->sprite->getScale().y));
            this->manager->hud->shapeHover->shape->setOrigin(sf::Vector2f(0.f * this->manager->hud->shapeHover->shape->getGlobalBounds().width / 2.f,
                                                                          0.f * this->manager->hud->shapeHover->shape->getGlobalBounds().height / 2.f));
            this->manager->hud->shapeHover->visible = true;
            this->status = PaletteStatus::psInsert;
            break;
        }

    for (auto& item : this->paletteItems)
        if (filename != item.filename)
            item.model->sprite->setColor(sf::Color(255, 255, 255, 255));

    return true;
}

std::list<std::string> Palette::loadFileLists(std::string directory)
{
    boost::filesystem::path path = boost::filesystem::current_path() /= "data/" + directory;
    boost::filesystem::directory_iterator iterator{ path };

    std::list<std::string> listFiles;
    listFiles.clear();

    while (iterator != boost::filesystem::directory_iterator{})
    {
        std::ostringstream stringStreamFile;
        stringStreamFile << *iterator++;
        std::string filename = this->getString(stringStreamFile.str());
        if (boost::filesystem::extension(filename) == ".json")
            listFiles.emplace_back(boost::filesystem::basename(filename));
    }

    return listFiles;
}

std::string Palette::getString(std::string value)
{
    boost::erase_all(value, "\"");
    return value;
}
