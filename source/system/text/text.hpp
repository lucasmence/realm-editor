#include <SFML/Graphics.hpp>
#include <memory>
#include "../object/viewElement.hpp"

#pragma once

#ifndef TEXT_HPP
#define TEXT_HPP

class Manager;

class Text : public ViewElement
{
	public:
		Manager* manager;
		std::shared_ptr<sf::Text> text;

		Text(Manager* manager, std::string caption, int size, sf::Vector2f position, sf::Color color = sf::Color(255, 255, 255, 255));
		~Text();

		bool draw();
};

#endif