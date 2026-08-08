#include <string>
#include <vector>

#pragma once

#ifndef TRIGGERDEFS_HPP
#define TRIGGERDEFS_HPP

// ---------------------------------------------------------------------------
// Trigger type metadata for the visual (GUI) trigger editor.
//
// Each type (event / condition / action) is described by:
//   - key          : the identifier used in the trigger JSON files
//                    ("unit-kill", "game-event-check", "spawn-unit", ...)
//   - description  : a short English description shown to the user in the
//                    searchable lookup and in the entry lists. Descriptions
//                    are intentionally English for now (fast to maintain);
//                    they do NOT go through the language files.
//   - params       : the fields the engine reads for that type (see
//                    event.cpp / condition.cpp / action.cpp and
//                    Map::loadTriggerParameters in world/map.cpp).
//
// Parameter fields:
//   - "unitName"       scalar string
//   - "text"           scalar string
//   - "integerValue"   scalar int
//   - "floatValue"     scalar float
//   - "strings"        array of { "value": "..." }
//   - "integers"       array of { "value": N }
//   - "floats"         array of { "value": N.N }
//   - "booleans"       array of { "value": true/false }
// ---------------------------------------------------------------------------

struct TriggerParam
{
    std::string field;      // json field name
    int index;              // array index; -1 for scalar fields
    std::string label;      // english label shown next to the edit box
    std::string type;       // "string", "int", "float", "bool"
    bool required;
};

struct TriggerTypeDef
{
    std::string key;
    std::string description;
    std::vector<TriggerParam> params;
};

// Event / condition / action metadata tables (defined in triggerDefs.cpp).
extern const std::vector<TriggerTypeDef> GUI_EVENT_DEFS;
extern const std::vector<TriggerTypeDef> GUI_CONDITION_DEFS;
extern const std::vector<TriggerTypeDef> GUI_ACTION_DEFS;

// Returns the type definition matching `key`, or nullptr when unknown.
const TriggerTypeDef *guiFindDef(const std::vector<TriggerTypeDef> &defs,
                                 const std::string &key);

#endif // TRIGGERDEFS_HPP
