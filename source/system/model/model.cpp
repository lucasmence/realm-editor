#include "model.hpp"
#include "../library/json.hpp"
#include "../manager.hpp"

Model::Model(Manager* manager, sf::Vector2f position, std::string filename, int priority)
{
	this->manager = manager;
	this->priority = priority;
	this->position = position;
	this->sprite = nullptr;
	this->shape = nullptr;

	if (filename != "")
		this->loadSprite(filename, position);
}

Model::~Model()
{

}

bool Model::draw()
{
	if (this->sprite)
		this->manager->window->draw(*this->sprite);
	if (this->shape)
		this->manager->window->draw(*this->shape);
	return ViewElement::draw();
}

bool Model::loadSprite(std::string filename, sf::Vector2f position)
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
	this->sprite->setPosition(position);

	return true;
}

bool Model::loadShape(sf::Vector2f size, sf::Color color)
{
	if (size.x > 0.f && size.y > 0.f)
	{
		this->shape = std::make_shared<sf::RectangleShape>(size);
		this->shape->setFillColor(color);
		this->shape->setPosition(this->position);
		return true;
	}
	else if (size.x > 0.f && size.y <= 0.f)
	{
		this->shape = std::make_shared<sf::CircleShape>(size.x);
		this->shape->setFillColor(color);
		this->shape->setPosition(this->position);
		return true;
	}
	return false;
}