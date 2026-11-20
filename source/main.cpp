#include "system/manager.hpp"
#include "system/spriteFusion/spriteFusionApp.hpp"
#include "system/trigger/triggerApp.hpp"
#include <string>
#include <iostream>
#include <exception>
#include <boost/filesystem.hpp>

#ifdef _WIN32
#include <windows.h>
#endif

static void reportError(const std::string &message)
{
    std::cerr << "realm-editor error: " << message << std::endl;
#ifdef _WIN32
    MessageBoxA(nullptr, message.c_str(), "realm-editor error", MB_OK | MB_ICONERROR);
#endif
}

int main(int argc, char* argv[])
{
    try
    {
        bool noSplash = false;
        bool triggerEditorMode = false;
        bool spriteFusionMode = false;
        std::string gamePath = "";
        std::string hudTheme = "";

        for (int i = 1; i < argc; ++i)
        {
            std::string arg = argv[i];
            if (arg == "--no-splash")
                noSplash = true;
            else if (arg == "--trigger-editor")
                triggerEditorMode = true;
            else if (arg == "--sprite-fusion")
                spriteFusionMode = true;
            else if (arg == "--game-path" && i + 1 < argc)
                gamePath = argv[++i];
            else if (arg == "--hud-theme" && i + 1 < argc)
                hudTheme = argv[++i];
        }

        if (triggerEditorMode)
        {
            std::string path = gamePath;
            if (path.empty())
                path = boost::filesystem::current_path().string();

            TriggerApp app(path, hudTheme);
            return app.run();
        }

        if (spriteFusionMode)
        {
            std::string path = gamePath;
            if (path.empty())
                path = boost::filesystem::current_path().string();

            SpriteFusionApp app(path, hudTheme);
            return app.run();
        }

        std::shared_ptr<Manager> manager(new Manager(noSplash, gamePath));

        while (manager->window->isOpen())
            manager->update();

        return 0;
    }
    catch (const std::exception &error)
    {
        reportError(error.what());
        return 1;
    }
    catch (...)
    {
        reportError("unknown exception");
        return 1;
    }
}
