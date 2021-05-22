#include <SFML/Graphics.hpp>
#include <memory>
#include "../object/viewElement.hpp"

#pragma once

#ifndef LABEL_HPP
#define LABEL_HPP

class Manager;

class Label : public ViewElement
{
	public:
		Manager* manager;
		std::shared_ptr<sf::Text> text;

		Label(Manager* manager, 
			  std::string caption, 
			  int size, 
			  sf::Vector2f position, 
			  int priority = 1, 
			  sf::Color color = sf::Color(255, 255, 255, 255), 
			  std::string name = "", 
			  bool canvasBound = true);

		~Label();

		bool setPosition(sf::Vector2f position);
		bool draw();
		bool reset();
};

#endif