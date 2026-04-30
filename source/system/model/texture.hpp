#include <SFML/Graphics.hpp>
#include <memory>

#pragma once

#ifndef TEXTURE_HPP
#define TEXTURE_HPP

class Texture 
{
	public:
		sf::Texture texture;
		std::string filename;
		std::string jsonPath;
		bool bitmask;

		Texture(std::string filename, std::string jsonPath = "");
		~Texture();

		bool checkBitmasking();
};

#endif