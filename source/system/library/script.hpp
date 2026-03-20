#include <iostream>

#pragma once

#ifndef SCRIPT_HPP
#define SCRIPT_HPP

namespace script
{
    std::string loadFile();
    std::string saveFile();
    bool deleteFile(std::string filename);
    std::string getTemp(std::string filename);
    bool openUrl(std::string url);
}

#endif