#include <windows.h>
#include <boost/filesystem.hpp>
#include <boost/algorithm/string.hpp>
#include <string>
#include "script.hpp"

namespace script
{
    bool openUrl(std::string url)
    {
        std::system(std::string("start " + url).c_str());
        return true;
    }
}