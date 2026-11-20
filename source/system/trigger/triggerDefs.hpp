#include <string>
#include <vector>

#pragma once

#ifndef TRIGGERDEFS_HPP
#define TRIGGERDEFS_HPP


struct TriggerParam
{
    std::string field;
    int index;
    std::string label;
    std::string type;
    bool required;
};

struct TriggerTypeDef
{
    std::string key;
    std::string description;
    std::vector<TriggerParam> params;
};

extern const std::vector<TriggerTypeDef> GUI_EVENT_DEFS;
extern const std::vector<TriggerTypeDef> GUI_CONDITION_DEFS;
extern const std::vector<TriggerTypeDef> GUI_ACTION_DEFS;

const TriggerTypeDef *guiFindDef(const std::vector<TriggerTypeDef> &defs,
                                 const std::string &key);

#endif
