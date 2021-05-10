#include "text.hpp"
#include "../manager.hpp"

Text::Text(Manager* manager, std::string caption, int size, sf::Vector2f position, sf::Color color)
{
	this->manager = manager;
    this->text = std::make_shared<sf::Text>();
	this->text->setFont(*this->manager->font);
    this->text->setFillColor(color);
    this->text->setCharacterSize(size);
	this->text->setString(caption);
	this->text->setPosition(position);
}

Text::~Text()
{

}

bool Text::draw()
{
	this->manager->window->draw(*this->text);
	return ViewElement::draw();
}