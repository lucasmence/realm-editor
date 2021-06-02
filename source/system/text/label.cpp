#include "label.hpp"
#include "../manager.hpp"

Label::Label(Manager* manager, 
			 std::string caption, 
			 int size, 
			 sf::Vector2f position, 
			 int priority, 
			 sf::Color color, 
			 std::string name, 
			 bool canvasBound)
{
	this->manager = manager;
    this->text = std::make_shared<sf::Text>();
	this->text->setFont(*this->manager->font);
    this->text->setFillColor(color);
    this->text->setCharacterSize(size);
	this->text->setString(caption);
	this->text->setPosition(position);
	this->position = position;
	this->priority = priority;
	this->name = name;
	this->canvasBound = canvasBound;
	this->elementType = ElementType::etLabel;
}

Label::~Label()
{

}

bool Label::draw()
{
	if (!this->visible)
		return ViewElement::draw();

	if (this->canvasBound)
		this->text->setPosition(this->manager->canvasPosition.x + this->position.x, this->manager->canvasPosition.y + this->position.y);
	
	if (this->fading)
	{
		this->text->setFillColor(sf::Color(this->text->getFillColor().r, this->text->getFillColor().g, this->text->getFillColor().b, this->text->getFillColor().a - this->fadeSpeed));
		this->visible = (this->text->getFillColor().a > 0);
	}
		
	this->manager->window->draw(*this->text);
	return ViewElement::draw();
}

bool Label::setPosition(sf::Vector2f position)
{
	ViewElement::setPosition(position);
	if (this->text)
		this->text->setPosition(position);
	return true;
}

bool Label::reset()
{
	this->text->setFillColor(sf::Color(this->text->getFillColor().r, this->text->getFillColor().g, this->text->getFillColor().b, 255));
	return ViewElement::reset();
}