#include <iostream>
#include <fstream>
#include <string>
#include <boost/algorithm/string.hpp>
#include <boost/filesystem.hpp>
#include "../external/nlohmann/json.hpp"

#pragma once

#ifndef JSON_HPP
#define JSON_HPP

using json = nlohmann::json;

namespace Json
{
    json loadFromFile(std::string filename);
    std::string getString(std::string value);
    std::string getValueFromList(json file, std::string field, int index = -1);
    std::string convertPathToString(boost::filesystem::path path);
}

#endif