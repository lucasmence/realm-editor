#include "viewElement.hpp"

ViewElement::~ViewElement()
{
	this->priority = 0;
	this->autoPriority = 0;
	this->initialization();
}

bool ViewElement::draw()
{
	if (this->timeMax > 0.f && this->time < this->timeMax)
	{
		this->time += this->timeSpeed;
		this->fading = (this->time >= this->timeMax);
	}
	return true;
}

bool ViewElement::setPosition(sf::Vector2f position)
{
	this->position = position;
	return true;
}

sf::Vector2f ViewElement::getPosition()
{
	return this->position;
}

bool ViewElement::reset()
{
	this->visible = true;
	this->fading = false;
	this->time = 0.f;
	return true;
}

bool ViewElement::initialization()
{
	this->visible = true;
	this->fading = false;
	this->timeMax = 0.f;
	this->time = 0.f;
	this->timeSpeed = 0.01f;
	this->fadeSpeed = 1.f;

	return true;
}