#include "button.hpp"
#include "../manager.hpp"

Button::Button(Manager* manager, std::string caption, sf::Vector2f position, int size)
{
	this->text = std::make_shared<Text>(manager, caption, size, position);
	this->shape = std::make_shared<Model>(manager, sf::Vector2f(position.x - 5.f, position.y));
	this->shape->loadShape(sf::Vector2f(this->text->text->getGlobalBounds().width + 10.f, this->text->text->getGlobalBounds().height + 10.f), sf::Color(150, 150, 150, 100));

	manager->addView(ObjectType::otText, std::static_pointer_cast<ViewElement>(this->text));
	manager->addView(ObjectType::otModel, std::static_pointer_cast<ViewElement>(this->shape));
}

Button::~Button()
{

}