#include <SFML/Graphics.hpp>
#include <memory>
#include "../model/model.hpp"
#include "../text/label.hpp"

#pragma once

#ifndef BUTTON_HPP
#define BUTTON_HPP

class Manager;

class Button 
{
	public:
		Manager* manager;
		std::shared_ptr<Model> shape;
		std::shared_ptr<Label> label;
		std::string name;
		bool selected;

		Button(Manager* manager, 
			   std::string caption, 
			   sf::Vector2f position, 
			   std::string name, 
			   int size = 20, 
			   std::shared_ptr<Button> neighbor = nullptr, 
			   sf::Vector2i side = sf::Vector2i(0, 0));
		~Button();
		bool clear();

};

#endif