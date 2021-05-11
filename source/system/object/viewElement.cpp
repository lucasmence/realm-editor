#include "viewElement.hpp"
ViewElement::~ViewElement()
{
	this->priority = 0;
}

bool ViewElement::draw()
{
	return true;
}