#include <boost/algorithm/string.hpp>
#include "texture.hpp"
#include "../library/json.hpp"

Texture::Texture(std::string filename, std::string jsonPath)
{
	this->filename = filename;
	this->jsonPath = jsonPath;
	this->texture.loadFromFile(filename + ".png");
	this->bitmask = this->checkBitmasking();
}

Texture::~Texture()
{

}

bool Texture::checkBitmasking()
{
	if (this->jsonPath == "") return false;

	json jsonFile = Json::loadFromFile(this->jsonPath + ".json");

	for (int index = 0; index < jsonFile["animation"].size(); index++)
	{
		std::string name = jsonFile["animation"][index].value("name", "");
		if (boost::algorithm::contains(name, ":"))
			return true;
	}

	jsonFile.clear();

	return false;
}