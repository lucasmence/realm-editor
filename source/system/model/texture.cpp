#include "texture.hpp"

Texture::Texture(std::string filename)
{
	this->filename = filename;
	this->texture.loadFromFile("resources/sprites/" + filename + ".png");
}

Texture::~Texture()
{

}