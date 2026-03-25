#include "json.hpp"
#include <fstream>
#include <sstream>
#include <algorithm>

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

        json jsonFile;
        fileStream >> jsonFile;

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