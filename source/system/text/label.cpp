#include "label.hpp"
#include "../manager.hpp"

Label::Label(Manager* manager, std::string caption, int size, sf::Vector2f position, int priority, sf::Color color, std::string name)
{
	this->manager = manager;
    this->text = std::make_shared<sf::Text>();
	this->text->setFont(*this->manager->font);
    this->text->setFillColor(color);
    this->text->setCharacterSize(size);
	this->text->setString(caption);
	this->text->setPosition(position);
	this->priority = priority;
	this->name = name;
}

Label::~Label()
{

}

bool Label::draw()
{
	this->manager->window->draw(*this->text);
	return ViewElement::draw();
}