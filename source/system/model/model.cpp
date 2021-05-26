#include "model.hpp"
#include "../library/json.hpp"
#include "../manager.hpp"

Model::Model(Manager* manager, sf::Vector2f position, std::string filename, int priority, bool canvasBound, std::string name)
{
	this->manager = manager;
	this->priority = priority;
	this->position = position;
	this->sprite = nullptr;
	this->shape = nullptr;
	this->canvasBound = canvasBound;
	this->name = name;
	this->filename = filename;

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
		if (this->fading)
		{
			this->sprite->setColor(sf::Color(this->sprite->getColor().r, this->sprite->getColor().g, this->sprite->getColor().b, this->sprite->getColor().a - this->fadeSpeed));
			this->visible = (this->sprite->getColor().a > 0);
		}
		this->manager->window->draw(*this->sprite);
	}
		
	if (this->shape)
	{
		if (this->canvasBound)
			this->shape->setPosition(this->manager->canvasPosition.x + this->position.x, this->manager->canvasPosition.y + this->position.y);
		if (this->fading)
		{
			this->shape->setFillColor(sf::Color(this->shape->getFillColor().r, this->shape->getFillColor().g, this->shape->getFillColor().b, this->shape->getFillColor().a - this->fadeSpeed));
			this->visible = (this->shape->getFillColor().a > 0);
		}
		this->manager->window->draw(*this->shape);
	}	

	return ViewElement::draw();
}

bool Model::loadSprite(std::string filename, sf::Vector2f position)
{
	json jsonFile = Json::loadFromFile("data/" + filename + ".json");
	this->filename = filename;

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

bool Model::reset()
{
	if (this->sprite)
		this->sprite->setColor(sf::Color(this->sprite->getColor().r, this->sprite->getColor().g, this->sprite->getColor().b, 255));

	if (this->shape)
		this->shape->setFillColor(sf::Color(this->shape->getFillColor().r, this->shape->getFillColor().g, this->shape->getFillColor().b, 255));

	return ViewElement::reset();
}