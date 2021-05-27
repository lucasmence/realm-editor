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
		std::string filename;
		std::string origin;

		Model(Manager* manager, sf::Vector2f position, std::string filename = "", int priority = 2, bool canvasBound = true, std::string name = "", std::string origin = "");
		~Model();

		bool loadSprite(std::string filename, sf::Vector2f position);
		bool loadShape(sf::Vector2f size, sf::Color color);
		bool setPosition(sf::Vector2f position);
		virtual bool draw();
		virtual bool reset();

};

#endif