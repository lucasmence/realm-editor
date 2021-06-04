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
    this->selectedTexture = "";
    this->selectedOrigin = "";
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
    this->prop.clear();
    this->environment.clear();
    this->unit.clear();
    this->merchant.clear();
    this->item.clear();
    return true;
}
bool Palette::loadPalettes()
{
    this->unloadPalettes();
    this->terrain = this->loadFileLists("textures/terrain");
    this->prop = this->loadFileLists("textures/prop");
    this->environment = this->loadFileLists("textures/environment");
    this->unit = this->loadFileLists("characters");
    this->merchant = this->loadFileLists("merchants/stores");
    this->item = this->loadFileLists("items");
    this->portal = {"spawner", "level", "generator", "wall"};

    this->selectPalette(this->type);

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

bool Palette::loadPaletteItemList(std::list<std::string>& list, std::string field)
{
    int x = 0, y = 0, xLimit = 4, pageLimit = 32, index = 0, count = pageLimit * this->pageIndex, countLimit = pageLimit * (this->pageIndex + 1);
    sf::Vector2f initialPosition(1650.f, 325.f);

    for (auto& filename : list)
    {
        index++;

        if (index < count)
            continue;

        std::shared_ptr<Model> model = nullptr;
        sf::Vector2f position(initialPosition.x + x * 64.f, initialPosition.y + y * 64.f);

        switch (this->type)
        {
            case (PaletteType::ptUnit): case (PaletteType::ptMerchant): case (PaletteType::ptItem):
            {
                model = this->loadPaletteItemModel(filename, position);
                break;
            }

            case (PaletteType::ptPortal):
            {
                model = std::make_shared<Model>(this->manager, position, "", 2, true, "", filename);
                
                this->manager->palette->loadPaletteShape(model, filename);

                break;
            }

            default:
            {
                std::string filenameComplete = field + "/" + filename;
                model = std::make_shared<Model>(this->manager, position, "textures/" + filenameComplete, 2, true, "", filenameComplete);
            }
        }

        if (model != nullptr)
        {
            if (model->sprite)
                model->sprite->setScale(sf::Vector2f(64.f / model->sprite->getGlobalBounds().width, 64.f / model->sprite->getGlobalBounds().height));
            this->manager->addView(std::static_pointer_cast<ViewElement>(model));
            this->paletteItems.emplace_back(PaletteItem{ model, filename });
        }

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

    return true;
}

bool Palette::loadPaletteShape(std::shared_ptr<Model> model, std::string filename, sf::Vector2f size)
{
    if (filename == "spawner")
        model->loadShape(sf::Vector2f(32.f, 0), sf::Color(200, 255, 0, 100));
    if (filename == "level")
    {
        if (size.x <= 0.f && size.y <= 0.f)
            model->loadShape(sf::Vector2f(32.f, 0), sf::Color(50, 50, 255, 100));
        else
            model->loadShape(size, sf::Color(50, 50, 255, 100));
    }   
    if (filename == "generator")
        model->loadShape(sf::Vector2f(32.f, 0), sf::Color(255, 0, 0, 100));
    if (filename == "wall")
    {
        if (size.x <= 0.f && size.y <= 0.f)
            model->loadShape(sf::Vector2f(32.f, 0), sf::Color(255, 255, 255, 100));
        else
            model->loadShape(size, sf::Color(255, 255, 255, 100));
    }

    return true;
}

std::shared_ptr<Model> Palette::loadPaletteItemModel(std::string filename, sf::Vector2f position)
{
    switch (this->type)
    {
        case (PaletteType::ptUnit):
        {   
            json file = Json::loadFromFile("data/characters/" + filename + ".json");

            std::string texture = Json::getString(file.value("texture", ""));
            file.clear();

            if (texture == "")
                return nullptr;

            return std::make_shared<Model>(this->manager, position, "textures/" + texture, 2, true, "", "characters/" + filename);

            break;
        }

        case (PaletteType::ptMerchant):
        {
            json file = Json::loadFromFile("data/merchants/stores/" + filename + ".json");

            std::string texture = Json::getString(file["models"][0].value("value", ""));
            file.clear();

            if (texture == "")
                return nullptr;

            return std::make_shared<Model>(this->manager, position, "textures/" + texture, 2, true, "", "merchants/stores/" + filename);

            break;
        }

        case (PaletteType::ptItem):
        { 
            json file = Json::loadFromFile("data/items/" + filename + ".json");

            std::string texture = Json::getString(file.value("texture", ""));
            file.clear();

            if (texture == "")
                return nullptr;

            return std::make_shared<Model>(this->manager, position, "textures/" + texture, 2, true, "", "items/" + filename);

            break;
        }
    }

    return nullptr;
}

bool Palette::selectPalette(PaletteType type)
{
    this->clearPaletteItems();
    this->clearPaletteItem();
    this->type = type;
    
    int x = 0, y = 0, xLimit = 4, pageLimit = 32, index = 0, count = pageLimit * this->pageIndex, countLimit = pageLimit * (this->pageIndex + 1);
    sf::Vector2f initialPosition(1650.f, 325.f);

    switch (this->type)
    {
        case (PaletteType::ptTerrain):
        {
            this->loadPaletteItemList(this->terrain, "terrain");
            this->manager->hud->updateExtraEditsValue({ "Allow Teleport", "Background" }, { EditType::etBoolean, EditType::etBoolean }, { "true", "false" }, { 5, 5 });
            break;
        }
        case (PaletteType::ptProp):
        {
            this->loadPaletteItemList(this->prop, "prop");
            this->manager->hud->updateExtraEditsValue({}, {}, {}, {});
            break;
        }
        case (PaletteType::ptEnvironment):
        {
            this->loadPaletteItemList(this->environment, "environment");
            this->manager->hud->updateExtraEditsValue({}, {}, {}, {});
            break;
        }
        case (PaletteType::ptUnit):
        {
            this->loadPaletteItemList(this->unit, "unit");
            this->manager->hud->updateExtraEditsValue({"Alliance"}, {EditType::etString}, {"enemy"}, {10});
            break;
        }
        case (PaletteType::ptMerchant):
        {
            this->loadPaletteItemList(this->merchant, "merchant");
            this->manager->hud->updateExtraEditsValue({}, {}, {}, {});
            break;
        }

        case (PaletteType::ptPortal):
        {
            this->loadPaletteItemList(this->portal, "portal");
            this->manager->hud->updateExtraEditsValue({}, {}, {}, {});
            break;
        }

        case (PaletteType::ptItem):
        {
            this->loadPaletteItemList(this->item, "item");
            this->manager->hud->updateExtraEditsValue({}, {}, {}, {});
            break;
        }
    }

    if (this->pageIndex > 0 && this->paletteItems.size() <= 0)
    {
        this->pageIndex--;
        this->selectPalette(this->type);
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
    this->selectedTexture = "";
    this->selectedOrigin = "";
    this->manager->hud->shapeHover->visible = false;
    for (auto& item : this->paletteItems)
        if (item.model->sprite)
            item.model->setColor(sf::Color(255, 255, 255, 255));

    return true;
}

bool Palette::selectPaletteItem(sf::Vector2f cursor)
{
    std::string filename = "";
    for (auto& item : this->paletteItems)
        if (item.model->getGlobalBounds().contains(cursor))
        {
            if (item.model->sprite)
                item.model->setColor(sf::Color(255,0,0,255));
            filename = item.filename;
            this->selectedItem = item.filename;
            this->selectedTexture = item.model->filename;
            this->selectedOrigin = item.model->origin;
            this->manager->hud->hoverShapeSize = sf::Vector2f(item.model->getGlobalBounds().width / item.model->getScale().x,
                                                              item.model->getGlobalBounds().height / item.model->getScale().y);
            this->manager->hud->updateHoverShapeSize();
            this->manager->hud->shapeHover->visible = true;
            this->status = PaletteStatus::psInsert;
            break;
        }

    for (auto& item : this->paletteItems)
        if (filename != item.filename && item.model->sprite)
            item.model->setColor(sf::Color(255, 255, 255, 255));

    if (this->type == PaletteType::ptPortal)
    {
        if (filename == "spawner")
            this->manager->hud->updateExtraEditsValue({ "Default", "index" }, { EditType::etInteger, EditType::etInteger }, { "1", "0" }, { 1, 32 });
        else if (filename == "level")
            this->manager->hud->updateExtraEditsValue({ "Group", "Index", "Target Index", "Width", "Height", "Map"}, 
                                                      { EditType::etInteger, EditType::etInteger, EditType::etInteger, EditType::etInteger, EditType::etInteger, EditType::etString },
                                                      { "1", "1", "1", "100", "100", "" }, { 99, 99, 99, 999, 999, 255 });
        else if (filename == "generator")
            this->manager->hud->updateExtraEditsValue({"Alliance", "Index", "Target X", "Target Y", "Cooldown", "Unit" },
                { EditType::etString, EditType::etInteger, EditType::etInteger, EditType::etInteger, EditType::etInteger, EditType::etString },
                { "enemy", "1", "0", "0", "5", "" }, {12, 99, 99999, 99999, 9999, 255});
        else if (filename == "wall")
            this->manager->hud->updateExtraEditsValue({ "Width", "Height" },
                { EditType::etInteger, EditType::etInteger },
                { "100", "100" }, { 99999, 99999 });
    }

    return true;
}

std::list<std::string> Palette::loadFileLists(std::string directory)
{
   return this->loadFileFromDirectory(directory);
}

std::list<std::string> Palette::loadFileFromDirectory(std::string directory, std::string base)
{
    boost::filesystem::path path = directory;
    if (base == "")
        path = boost::filesystem::current_path() /= "data/" + directory;
    else
        base += "\\";

    std::list<std::string> listFiles = {};

    for (auto& entry : boost::make_iterator_range(boost::filesystem::directory_iterator(path), {}))
        if (boost::filesystem::is_directory(entry))
        {
            std::ostringstream stringStreamDirectory, stringStreamName;
            stringStreamDirectory << entry;
            stringStreamName << entry.path().filename();
            std::string directory = this->getString(stringStreamDirectory.str()), name = this->getString(stringStreamName.str());
            boost::algorithm::replace_all(directory, "\\", "/");
            std::list<std::string> subListFiles = this->loadFileFromDirectory(directory, name);
            if (subListFiles.size() > 0)
                listFiles.insert(listFiles.end(), subListFiles.begin(), subListFiles.end());
        }
        else if (boost::filesystem::extension(entry) == ".json")
            listFiles.emplace_back(base + boost::filesystem::basename(entry));

    return listFiles;
}

std::string Palette::getString(std::string value)
{
    boost::erase_all(value, "\"");
    return value;
}
