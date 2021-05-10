#include "texture.hpp"

Texture::Texture(std::string filename)
{
	this->filename = filename;
	this->texture = std::shared_ptr<sf::Texture>(new sf::Texture);
	this->texture->loadFromFile("resources/sprites/" + filename + ".png");
}

Texture::~Texture()
{

}