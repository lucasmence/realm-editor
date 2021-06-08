#include <windows.h>
#include <boost/filesystem.hpp>
#include <boost/algorithm/string.hpp>
#include <string>
#include "script.hpp"

namespace script
{
    std::string loadFile()
    {
        std::string filename = "filename.data";
        deleteFile("temp/" + filename);

        auto Command = std::string("cd scripts && cmd.exe /C openfile.bat");
        std::system(Command.c_str());

        return getTemp(filename);
    }

    std::string saveFile()
    {
        std::string filename = "savefile.data";
        deleteFile("temp/" + filename);

        auto Command = std::string("cd scripts && cmd.exe /C savefile.bat");
        std::system(Command.c_str());

        return getTemp(filename);
    }
    
    std::string getTemp(std::string filename)
    {
        std::string line = "", text = "";
        std::ifstream file("temp/" + filename);
        if (file.is_open())
        {
            while (getline(file, line))
                text += line;
            
            file.close();
        }

        deleteFile("temp/" + filename);

        return text;
    }

    bool deleteFile(std::string filename)
    {
        std::string prePath = filename;
        boost::filesystem::path path = boost::filesystem::current_path() /= prePath;
        boost::filesystem::remove(path);
        return true;
    }

    bool maximizeWindow(std::string windowName)
    {
        std::system(std::string("cd scripts && windowmode -title "+windowName+" -mode maximized").c_str());
        return true;
    }

    bool openUrl(std::string url)
    {
        std::system(std::string("start " + url).c_str());
        return true;
    }
}