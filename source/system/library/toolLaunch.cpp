#include "toolLaunch.hpp"

#include <boost/filesystem.hpp>

#include <cerrno>

#ifdef _WIN32
#include <windows.h>
#else
#include <unistd.h>
#include <spawn.h>
#include <sys/wait.h>
extern char **environ;
#endif

#include <vector>
#include <string>
#include <cstdlib>

std::string ToolLaunch::toolName(const std::string &toolFlag)
{
    if (toolFlag == "--trigger-editor")
        return "Trigger Editor";
    if (toolFlag == "--sprite-fusion")
        return "Sprite Fusion";
    return toolFlag;
}

std::string ToolLaunch::getExecutablePath()
{
    char buffer[4096];
#ifdef _WIN32
    DWORD length = GetModuleFileNameA(nullptr, buffer, sizeof(buffer));
    if (length == 0 || length >= sizeof(buffer))
        return "";
#else
    ssize_t length = readlink("/proc/self/exe", buffer, sizeof(buffer) - 1);
    if (length <= 0)
        return "";
    buffer[length] = '\0';
#endif
    return std::string(buffer);
}

// The spawned tool is the same realm-editor binary: it resolves its own
// runtime data (data/options/realm-editor.json, font, icon) from the working
// directory, which here is the folder that contains the running executable.
int ToolLaunch::launchTool(const std::string &gamePath, const std::string &toolFlag)
{
    boost::filesystem::path exePath(getExecutablePath());
    if (exePath.empty())
        exePath = boost::filesystem::current_path() / "realm-editor";
    else
        exePath = exePath.parent_path() / exePath.filename();

    if (!boost::filesystem::exists(exePath))
        return 0;

    const std::string workingDirectory = exePath.parent_path().string();

#ifdef _WIN32
    // --game-path is quoted so paths containing spaces survive.
    std::string commandLine = "\"" + exePath.string() + "\" --no-splash " + toolFlag
                              + " --game-path \"" + gamePath + "\"";
    std::vector<char> commandBuffer(commandLine.begin(), commandLine.end());
    commandBuffer.push_back('\0');

    STARTUPINFOA startupInfo;
    PROCESS_INFORMATION processInfo;
    ZeroMemory(&startupInfo, sizeof(startupInfo));
    startupInfo.cb = sizeof(startupInfo);
    ZeroMemory(&processInfo, sizeof(processInfo));

    if (!CreateProcessA(exePath.string().c_str(), commandBuffer.data(), nullptr, nullptr, FALSE, 0,
                        nullptr,
                        workingDirectory.empty() ? nullptr : workingDirectory.c_str(),
                        &startupInfo, &processInfo))
        return -1;

    // Fire and forget: the tool closes itself when the user leaves it, and
    // the editor never needs to wait for it.
    CloseHandle(processInfo.hThread);
    CloseHandle(processInfo.hProcess);
    return 1;
#else
    // Build argv for posix_spawnp (absolute path, so no PATH lookup needed).
    std::string exeArg = exePath.string();
    std::string noSplash = "--no-splash";
    std::string flag = toolFlag;
    std::string pathOption = "--game-path";
    std::string pathValue = gamePath;

    std::vector<char *> argv;
    argv.push_back(&exeArg[0]);
    argv.push_back(&noSplash[0]);
    argv.push_back(&flag[0]);
    argv.push_back(&pathOption[0]);
    argv.push_back(&pathValue[0]);
    argv.push_back(nullptr);

    pid_t pid = -1;
    if (posix_spawnp(&pid, exeArg.c_str(), nullptr, nullptr, argv.data(), environ) != 0)
        return -1;
    if (pid <= 0)
        return -1;

    // Reap the child whenever it exits (posix_spawnp children are not
    // automatically reaped): without this every spawned tool stays a zombie
    // until the editor closes.
    int status = 0;
    while (waitpid(pid, &status, 0) < 0)
    {
        if (errno != EINTR)
            break;
    }
    return 1;
#endif
}
