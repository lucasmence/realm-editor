#include "model.hpp"
#include "../library/json.hpp"
#include "../manager.hpp"

Model::Model(Manager* manager, std::string filename)
{
	this->manager = manager;

	if (filename != "")
		this->loadSprite(filename);
}

Model::~Model()
{

}

bool Model::draw()
{
	this->manager->window->draw(*this->sprite);
	return ViewElement::draw();
}

bool Model::loadSprite(std::string filename)
{
	json jsonFile = Json::loadFromFile("data/" + filename + ".json");

	sf::Vector2i dimension(0, 0);
	for (int index = 0; index < jsonFile["animation"].size(); index++)
	{
		dimension = sf::Vector2i(jsonFile["animation"][index].value("sprite-direction-width", 0), jsonFile["animation"][index].value("sprite-direction-height", 0));
		break;
	}

	this->texture = std::make_shared<Texture>(jsonFile.value("texturename", ""));
	this->sprite = std::make_shared<sf::Sprite>();
	this->sprite->setTexture(*this->texture->texture);
	this->sprite->setTextureRect(sf::IntRect(0, 0, dimension.x, dimension.y));
	this->sprite->setPosition(0.f, 0.f);

	return true;
}

bool Model::loadShape(sf::Vector2f size, sf::Color color)
{
	return true;
}