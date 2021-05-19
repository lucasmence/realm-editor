#include <SFML/Graphics.hpp>

#pragma once

#ifndef VIEWELEMENT_HPP
#define VIEWELEMENT_HPP

class ViewElement
{
	public:
		int priority;
		bool canvasBound;
		sf::Vector2f position;

		virtual ~ViewElement();
		virtual bool draw();
		virtual bool setPosition(sf::Vector2f position);

};

#endif