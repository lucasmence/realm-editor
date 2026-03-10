#include "texture.hpp"

Texture::Texture(std::string filename)
{
	this->filename = filename;
	this->texture.loadFromFile(filename + ".png");
}

Texture::~Texture()
{

}