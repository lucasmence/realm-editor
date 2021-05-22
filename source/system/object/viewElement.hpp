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
		std::string name;
		bool visible;
		bool fading;
		float timeMax;
		float time;
		float timeSpeed;
		float fadeSpeed;

		virtual ~ViewElement();
		virtual bool draw();
		virtual bool setPosition(sf::Vector2f position);
		virtual sf::Vector2f getPosition();
		virtual bool reset();
		virtual bool initialization();

};

#endif