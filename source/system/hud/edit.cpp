#include <boost/lexical_cast.hpp>
#include "edit.hpp"
#include "../manager.hpp"
#include "../library/position.hpp"

Edit::Edit(Manager* manager,
	       EditType type,
		   std::string caption, 
		   sf::Vector2f position, 
		   std::string name, 
		   int size,
		   sf::FloatRect neighbor,
		   sf::Vector2i side)
{
	this->type = type;
	this->value = EditValue{ "", 0 };
	this->maxLength = 0;
	this->integerMaxValue = 0;
	this->integerMinValue = 0;
	this->selected = false;
	this->manager = manager;
	this->name = name;
	this->label = std::make_shared<Label>(manager, caption, size, position);
	this->shape = std::make_shared<Model>(manager, sf::Vector2f(position.x - 5.f, position.y));
	this->shape->loadShape(sf::Vector2f(this->label->text->getGlobalBounds().width + 10.f, size * 1.25f), sf::Color(150, 150, 150, 100));

	if (neighbor.width >= 0)
	{
		sf::Vector2f positionSide = position::getSidePosition(neighbor, this->shape->shape->getGlobalBounds(), position, side);

		this->shape->setPosition(positionSide);
		this->label->setPosition(sf::Vector2f(positionSide.x + 5.f, positionSide.y));		
	}

	manager->addView(std::static_pointer_cast<ViewElement>(this->label));
	manager->addView(std::static_pointer_cast<ViewElement>(this->shape));
}

Edit::~Edit()
{
	this->clear();
}

int Edit::getInt(std::string value)
{
	if (value == "")
		return 0;

	for (int index = 0; index < value.length(); index++)
		if (!isdigit(value[index]))
			return -1;

	int valueInteger = boost::lexical_cast<int>(value);
	if (this->integerMinValue > valueInteger)
		valueInteger = this->integerMinValue;
	else if (this->integerMaxValue < valueInteger && this->integerMaxValue > 0)
		valueInteger = this->integerMaxValue;

	return valueInteger;
}
bool Edit::setValue(std::string value)
{
	if (this->maxLength > 0 && value.length() > this->maxLength)
		return false;

	switch (this->type)
	{
		case EditType::etString:
		{
			this->updateLabel(value);
			this->value.string = value;
			break;
		}
		case EditType::etInteger:
		{
			if (value == "")
				value = "0";
			int valueInteger = this->getInt(value);
			if (valueInteger == -1)
				return false;
			value = boost::lexical_cast<std::string>(valueInteger);
			this->updateLabel(value);
			this->value.string = value;
			this->value.integer = valueInteger;
			break;
		}
	}
	return true;
}
EditValue Edit::getValue()
{
	return this->value;
}

bool Edit::updateLabel(std::string value)
{
	this->label->text->setString("<" + value + ">");
	std::static_pointer_cast<sf::RectangleShape>(this->shape->shape)->setSize(sf::Vector2f(this->label->text->getGlobalBounds().width + 10.f, this->shape->shape->getGlobalBounds().height));
	return true;
}

bool Edit::clear()
{
	this->manager->removeView(std::static_pointer_cast<ViewElement>(this->label));
	this->manager->removeView(std::static_pointer_cast<ViewElement>(this->shape));
	this->label = nullptr;
	this->shape = nullptr;
	return true;
}