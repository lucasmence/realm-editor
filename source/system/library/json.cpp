#include "json.hpp"

namespace Json
{
    json loadFromFile(std::string filename)
    {
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
}