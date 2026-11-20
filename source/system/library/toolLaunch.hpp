#pragma once

#ifndef TOOLLAUNCH_HPP
#define TOOLLAUNCH_HPP

#include <string>

namespace ToolLaunch
{
    int launchTool(const std::string &gamePath, const std::string &toolFlag,
                   const std::string &hudTheme = "");

    std::string getExecutablePath();

    std::string toolName(const std::string &toolFlag);
}

#endif
