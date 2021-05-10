#include <SFML/Graphics.hpp>
#include <memory>

#pragma once

#ifndef TEXTURE_HPP
#define TEXTURE_HPP

class Texture 
{
	public:
		std::shared_ptr<sf::Texture> texture;
		std::string filename;

		Texture(std::string filename);
		~Texture();

};

#endif