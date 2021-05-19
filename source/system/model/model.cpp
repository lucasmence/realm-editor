#include "model.hpp"
#include "../library/json.hpp"
#include "../manager.hpp"

Model::Model(Manager* manager, sf::Vector2f position, std::string filename, int priority, bool canvasBound)
{
	this->manager = manager;
	this->priority = priority;
	this->position = position;
	this->sprite = nullptr;
	this->shape = nullptr;
	this->visible = true;
	this->canvasBound = canvasBound;

	if (filename != "")
		this->loadSprite(filename, position);
}

Model::~Model()
{

}

bool Model::draw()
{
	if (!this->visible)
		return ViewElement::draw();

	if (this->sprite)
	{
		if (this->canvasBound)
			this->sprite->setPosition(this->manager->canvasPosition.x + this->position.x, this->manager->canvasPosition.y + this->position.y);
		this->manager->window->draw(*this->sprite);
	}
		
	if (this->shape)
	{
		if (this->canvasBound)
			this->shape->setPosition(this->manager->canvasPosition.x + this->position.x, this->manager->canvasPosition.y + this->position.y);
		this->manager->window->draw(*this->shape);
	}	

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

bool Model::setPosition(sf::Vector2f position)
{
	ViewElement::setPosition(position);
	if (this->sprite)
		this->sprite->setPosition(position);

	if (this->shape)
		this->shape->setPosition(position);
	return true;
}