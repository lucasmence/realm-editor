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

int ToolLaunch::launchTool(const std::string &gamePath, const std::string &toolFlag,
                           const std::string &hudTheme)
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
    std::string commandLine = "\"" + exePath.string() + "\" --no-splash " + toolFlag
                              + (hudTheme.empty() ? std::string()
                                                  : " --hud-theme \"" + hudTheme + "\"")
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

    CloseHandle(processInfo.hThread);
    CloseHandle(processInfo.hProcess);
    return 1;
#else
    std::string exeArg = exePath.string();
    std::string noSplash = "--no-splash";
    std::string flag = toolFlag;
    std::string themeOption = "--hud-theme";
    std::string themeValue = hudTheme;
    std::string pathOption = "--game-path";
    std::string pathValue = gamePath;

    std::vector<char *> argv;
    argv.push_back(&exeArg[0]);
    argv.push_back(&noSplash[0]);
    argv.push_back(&flag[0]);
    if (!hudTheme.empty())
    {
        argv.push_back(&themeOption[0]);
        argv.push_back(&themeValue[0]);
    }
    argv.push_back(&pathOption[0]);
    argv.push_back(&pathValue[0]);
    argv.push_back(nullptr);

    pid_t pid = -1;
    if (posix_spawnp(&pid, exeArg.c_str(), nullptr, nullptr, argv.data(), environ) != 0)
        return -1;
    if (pid <= 0)
        return -1;

    int status = 0;
    while (waitpid(pid, &status, 0) < 0)
    {
        if (errno != EINTR)
            break;
    }
    return 1;
#endif
}
