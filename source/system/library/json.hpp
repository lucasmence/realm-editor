#include <iostream>
#include <fstream>
#include <string>
#include <boost/algorithm/string.hpp>
#include <boost/filesystem.hpp>
#include "../external/nlohmann/json.hpp"
#include <stdexcept>

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
    // YAML <-> json. parseYaml throws std::runtime_error whose message ends with
    // "(at line L, column C)" when the text is not valid YAML.
    json parseYaml(const std::string &text);
    std::string dumpYaml(const json &value);
}

#endif