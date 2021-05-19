#include "viewElement.hpp"
ViewElement::~ViewElement()
{
	this->priority = 0;
}

bool ViewElement::draw()
{
	return true;
}

bool ViewElement::setPosition(sf::Vector2f position)
{
	this->position = position;
	return true;
}