#include "system/manager.hpp"
#include <string>
#include <iostream>
#include <exception>

#ifdef _WIN32
#include <windows.h>
#endif

// Shows a startup/runtime error to the user instead of letting the process
// die silently (which looked like the editor "opening and closing" on its
// own — the game hides its window while the editor runs). On Windows the
// message is shown in a message box; on other platforms it goes to stderr.
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
        // Command-line switches used when the editor is launched as an in-game
        // tool by Grimsolf (Studio -> World Editor):
        //   --no-splash           skip the initial splash screen
        //   --game-path <path>    open the editor directly on that game folder
        //                         (skips the "Select game folder" dialog)
        bool noSplash = false;
        std::string gamePath = "";

        for (int i = 1; i < argc; ++i)
        {
            std::string arg = argv[i];
            if (arg == "--no-splash")
                noSplash = true;
            else if (arg == "--game-path" && i + 1 < argc)
                gamePath = argv[++i];
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
