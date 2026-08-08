#include "system/manager.hpp"
#include <string>

int main(int argc, char* argv[])
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
