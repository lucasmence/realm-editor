#include <memory>
#include "model.hpp"
#include "../library/json.hpp"
#include "../manager.hpp"

Model::Model(Manager* manager, sf::Vector2f position, std::string filename, int priority, bool canvasBound, std::string name, std::string origin)
{
	this->manager = manager;
	this->priority = priority;
	this->position = position;
	this->sprite = nullptr;
	this->shape = nullptr;
	this->canvasBound = canvasBound;
	this->name = name;
	this->filename = filename;
	this->origin = origin;
	this->shapeType = ShapeType::stRectangle;
	this->elementType = ElementType::etModel;

	if (filename != "")
		this->loadSprite(filename, position);
}

Model::~Model()
{

}

bool Model::draw()
{
	if (!this->visible || 
		(this->canvasBound && this->manager->hud->zoom != 1.f) || 
		(this->canvasBound && this->manager->minimapViewUpdate))
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
			this->shape->setPosition(this->manager->canvasPosition.x + this->position.x, 
									 this->manager->canvasPosition.y + this->position.y);
			
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
	this->filename = filename;
	bool textureFound = false;
	sf::Vector2i dimension(0, 0);
	std::shared_ptr<Model> usedModel = nullptr;
		
	if (!usedModel)
		for (auto& modelIndex : this->manager->list.viewElements)
			if (modelIndex->elementType == this->elementType)
				if (std::dynamic_pointer_cast<Model>(modelIndex)->filename == this->filename)
				{
					usedModel = std::dynamic_pointer_cast<Model>(modelIndex);
					break;
				}

	if (usedModel)
	{
		this->texture = usedModel->texture;
		sf::IntRect textureRect = usedModel->sprite->getTextureRect();
		dimension = sf::Vector2i(textureRect.width, textureRect.height);
		textureFound = true;
	}

	if (!textureFound)
	{ 
		std::string textureName = "";
		json jsonFile = Json::loadFromFile("data/" + filename + ".json");
		
		for (int index = 0; index < jsonFile["animation"].size(); index++)
		{
			dimension = sf::Vector2i(jsonFile["animation"][index].value("sprite-direction-width", 0), jsonFile["animation"][index].value("sprite-direction-height", 0));
			break;
		}

		textureName = jsonFile.value("texturename", "");
		this->texture = this->manager->getTexture(textureName);
	}
	
	this->sprite = std::make_shared<sf::Sprite>();
	this->sprite->setTexture(this->texture->texture);
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
		this->shapeType = ShapeType::stRectangle;
		return true;
	}
	else if (size.x > 0.f && size.y <= 0.f)
	{
		this->shape = std::make_shared<sf::CircleShape>(size.x);
		this->shape->setFillColor(color);
		this->shape->setPosition(this->position);
		this->shapeType = ShapeType::stCircle;
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

sf::Vector2f Model::getPosition()
{
	if (this->sprite)
		return this->sprite->getPosition();
	if (this->shape)
		return this->shape->getPosition();

	return sf::Vector2f(0.f, 0.f);
}

sf::Vector2f Model::getScale()
{
	if (this->sprite)
		return this->sprite->getScale();
	if (this->shape)
		return this->shape->getScale();

	return sf::Vector2f(0.f, 0.f);
}

float Model::getRotation()
{
	if (this->sprite)
		return this->sprite->getRotation();
	if (this->shape)
		return this->shape->getRotation();

	return 0;
}

sf::FloatRect Model::getGlobalBounds()
{
	if (this->sprite)
		return this->sprite->getGlobalBounds();
	if (this->shape)
		return this->shape->getGlobalBounds();

	return sf::FloatRect(0.f, 0.f, 0.f, 0.f);
}

bool Model::setColor(sf::Color color)
{
	if (this->sprite)
		this->sprite->setColor(color);
	if (this->shape)
		this->shape->setFillColor(color);
	return true;
}

bool Model::setOrigin(sf::Vector2f position)
{
	if (this->sprite)
		this->sprite->setOrigin(position);
	if (this->shape)
		this->shape->setOrigin(position);
	return true;
}