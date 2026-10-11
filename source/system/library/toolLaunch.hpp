#pragma once

#ifndef TOOLLAUNCH_HPP
#define TOOLLAUNCH_HPP

#include <string>

// ---------------------------------------------------------------------------
// Process launching helpers used by the World Editor "Tools" menu.
//
// The Trigger Editor and Sprite Fusion are standalone modes of this same
// realm-editor binary (selected with --trigger-editor / --sprite-fusion).
// When the user opens them from the World Editor's Tools menu we spawn a
// SECOND realm-editor process in that mode instead of hosting the tool in a
// second window of this process. Closing the tool ("Back to Studio" button or
// window close) then exits only that process - the World Editor window, map
// and unsaved changes stay untouched (the previous in-window hosting closed
// the whole editor when the tool was closed, losing the open project).
//
// All platform-specific code (CreateProcess / posix_spawnp) lives in
// toolLaunch.cpp so no other translation unit needs <windows.h>.
// ---------------------------------------------------------------------------
namespace ToolLaunch
{
    // Spawns a new realm-editor process running the requested standalone
    // tool:
    //   toolFlag: "--trigger-editor" -> Trigger Editor
    //             "--sprite-fusion"  -> Sprite Fusion
    // Return values:
    //   1  process started successfully
    //   0  the executable could not be located next to the running one
    //  -1  the process could not be spawned (CreateProcess/fork/exec failed)
    int launchTool(const std::string &gamePath, const std::string &toolFlag);

    // Returns "" when the running executable's path could not be determined.
    std::string getExecutablePath();

    // Tool caption for log messages ("Trigger Editor" / "Sprite Fusion").
    std::string toolName(const std::string &toolFlag);
}

#endif // TOOLLAUNCH_HPP
