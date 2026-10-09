#include "json.hpp"
#include "../external/fkYAML/fkYAML.hpp"
#include <fstream>
#include <sstream>
#include <algorithm>
#include <cctype>

namespace Json
{
    void fixPath(std::string& path) 
    {
        std::replace(path.begin(), path.end(), '\\', '/');
    }
    
json loadFromFile(std::string filename)
{
    fixPath(filename);
    std::ifstream fileStream(filename);

    if (!fileStream.is_open())
        return json();

    json jsonFile;
    try
    {
        fileStream >> jsonFile;
    }
    catch (...)
    {
        return json();
    }

    return jsonFile;
}

    std::string getString(std::string value)
    { 
        boost::erase_all(value, "\"");
        return value;
    }

    std::string getValueFromList(json file, std::string field, int index)
    {
        if (index >= 0 && file.size() > index)
            return file[index].value(field, "");

        index = rand() % file.size();
        return file[index].value(field, "");
    }

    std::string convertPathToString(boost::filesystem::path path)
    {
        std::ostringstream stringStream;
        stringStream << path;
        std::string result = getString(stringStream.str());
        boost::algorithm::replace_all(result, "\\", "/");
        return result;
    }
}

namespace Json
{
    namespace
    {
        json yamlNodeToJson(const fkyaml::node &node)
        {
            if (node.is_mapping())
            {
                json object = json::object();
                for (const auto &entry : node.as_map())
                {
                    if (!entry.first.is_string())
                        throw std::runtime_error("YAML mapping keys must be strings");
                    object[entry.first.as_str()] = yamlNodeToJson(entry.second);
                }
                return object;
            }

            if (node.is_sequence())
            {
                json array = json::array();
                for (const auto &item : node.as_seq())
                    array.push_back(yamlNodeToJson(item));
                return array;
            }

            if (node.is_boolean())
                return node.as_bool();
            if (node.is_integer())
                return node.as_int();
            if (node.is_float_number())
                return node.as_float();
            if (node.is_string())
                return node.as_str();

            return nullptr;
        }

        // Keys stay bare only when they cannot be mistaken for another YAML type.
        bool isPlainKey(const std::string &key)
        {
            if (key.empty() || key == "true" || key == "false" || key == "null"
                || key == "yes" || key == "no" || key == "on" || key == "off")
                return false;

            if (!(std::isalpha((unsigned char)key[0]) || key[0] == '_'))
                return false;

            for (char c : key)
                if (!(std::isalnum((unsigned char)c) || c == '_' || c == '-'))
                    return false;

            return true;
        }

        bool isEmptyContainer(const json &value)
        {
            return (value.is_object() || value.is_array()) && value.empty();
        }

        // Scalars use JSON syntax, which YAML accepts as double-quoted strings,
        // numbers, booleans and null.
        std::string scalarText(const json &value)
        {
            return value.dump();
        }

        // Block-style emitter: returns the lines of `value` indented by `indent`.
        std::vector<std::string> emitBlock(const json &value, int indent)
        {
            std::vector<std::string> lines;
            std::string pad(indent, ' ');

            if (value.is_object())
            {
                for (auto it = value.begin(); it != value.end(); ++it)
                {
                    std::string key = isPlainKey(it.key()) ? it.key() : json(it.key()).dump();

                    if (it->is_object() || it->is_array())
                    {
                        if (isEmptyContainer(*it))
                        {
                            lines.push_back(pad + key + (it->is_object() ? ": {}" : ": []"));
                        }
                        else
                        {
                            lines.push_back(pad + key + ":");
                            for (auto &line : emitBlock(*it, indent + 2))
                                lines.push_back(line);
                        }
                    }
                    else
                    {
                        lines.push_back(pad + key + ": " + scalarText(*it));
                    }
                }
            }
            else if (value.is_array())
            {
                for (const auto &item : value)
                {
                    if (item.is_object() || item.is_array())
                    {
                        if (isEmptyContainer(item))
                        {
                            lines.push_back(pad + (item.is_object() ? "- {}" : "- []"));
                            continue;
                        }

                        std::vector<std::string> nested = emitBlock(item, indent + 2);
                        // The first line of the nested block shares the "- " marker.
                        nested[0] = pad + "- " + nested[0].substr(indent + 2);
                        for (auto &line : nested)
                            lines.push_back(line);
                    }
                    else
                    {
                        lines.push_back(pad + "- " + scalarText(item));
                    }
                }
            }
            else
            {
                lines.push_back(pad + scalarText(value));
            }

            return lines;
        }
    }

    json parseYaml(const std::string &text)
    {
        return yamlNodeToJson(fkyaml::node::deserialize(text));
    }

    std::string dumpYaml(const json &value)
    {
        if (isEmptyContainer(value))
            return (value.is_object() ? std::string("{}") : std::string("[]")) + "\n";

        std::string out;
        for (const auto &line : emitBlock(value, 0))
            out += line + "\n";
        return out;
    }
}

