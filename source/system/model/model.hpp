#include <SFML/Graphics.hpp>
#include <memory>
#include "texture.hpp"
#include "../object/viewElement.hpp"

#pragma once

#ifndef MODEL_HPP
#define MODEL_HPP

class Manager;

class Model : public ViewElement
{
	public:
		Manager* manager;
		std::shared_ptr<sf::Sprite> sprite;
		std::shared_ptr<sf::Shape> shape;
		std::shared_ptr<Texture> texture;

		Model(Manager* manager, sf::Vector2f position, std::string filename = "", int priority = 2);
		~Model();

		bool loadSprite(std::string filename, sf::Vector2f position);
		bool loadShape(sf::Vector2f size, sf::Color color);
		virtual bool draw();

};

#endif