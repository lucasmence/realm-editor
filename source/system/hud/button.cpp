#include "button.hpp"
#include "../manager.hpp"
#include "../library/position.hpp"

Button::Button(Manager* manager, 
			   std::string caption, 
			   sf::Vector2f position, 
			   std::string name, 
			   int size,
			   std::shared_ptr<Button> neighbor,
			   sf::Vector2i side)
{
	this->selected = false;
	this->manager = manager;
	this->name = name;
	this->label = std::make_shared<Label>(manager, caption, size, position);
	this->shape = std::make_shared<Model>(manager, sf::Vector2f(position.x - 5.f, position.y));
	this->shape->loadShape(sf::Vector2f(this->label->text->getGlobalBounds().width + 10.f, size * 1.25f), sf::Color(150, 150, 150, 100));

	if (neighbor)
	{
		sf::Vector2f positionSide = position::getSidePosition(neighbor->shape->shape->getGlobalBounds(), this->shape->shape->getGlobalBounds(), position, side);

		this->shape->setPosition(positionSide);
		this->label->setPosition(sf::Vector2f(positionSide.x + 5.f, positionSide.y));		
	}

	manager->addView(std::static_pointer_cast<ViewElement>(this->label));
	manager->addView(std::static_pointer_cast<ViewElement>(this->shape));
}

Button::~Button()
{
	this->clear();
}

bool Button::clear()
{
	this->manager->removeView(std::static_pointer_cast<ViewElement>(this->label));
	this->manager->removeView(std::static_pointer_cast<ViewElement>(this->shape));
	this->label = nullptr;
	this->shape = nullptr;
	return true;
}