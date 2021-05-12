#include <SFML/Graphics.hpp>
#include <memory>
#include "../model/model.hpp"
#include "../text/text.hpp"

#pragma once

#ifndef BUTTON_HPP
#define BUTTON_HPP

class Manager;

class Button 
{
	public:
		std::shared_ptr<Model> shape;
		std::shared_ptr<Text> text;

		Button(Manager* manager, std::string caption, sf::Vector2f position, int size = 20);
		~Button();

};

#endif