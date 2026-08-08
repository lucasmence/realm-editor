#include "triggerDefs.hpp"

// ---------------------------------------------------------------------------
// Metadata tables for the visual trigger editor.
//
// Descriptions and labels are English on purpose (the visual editor keeps its
// strings in English for now; they do not use the language files).
// Parameters were derived from the fields the engine actually reads:
//   - events     -> event.cpp / Map::loadTrigger
//   - conditions -> condition.cpp
//   - actions    -> action.cpp
// ---------------------------------------------------------------------------

namespace
{
    TriggerParam P(const char *field, int index, const char *label,
                   const char *type, bool required = false)
    {
        return TriggerParam{ field, index, label, type, required };
    }

    std::vector<TriggerParam> PT(const std::vector<TriggerParam> &params)
    {
        return params;
    }

    TriggerTypeDef T(const char *key, const char *description,
                     const std::vector<TriggerParam> &params)
    {
        return TriggerTypeDef{ key, description, params };
    }
}

// ===========================================================================
// EVENTS
// ===========================================================================

const std::vector<TriggerTypeDef> GUI_EVENT_DEFS =
{
    T("initialization", "When the map is initialized", PT({})),
    T("unit-kill", "When a unit dies", PT({
        P("unitName", -1, "Unit name", "string", true)
    })),
    T("region-between-units", "When a unit gets close to another unit", PT({
        P("unitName", -1, "Unit name", "string", true),
        P("floatValue", -1, "Maximum distance", "float", true)
    })),
    T("distance-between-units", "When a unit moves away from another unit", PT({
        P("unitName", -1, "Unit name", "string", true),
        P("floatValue", -1, "Minimum distance", "float", true)
    })),
    T("dialog-box-close", "When a dialog box is closed", PT({})),
    T("unit-cast-ability", "When a unit casts an ability", PT({})),
    T("unit-use-item", "When a unit uses an item", PT({})),
    T("call-event", "When triggered by the call-event action", PT({})),
    T("region-entered", "When a unit enters a region", PT({
        P("unitName", -1, "Unit name", "string", true),
        P("integerValue", -1, "Region index", "int", true)
    })),
    T("item-dropped", "When an item is dropped", PT({})),
    T("item-picked", "When an item is picked up", PT({})),
    T("player-joined", "When a player joins the game", PT({}))
};

// ===========================================================================
// CONDITIONS
// ===========================================================================

const std::vector<TriggerTypeDef> GUI_CONDITION_DEFS =
{
    T("game-event-check", "Game event is a specific value", PT({
        P("integerValue", -1, "Game event ID", "int", true)
    })),
    T("unit-check-target", "Triggering unit has a specific name", PT({
        P("unitName", -1, "Unit name", "string", true)
    })),
    T("unit-check-stats", "Unit stat matches a value", PT({
        P("strings", 0, "Attribute", "string", true),
        P("strings", 1, "Operator (equal/greater/lower/greater-or-equal/lower-or-equal)", "string", true),
        P("floats", 0, "Value", "float", true),
        P("strings", 2, "Attribute name (custom)", "string", false)
    })),
    T("dialog-box-option", "A specific dialog box option was chosen", PT({
        P("integers", 0, "Dialog box ID", "int", true),
        P("integers", 1, "Option index", "int", true)
    })),
    T("get-game-context", "Game context is a specific string", PT({
        P("strings", 0, "Context", "string", true),
        P("booleans", 0, "Negate (check NOT)", "bool", false)
    })),
    T("unit-check-target-ability-hotkey", "Unit last cast from a hotkey slot", PT({
        P("integers", 0, "Hotkey", "int", true)
    })),
    T("unit-check-used-item", "Unit last used a specific item", PT({
        P("strings", 0, "Item", "string", true)
    })),
    T("player-control-type", "Player control mode matches", PT({
        P("integerValue", -1, "Control mode", "int", true)
    })),
    T("get-global-variable", "Global variable is a specific boolean", PT({
        P("strings", 0, "Variable name", "string", true),
        P("booleans", 0, "Expected value", "bool", true)
    })),
    T("unit-check-alive", "A unit is alive or dead", PT({
        P("strings", 0, "Unit name", "string", true),
        P("booleans", 0, "Expected alive", "bool", true)
    })),
    T("region-check-item", "An item exists inside a region", PT({
        P("strings", 0, "Item name", "string", true),
        P("integers", 0, "Region index", "int", true),
        P("booleans", 0, "Expected present", "bool", false)
    })),
    T("region-check-unit", "A unit exists inside a region", PT({
        P("strings", 0, "Unit name", "string", true),
        P("integers", 0, "Region index", "int", true),
        P("booleans", 0, "Expected present", "bool", false)
    })),
    T("unit-player-check-item", "A unit or its storage has an item", PT({
        P("strings", 0, "Unit name", "string", true),
        P("strings", 1, "Item name", "string", true),
        P("booleans", 0, "Expected present", "bool", false)
    })),
    T("map-check-visited", "A map was visited before", PT({
        P("strings", 0, "Map path", "string", true),
        P("booleans", 0, "Expected visited", "bool", false)
    })),
    T("unit-check-type", "A unit has a specific type/filename", PT({
        P("strings", 0, "Type/filename", "string", true),
        P("strings", 1, "Unit name", "string", false)
    })),
    T("unit-check-picked-item", "Unit last picked up a specific item", PT({
        P("strings", 0, "Item name", "string", true)
    })),
    T("unit-check-casted-ability", "Unit last cast a specific ability", PT({
        P("integers", 0, "Ability index", "int", true)
    }))
};

// ===========================================================================
// ACTIONS
// ===========================================================================

const std::vector<TriggerTypeDef> GUI_ACTION_DEFS =
{
    T("game-event-change", "Change the current game event", PT({
        P("integerValue", -1, "Game event ID", "int", true)
    })),
    T("camera-move-unit", "Move the camera to a unit", PT({
        P("unitName", -1, "Unit name", "string", true),
        P("floats", 0, "Duration (0 = instant)", "float", false)
    })),
    T("camera-lock-hero", "Lock or unlock the camera on the hero", PT({
        P("booleans", 0, "Locked", "bool", true)
    })),
    T("menu-show", "Open a menu", PT({
        P("strings", 0, "Menu name", "string", true),
        P("booleans", 0, "Full reset", "bool", false)
    })),
    T("system-sleep", "Pause for a duration", PT({
        P("floatValue", -1, "Seconds", "float", true)
    })),
    T("text-dialog", "Show a floating dialog over a unit", PT({
        P("unitName", -1, "Unit name", "string", true),
        P("strings", 0, "Dialog category", "string", true),
        P("strings", 1, "Dialog key", "string", true),
        P("strings", 2, "\"USE-DIALOGBOX\" to force dialog box", "string", false)
    })),
    T("unit-intelligence-range-min-change", "Change the aggro range of a unit", PT({
        P("unitName", -1, "Unit name", "string", true),
        P("floatValue", -1, "Minimum range", "float", true)
    })),
    T("set-gold-all-players", "Set the gold of all players", PT({
        P("integerValue", -1, "Gold", "int", true)
    })),
    T("set-food", "Set the food of a unit (boltcraft)", PT({
        P("strings", 0, "Unit name", "string", true),
        P("integers", 0, "Food amount", "int", true)
    })),
    T("text-messagebox", "Show a message box", PT({
        P("integers", 0, "Type (0 info / 1 warning)", "int", false),
        P("strings", 0, "Caption category", "string", false),
        P("strings", 1, "Caption key", "string", false),
        P("strings", 2, "Text category", "string", false),
        P("strings", 3, "Text key", "string", false)
    })),
    T("text-dialogbox", "Open a dialog box with options", PT({
        P("unitName", -1, "Unit name", "string", true),
        P("integers", 0, "Dialog box ID", "int", false),
        P("strings", 0, "Portrait name", "string", false),
        P("strings", 1, "Caption category", "string", false),
        P("strings", 2, "Caption key", "string", false),
        P("strings", 3, "Text category", "string", false),
        P("strings", 4, "Text key", "string", false)
    })),
    T("text-console", "Show a message in the console", PT({
        P("strings", 0, "Category", "string", true),
        P("strings", 1, "Key", "string", true),
        P("strings", 2, "Portrait icon", "string", false)
    })),
    T("add-gold-all-players", "Add gold to all players", PT({
        P("integerValue", -1, "Gold", "int", true)
    })),
    T("spawn-item", "Spawn one or more items", PT({
        P("strings", 0, "Item path", "string", true),
        P("strings", 1, "Item path (extra)", "string", false),
        P("integers", 0, "Stack", "int", false),
        P("floats", 0, "Position X (0 = unit)", "float", false),
        P("floats", 1, "Position Y", "float", false)
    })),
    T("add-item", "Add an item to a unit inventory", PT({
        P("unitName", -1, "Unit name", "string", true),
        P("strings", 0, "Item path", "string", true),
        P("integers", 0, "Stack", "int", false)
    })),
    T("trigger-reset", "Reset the trigger so it can fire again", PT({})),
    T("remove-skill-all", "Remove all abilities from a unit", PT({
        P("strings", 0, "Unit name", "string", true)
    })),
    T("remove-passive-all", "Remove all passives from a unit", PT({
        P("strings", 0, "Unit name", "string", true)
    })),
    T("clear-storage", "Clear the shared stash", PT({})),
    T("add-skill", "Add abilities to a unit or the shared stash", PT({
        P("booleans", 0, "Add to shared stash", "bool", false),
        P("strings", 0, "Unit name", "string", false),
        P("strings", 1, "Ability path", "string", false),
        P("integers", 0, "Hotkey", "int", false)
    })),
    T("add-passive", "Add passives to a unit", PT({
        P("strings", 0, "Unit name", "string", true),
        P("strings", 1, "Passive path (extra)", "string", false)
    })),
    T("disable-skill", "Disable specific abilities of a unit", PT({
        P("strings", 0, "Unit name", "string", true),
        P("strings", 1, "Ability path (extra)", "string", false)
    })),
    T("set-game-context", "Set the game context string", PT({
        P("strings", 0, "Context", "string", true)
    })),
    T("unit-edit-status", "Edit stats/attributes of a unit", PT({
        P("strings", 0, "Unit name", "string", true),
        P("booleans", 0, "Use attribute names", "bool", false),
        P("booleans", 1, "Add mode (true) / set mode (false)", "bool", false),
        P("strings", 1, "Attribute (extra)", "string", false),
        P("floats", 0, "Value (extra)", "float", false)
    })),
    T("spawn-unit", "Spawn a unit at a position", PT({
        P("strings", 0, "Unit path", "string", true),
        P("strings", 1, "Alliance (enemy/neutral)", "string", true),
        P("integers", 0, "Position X", "int", true),
        P("integers", 1, "Position Y", "int", true),
        P("integers", 2, "Group", "int", false),
        P("booleans", 0, "Summoned portal", "bool", false),
        P("strings", 2, "Custom name", "string", false),
        P("strings", 3, "Item drop", "string", false)
    })),
    T("spawn-unit-region", "Spawn a unit inside a region", PT({
        P("strings", 0, "Unit path", "string", true),
        P("strings", 1, "Alliance (enemy/neutral)", "string", true),
        P("integers", 0, "Region index", "int", true),
        P("integers", 1, "Group", "int", false),
        P("booleans", 0, "Summoned portal", "bool", false),
        P("strings", 2, "Custom name", "string", false),
        P("strings", 3, "Item drop", "string", false)
    })),
    T("unit-edit-exp", "Edit the experience of a unit", PT({
        P("strings", 0, "Unit name", "string", true),
        P("integers", 0, "Experience", "int", true),
        P("booleans", 0, "Add mode (true) / set mode (false)", "bool", false)
    })),
    T("unit-pause", "Pause a unit for a duration", PT({
        P("strings", 0, "Unit name", "string", true),
        P("floats", 0, "Seconds", "float", true)
    })),
    T("unit-unpause", "Unpause a unit", PT({
        P("strings", 0, "Unit name", "string", true)
    })),
    T("add-special-effect", "Add a special effect", PT({
        P("strings", 0, "Texture", "string", true),
        P("strings", 1, "Animation", "string", false),
        P("strings", 2, "Attach type", "string", false),
        P("strings", 3, "Unit name", "string", false),
        P("floats", 0, "Position X", "float", false),
        P("floats", 1, "Position Y", "float", false),
        P("floats", 2, "Duration", "float", false),
        P("floats", 3, "Scale", "float", false),
        P("integers", 0, "Color R", "int", false),
        P("integers", 1, "Color G", "int", false),
        P("integers", 2, "Color B", "int", false),
        P("integers", 3, "Color A", "int", false)
    })),
    T("unit-teleport", "Teleport a unit to a position", PT({
        P("strings", 0, "Unit name", "string", true),
        P("floats", 0, "Position X", "float", true),
        P("floats", 1, "Position Y", "float", true)
    })),
    T("unit-kill", "Kill a unit", PT({
        P("strings", 0, "Unit name", "string", true)
    })),
    T("unit-remove", "Remove a unit from the map", PT({
        P("strings", 0, "Unit name", "string", true)
    })),
    T("unit-change-name", "Change the name of a unit", PT({
        P("strings", 0, "Unit name", "string", true),
        P("strings", 1, "New name", "string", true)
    })),
    T("item-remove", "Remove an item from the world", PT({
        P("strings", 0, "Item name", "string", true)
    })),
    T("unit-cast-skill", "Make a unit cast a skill on another unit", PT({
        P("strings", 0, "Unit name", "string", true),
        P("strings", 1, "Target unit", "string", true),
        P("strings", 2, "Skill name", "string", true),
        P("strings", 3, "Animation", "string", false)
    })),
    T("spawn-merchant", "Spawn a merchant at a position", PT({
        P("strings", 0, "Merchant path", "string", true),
        P("integers", 0, "Position X", "int", true),
        P("integers", 1, "Position Y", "int", true)
    })),
    T("merchant-remove", "Remove a merchant by name", PT({
        P("strings", 0, "Merchant name", "string", true)
    })),
    T("merchant-remove-all", "Remove all merchants", PT({})),
    T("merchant-update", "Refresh the inventory of a merchant", PT({
        P("strings", 0, "Merchant name", "string", true)
    })),
    T("unit-mass-enemy-kill", "Kill all enemy units", PT({})),
    T("unit-mass-kill-type", "Kill all units of a specific type", PT({
        P("strings", 0, "Unit type/filename", "string", true)
    })),
    T("delete-save", "Delete the save file", PT({})),
    T("game-select-subclass", "Open the subclass selection screen", PT({})),
    T("check-official-custom-map", "Check that the map is an official custom map (boltcraft)", PT({})),
    T("check-campaign-endgame", "Check the campaign endgame state (boltcraft)", PT({})),
    T("check-campaign-final", "Check the campaign final state (boltcraft)", PT({})),
    T("unit-hide", "Hide a unit", PT({
        P("strings", 0, "Unit name", "string", true),
        P("floatValue", -1, "Hide duration", "float", false)
    })),
    T("unit-unhide", "Reveal a hidden unit", PT({
        P("strings", 0, "Unit name", "string", true)
    })),
    T("add-sfx", "Play a special effect", PT({
        P("strings", 0, "Effect path", "string", true),
        P("floats", 0, "Position X", "float", false),
        P("floats", 1, "Position Y", "float", false),
        P("integers", 0, "Region index", "int", false),
        P("booleans", 0, "HUD canvas", "bool", false)
    })),
    T("play-animation", "Play an animation on a unit", PT({
        P("strings", 0, "Unit name", "string", true),
        P("strings", 1, "Animation", "string", true),
        P("booleans", 0, "Set as default animation", "bool", false)
    })),
    T("call-event", "Trigger the call-event event", PT({})),
    T("prop-play-animation", "Play an animation on a prop", PT({
        P("strings", 0, "Prop variable", "string", true),
        P("strings", 1, "Animation", "string", true)
    })),
    T("environment-play-animation", "Play an animation on an environment", PT({
        P("strings", 0, "Environment variable", "string", true),
        P("strings", 1, "Animation", "string", true)
    })),
    T("camera-move-prop", "Move the camera to a prop", PT({
        P("strings", 0, "Prop variable", "string", true),
        P("floats", 0, "Duration (0 = instant)", "float", false)
    })),
    T("camera-move-region", "Move the camera to a region", PT({
        P("integers", 0, "Region index", "int", true),
        P("floats", 0, "Duration (0 = instant)", "float", false)
    })),
    T("camera-shake", "Shake the camera", PT({
        P("floats", 0, "Duration", "float", true)
    })),
    T("play-sound", "Play a sound effect", PT({
        P("strings", 0, "Sound path", "string", true)
    })),
    T("sound-play", "Play a sound effect (boltcraft)", PT({
        P("strings", 0, "Sound path", "string", true)
    })),
    T("portal-enable", "Enable or disable a portal", PT({
        P("integers", 0, "Portal index", "int", true),
        P("booleans", 0, "Active", "bool", true)
    })),
    T("set-global-variable", "Set a global boolean variable", PT({
        P("strings", 0, "Variable name", "string", true),
        P("booleans", 0, "Value", "bool", true)
    })),
    T("prop-kill", "Remove a prop", PT({
        P("strings", 0, "Prop variable", "string", true)
    })),
    T("block-map-teleport", "Block or allow map teleport", PT({
        P("booleans", 0, "Blocked", "bool", true)
    })),
    T("unit-cast-skill-region", "Make a unit cast a skill at a region", PT({
        P("strings", 0, "Unit name", "string", true),
        P("strings", 1, "Skill name", "string", true),
        P("integers", 0, "Region index", "int", true),
        P("strings", 2, "Animation", "string", false)
    })),
    T("unit-teleport-region", "Teleport a unit to a region", PT({
        P("strings", 0, "Unit name", "string", true),
        P("integers", 0, "Region index", "int", true)
    })),
    T("unit-move-unit", "Make a unit move to another unit", PT({
        P("strings", 0, "Unit name", "string", true),
        P("strings", 1, "Target unit", "string", true)
    })),
    T("unit-move-region", "Make a unit move to a region", PT({
        P("strings", 0, "Unit name", "string", true),
        P("integers", 0, "Region index", "int", true)
    })),
    T("spawn-item-region", "Spawn items inside a region", PT({
        P("strings", 0, "Item path", "string", true),
        P("strings", 1, "Item path (extra)", "string", false),
        P("integers", 0, "Stack", "int", false),
        P("integers", 1, "Region index", "int", true)
    })),
    T("region-terrain-animation", "Play an animation on terrain in a region", PT({
        P("integers", 0, "Region index", "int", true),
        P("strings", 0, "Animation", "string", true)
    })),
    T("region-terrain-remove", "Remove terrain inside a region", PT({
        P("integers", 0, "Region index", "int", true)
    })),
    T("apply-map-fog", "Apply fog of war", PT({
        P("integers", 0, "Style", "int", true)
    })),
    T("break-map-fog", "Remove fog of war", PT({})),
    T("music-change", "Change the background music", PT({
        P("strings", 0, "Music name", "string", true)
    })),
    T("spawn-environment", "Spawn an environment object", PT({
        P("strings", 0, "Environment path", "string", true),
        P("strings", 1, "Variable", "string", true),
        P("integers", 0, "Position X", "int", true),
        P("integers", 1, "Position Y", "int", true)
    })),
    T("music-pause", "Pause the music", PT({})),
    T("travel-block", "Block or allow map travel", PT({
        P("booleans", 0, "Blocked", "bool", true)
    })),
    T("unit-set-dialog-value", "Set the dialog value of a unit", PT({
        P("strings", 0, "Unit name", "string", true),
        P("integers", 0, "Value", "int", true)
    })),
    T("wall-remove", "Remove walls inside a region", PT({
        P("integers", 0, "Region index", "int", true)
    })),
    T("change-map-particles", "Change the map particles", PT({
        P("strings", 0, "Particles name", "string", true)
    })),
    T("change-mass-environment", "Replace all environments of one type", PT({
        P("strings", 0, "Old environment path", "string", true),
        P("strings", 1, "New environment path", "string", true)
    })),
    T("show-help-tooltip", "Show the help tooltip", PT({})),
    T("reset-unit", "Reset a unit to its defaults", PT({
        P("strings", 0, "Unit name", "string", true)
    })),
    T("remove-skills-hotkey", "Remove abilities by hotkey slot", PT({
        P("strings", 0, "Unit name", "string", true),
        P("integers", 0, "Hotkey (extra)", "int", false)
    })),
    T("add-environment-sfx", "Spawn an environment with particles", PT({
        P("strings", 0, "Environment path", "string", true),
        P("strings", 1, "Variable", "string", false),
        P("floats", 0, "Position X", "float", true),
        P("floats", 1, "Position Y", "float", true),
        P("integers", 0, "Color R", "int", false),
        P("integers", 1, "Color G", "int", false),
        P("integers", 2, "Color B", "int", false),
        P("integers", 3, "Color A", "int", false),
        P("integers", 4, "Particle count", "int", false)
    })),
    T("add-environment-sfx-region", "Spawn an environment with particles in a region", PT({
        P("strings", 0, "Environment path", "string", true),
        P("strings", 1, "Variable", "string", false),
        P("integers", 0, "Region index", "int", true),
        P("integers", 1, "Color R", "int", false),
        P("integers", 2, "Color G", "int", false),
        P("integers", 3, "Color B", "int", false),
        P("integers", 4, "Color A", "int", false),
        P("integers", 5, "Particle count", "int", false)
    })),
    T("enable-skill-tree", "Show or hide the skill tree", PT({
        P("booleans", 0, "Show", "bool", true)
    })),
    T("clear-skill-tree", "Clear the skill tree choices of a unit", PT({
        P("strings", 0, "Unit name", "string", true)
    })),
    T("clear-passive-tree", "Clear the passive tree choices of a unit", PT({
        P("strings", 0, "Unit name", "string", true)
    })),
    T("move-all-items-region", "Drop all items of a unit at a position", PT({
        P("strings", 0, "Unit name", "string", true),
        P("floats", 0, "Position X", "float", true),
        P("floats", 1, "Position Y", "float", true)
    })),
    T("take-all-items-region", "Pick up all items at a position into a unit", PT({
        P("strings", 0, "Unit name", "string", true),
        P("floats", 0, "Position X", "float", true),
        P("floats", 1, "Position Y", "float", true)
    })),
    T("minimap-disable", "Disable the minimap", PT({})),
    T("environment-play-animation-region", "Play an animation on environments in a region", PT({
        P("integers", 0, "Region index", "int", true),
        P("strings", 0, "Animation", "string", true)
    })),
    T("add-wall", "Add a wall at a region", PT({
        P("integers", 0, "Region index", "int", true)
    })),
    T("fog-toggle", "Toggle the fog setting", PT({
        P("booleans", 0, "Enabled", "bool", true)
    })),
    T("add-intro-sound", "Set the guardian intro sounds", PT({
        P("strings", 0, "Sound path", "string", true),
        P("strings", 1, "Sound path (extra)", "string", false)
    })),
    T("play-intro-sound", "Play the current guardian intro sound", PT({})),
    T("erase-world", "Erase everything outside a region", PT({
        P("integers", 0, "Region index", "int", true)
    })),
    T("exit", "Go to the exit map", PT({})),
    T("show-credits-label", "Show a credits label in a region", PT({
        P("strings", 0, "Credits frame", "string", true),
        P("integers", 0, "Region index", "int", true)
    })),
    T("toggle-control", "Enable or disable player control", PT({
        P("booleans", 0, "Enabled", "bool", true)
    })),
    T("toggle-extra-modes", "Enable or disable extra modes", PT({
        P("booleans", 0, "Enabled", "bool", true)
    })),
    T("extra-modes-info", "Show the extra modes info box", PT({})),
    T("server-lock", "Lock the server", PT({})),
    T("server-standby", "Put the server in standby", PT({})),
    T("player-alliance", "Set the alliance between two players", PT({
        P("integers", 0, "Player A", "int", true),
        P("integers", 1, "Player B", "int", true),
        P("strings", 0, "Alliance (ally/neutral/enemy)", "string", true)
    })),
    T("show-map-caption", "Show a caption on the map", PT({
        P("strings", 0, "Category", "string", true),
        P("strings", 1, "Key", "string", true)
    })),
    T("console-dialogbox", "Show a dialog in the console", PT({
        P("unitName", -1, "Unit name", "string", false),
        P("strings", 0, "Portrait", "string", true),
        P("strings", 1, "Category", "string", true),
        P("strings", 2, "Key", "string", true)
    })),
    T("world-minimap-set", "Set the world minimap data", PT({
        P("strings", 0, "Minimap name", "string", true)
    })),
    T("quest-new", "Add a new quest", PT({
        P("strings", 0, "Quest path", "string", true),
        P("strings", 1, "Quest ID", "string", true)
    })),
    T("quest-update", "Update a quest objective", PT({
        P("strings", 0, "Quest ID", "string", true),
        P("strings", 1, "Objective ID", "string", true),
        P("integers", 0, "Progress", "int", true),
        P("booleans", 0, "Visible", "bool", false)
    })),
    T("dialog-mark", "Show/hide a dialog mark over a unit", PT({
        P("unitName", -1, "Unit name", "string", true),
        P("strings", 0, "Mark effect path", "string", false)
    }))
};

// ---------------------------------------------------------------------------

const TriggerTypeDef *guiFindDef(const std::vector<TriggerTypeDef> &defs,
                                 const std::string &key)
{
    for (const auto &def : defs)
        if (def.key == key)
            return &def;
    return nullptr;
}
