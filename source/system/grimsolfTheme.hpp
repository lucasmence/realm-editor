#pragma once

#include <string>
#include <vector>

#include "external/imgui/imgui.h"
#include "library/json.hpp"

namespace GrimsolfTheme
{
    // True while a theme that paints dark text over light backgrounds is
    // active. The tools author their grays against the dark themes (near-white
    // text, dark panels), so gray()/toGray() flip their values in a light
    // theme to keep the same contrast.
    inline bool &lightModeState()
    {
        static bool light = false;
        return light;
    }

    inline bool isLight()
    {
        return lightModeState();
    }

    // Foreground colors of the active theme. Manager restores them at the
    // start of every frame (some UI code pushes text colors and never pops
    // them), which used to hardcode white and hide the light theme's text.
    inline ImVec4 &textColorState()
    {
        static ImVec4 text(1.0f, 1.0f, 1.0f, 1.0f);
        return text;
    }

    inline ImVec4 &textDisabledColorState()
    {
        static ImVec4 textDisabled(0.55f, 0.55f, 0.55f, 1.0f);
        return textDisabled;
    }

    inline ImVec4 textColor()
    {
        return textColorState();
    }

    inline ImVec4 textDisabledColor()
    {
        return textDisabledColorState();
    }

    inline ImVec4 gray(float value, float alpha = 1.0f)
    {
        if (lightModeState())
            value = 1.0f - value;
        return ImVec4(value, value, value, alpha);
    }

    // Keeps the perceived brightness of a colored value while removing all
    // saturation. Used by the tools that still receive colored log entries.
    inline ImVec4 toGray(const ImVec4 &color)
    {
        float value = 0.299f * color.x + 0.587f * color.y + 0.114f * color.z;
        value = lightModeState() ? 0.45f * value : 0.72f + 0.28f * value;
        return ImVec4(value, value, value, color.w);
    }

    // Theme names shown in the Options window. The first one is the default.
    inline const std::vector<std::string> &names()
    {
        static const std::vector<std::string> themeNames = {
            "Grimsolf",
            "Light",
            "Navy Green",
            "Wine",
            "Monokai",
            "Terminal Blue",
            "ImGui Default"
        };
        return themeNames;
    }

    // Geometry shared by every bundled theme (the ImGui default theme resets
    // it instead).
    inline void applyMetrics(ImGuiStyle &style)
    {
        style.WindowPadding = ImVec2(10.0f, 10.0f);
        style.FramePadding = ImVec2(8.0f, 4.0f);
        style.ItemSpacing = ImVec2(8.0f, 6.0f);
        style.ItemInnerSpacing = ImVec2(6.0f, 4.0f);
        style.IndentSpacing = 18.0f;
        style.ScrollbarSize = 14.0f;
        style.GrabMinSize = 12.0f;

        style.WindowBorderSize = 1.0f;
        style.ChildBorderSize = 1.0f;
        style.PopupBorderSize = 1.0f;
        style.FrameBorderSize = 1.0f;
        style.TabBorderSize = 1.0f;

        style.WindowRounding = 0.0f;
        style.ChildRounding = 0.0f;
        style.FrameRounding = 0.0f;
        style.PopupRounding = 0.0f;
        style.ScrollbarRounding = 0.0f;
        style.GrabRounding = 0.0f;
        style.TabRounding = 0.0f;

        style.WindowTitleAlign = ImVec2(0.0f, 0.5f);
        style.ButtonTextAlign = ImVec2(0.5f, 0.5f);
        style.SelectableTextAlign = ImVec2(0.0f, 0.5f);
    }

    inline void applyGrimsolf(ImGuiStyle &style)
    {
        applyMetrics(style);

        ImVec4 *colors = style.Colors;

        colors[ImGuiCol_Text] = gray(1.00f);
        colors[ImGuiCol_TextDisabled] = gray(0.55f);

        colors[ImGuiCol_WindowBg] = gray(0.05f);
        colors[ImGuiCol_ChildBg] = gray(0.035f);
        colors[ImGuiCol_PopupBg] = gray(0.05f, 0.98f);
        colors[ImGuiCol_Border] = gray(0.35f);
        colors[ImGuiCol_BorderShadow] = gray(0.0f, 0.0f);

        colors[ImGuiCol_FrameBg] = gray(0.12f);
        colors[ImGuiCol_FrameBgHovered] = gray(0.20f);
        colors[ImGuiCol_FrameBgActive] = gray(0.28f);

        colors[ImGuiCol_TitleBg] = gray(0.04f);
        colors[ImGuiCol_TitleBgActive] = gray(0.09f);
        colors[ImGuiCol_TitleBgCollapsed] = gray(0.03f);
        colors[ImGuiCol_MenuBarBg] = gray(0.06f);

        colors[ImGuiCol_ScrollbarBg] = gray(0.03f);
        colors[ImGuiCol_ScrollbarGrab] = gray(0.32f);
        colors[ImGuiCol_ScrollbarGrabHovered] = gray(0.45f);
        colors[ImGuiCol_ScrollbarGrabActive] = gray(0.60f);

        colors[ImGuiCol_CheckMark] = gray(0.95f);
        colors[ImGuiCol_SliderGrab] = gray(0.55f);
        colors[ImGuiCol_SliderGrabActive] = gray(0.80f);

        colors[ImGuiCol_Button] = gray(0.16f);
        colors[ImGuiCol_ButtonHovered] = gray(0.27f);
        colors[ImGuiCol_ButtonActive] = gray(0.36f);

        colors[ImGuiCol_Header] = gray(0.24f);
        colors[ImGuiCol_HeaderHovered] = gray(0.30f);
        colors[ImGuiCol_HeaderActive] = gray(0.38f);

        colors[ImGuiCol_Separator] = gray(0.30f);
        colors[ImGuiCol_SeparatorHovered] = gray(0.50f);
        colors[ImGuiCol_SeparatorActive] = gray(0.65f);

        colors[ImGuiCol_ResizeGrip] = gray(0.25f);
        colors[ImGuiCol_ResizeGripHovered] = gray(0.40f);
        colors[ImGuiCol_ResizeGripActive] = gray(0.55f);

        colors[ImGuiCol_Tab] = gray(0.10f);
        colors[ImGuiCol_TabHovered] = gray(0.28f);
        colors[ImGuiCol_TabActive] = gray(0.22f);
        colors[ImGuiCol_TabUnfocused] = gray(0.08f);
        colors[ImGuiCol_TabUnfocusedActive] = gray(0.16f);

        colors[ImGuiCol_PlotLines] = gray(0.85f);
        colors[ImGuiCol_PlotLinesHovered] = gray(1.00f);
        colors[ImGuiCol_PlotHistogram] = gray(0.85f);
        colors[ImGuiCol_PlotHistogramHovered] = gray(1.00f);

        colors[ImGuiCol_TableHeaderBg] = gray(0.14f);
        colors[ImGuiCol_TableBorderStrong] = gray(0.45f);
        colors[ImGuiCol_TableBorderLight] = gray(0.25f);
        colors[ImGuiCol_TableRowBg] = gray(0.0f, 0.0f);
        colors[ImGuiCol_TableRowBgAlt] = gray(1.0f, 0.04f);

        colors[ImGuiCol_TextSelectedBg] = gray(0.90f, 0.25f);
        colors[ImGuiCol_DragDropTarget] = gray(1.00f, 0.90f);

        colors[ImGuiCol_NavHighlight] = gray(1.00f, 0.70f);
        colors[ImGuiCol_NavWindowingHighlight] = gray(1.00f, 0.70f);
        colors[ImGuiCol_NavWindowingDimBg] = gray(0.0f, 0.60f);
        colors[ImGuiCol_ModalWindowDimBg] = gray(0.0f, 0.60f);
    }

    inline void applyLight(ImGuiStyle &style)
    {
        applyMetrics(style);

        ImVec4 *colors = style.Colors;

        colors[ImGuiCol_Text] = ImVec4(0.12f, 0.12f, 0.12f, 1.00f);
        colors[ImGuiCol_TextDisabled] = ImVec4(0.45f, 0.45f, 0.45f, 1.00f);

        colors[ImGuiCol_WindowBg] = ImVec4(0.94f, 0.94f, 0.94f, 1.00f);
        colors[ImGuiCol_ChildBg] = ImVec4(0.97f, 0.97f, 0.97f, 1.00f);
        colors[ImGuiCol_PopupBg] = ImVec4(0.97f, 0.97f, 0.97f, 0.98f);
        colors[ImGuiCol_Border] = ImVec4(0.70f, 0.70f, 0.70f, 1.00f);
        colors[ImGuiCol_BorderShadow] = ImVec4(0.0f, 0.0f, 0.0f, 0.0f);

        colors[ImGuiCol_FrameBg] = ImVec4(0.86f, 0.86f, 0.86f, 1.00f);
        colors[ImGuiCol_FrameBgHovered] = ImVec4(0.78f, 0.78f, 0.78f, 1.00f);
        colors[ImGuiCol_FrameBgActive] = ImVec4(0.70f, 0.70f, 0.70f, 1.00f);

        colors[ImGuiCol_TitleBg] = ImVec4(0.90f, 0.90f, 0.90f, 1.00f);
        colors[ImGuiCol_TitleBgActive] = ImVec4(0.82f, 0.82f, 0.82f, 1.00f);
        colors[ImGuiCol_TitleBgCollapsed] = ImVec4(0.88f, 0.88f, 0.88f, 1.00f);
        colors[ImGuiCol_MenuBarBg] = ImVec4(0.90f, 0.90f, 0.90f, 1.00f);

        colors[ImGuiCol_ScrollbarBg] = ImVec4(0.92f, 0.92f, 0.92f, 1.00f);
        colors[ImGuiCol_ScrollbarGrab] = ImVec4(0.65f, 0.65f, 0.65f, 1.00f);
        colors[ImGuiCol_ScrollbarGrabHovered] = ImVec4(0.55f, 0.55f, 0.55f, 1.00f);
        colors[ImGuiCol_ScrollbarGrabActive] = ImVec4(0.45f, 0.45f, 0.45f, 1.00f);

        colors[ImGuiCol_CheckMark] = ImVec4(0.20f, 0.20f, 0.20f, 1.00f);
        colors[ImGuiCol_SliderGrab] = ImVec4(0.55f, 0.55f, 0.55f, 1.00f);
        colors[ImGuiCol_SliderGrabActive] = ImVec4(0.35f, 0.35f, 0.35f, 1.00f);

        colors[ImGuiCol_Button] = ImVec4(0.86f, 0.86f, 0.86f, 1.00f);
        colors[ImGuiCol_ButtonHovered] = ImVec4(0.78f, 0.78f, 0.78f, 1.00f);
        colors[ImGuiCol_ButtonActive] = ImVec4(0.70f, 0.70f, 0.70f, 1.00f);

        colors[ImGuiCol_Header] = ImVec4(0.80f, 0.80f, 0.80f, 1.00f);
        colors[ImGuiCol_HeaderHovered] = ImVec4(0.74f, 0.74f, 0.74f, 1.00f);
        colors[ImGuiCol_HeaderActive] = ImVec4(0.68f, 0.68f, 0.68f, 1.00f);

        colors[ImGuiCol_Separator] = ImVec4(0.65f, 0.65f, 0.65f, 1.00f);
        colors[ImGuiCol_SeparatorHovered] = ImVec4(0.50f, 0.50f, 0.50f, 1.00f);
        colors[ImGuiCol_SeparatorActive] = ImVec4(0.40f, 0.40f, 0.40f, 1.00f);

        colors[ImGuiCol_ResizeGrip] = ImVec4(0.60f, 0.60f, 0.60f, 1.00f);
        colors[ImGuiCol_ResizeGripHovered] = ImVec4(0.50f, 0.50f, 0.50f, 1.00f);
        colors[ImGuiCol_ResizeGripActive] = ImVec4(0.40f, 0.40f, 0.40f, 1.00f);

        colors[ImGuiCol_Tab] = ImVec4(0.86f, 0.86f, 0.86f, 1.00f);
        colors[ImGuiCol_TabHovered] = ImVec4(0.78f, 0.78f, 0.78f, 1.00f);
        colors[ImGuiCol_TabActive] = ImVec4(0.74f, 0.74f, 0.74f, 1.00f);
        colors[ImGuiCol_TabUnfocused] = ImVec4(0.90f, 0.90f, 0.90f, 1.00f);
        colors[ImGuiCol_TabUnfocusedActive] = ImVec4(0.84f, 0.84f, 0.84f, 1.00f);

        colors[ImGuiCol_PlotLines] = ImVec4(0.45f, 0.45f, 0.45f, 1.00f);
        colors[ImGuiCol_PlotLinesHovered] = ImVec4(0.25f, 0.25f, 0.25f, 1.00f);
        colors[ImGuiCol_PlotHistogram] = ImVec4(0.45f, 0.45f, 0.45f, 1.00f);
        colors[ImGuiCol_PlotHistogramHovered] = ImVec4(0.25f, 0.25f, 0.25f, 1.00f);

        colors[ImGuiCol_TableHeaderBg] = ImVec4(0.86f, 0.86f, 0.86f, 1.00f);
        colors[ImGuiCol_TableBorderStrong] = ImVec4(0.60f, 0.60f, 0.60f, 1.00f);
        colors[ImGuiCol_TableBorderLight] = ImVec4(0.75f, 0.75f, 0.75f, 1.00f);
        colors[ImGuiCol_TableRowBg] = ImVec4(0.0f, 0.0f, 0.0f, 0.0f);
        colors[ImGuiCol_TableRowBgAlt] = ImVec4(0.0f, 0.0f, 0.0f, 0.05f);

        colors[ImGuiCol_TextSelectedBg] = ImVec4(0.26f, 0.59f, 0.98f, 0.35f);
        colors[ImGuiCol_DragDropTarget] = ImVec4(0.90f, 0.70f, 0.00f, 0.90f);

        colors[ImGuiCol_NavHighlight] = ImVec4(0.26f, 0.59f, 0.98f, 1.00f);
        colors[ImGuiCol_NavWindowingHighlight] = ImVec4(0.10f, 0.10f, 0.10f, 0.70f);
        colors[ImGuiCol_NavWindowingDimBg] = ImVec4(0.20f, 0.20f, 0.20f, 0.35f);
        colors[ImGuiCol_ModalWindowDimBg] = ImVec4(0.20f, 0.20f, 0.20f, 0.35f);
    }

    inline void applyNavyGreen(ImGuiStyle &style)
    {
        applyMetrics(style);

        ImVec4 *colors = style.Colors;

        colors[ImGuiCol_Text] = ImVec4(0.88f, 0.95f, 0.90f, 1.00f);
        colors[ImGuiCol_TextDisabled] = ImVec4(0.50f, 0.65f, 0.58f, 1.00f);

        colors[ImGuiCol_WindowBg] = ImVec4(0.03f, 0.09f, 0.08f, 1.00f);
        colors[ImGuiCol_ChildBg] = ImVec4(0.02f, 0.07f, 0.06f, 1.00f);
        colors[ImGuiCol_PopupBg] = ImVec4(0.03f, 0.09f, 0.08f, 0.98f);
        colors[ImGuiCol_Border] = ImVec4(0.15f, 0.35f, 0.30f, 1.00f);
        colors[ImGuiCol_BorderShadow] = ImVec4(0.0f, 0.0f, 0.0f, 0.0f);

        colors[ImGuiCol_FrameBg] = ImVec4(0.06f, 0.18f, 0.16f, 1.00f);
        colors[ImGuiCol_FrameBgHovered] = ImVec4(0.10f, 0.28f, 0.24f, 1.00f);
        colors[ImGuiCol_FrameBgActive] = ImVec4(0.13f, 0.36f, 0.30f, 1.00f);

        colors[ImGuiCol_TitleBg] = ImVec4(0.02f, 0.06f, 0.05f, 1.00f);
        colors[ImGuiCol_TitleBgActive] = ImVec4(0.05f, 0.16f, 0.14f, 1.00f);
        colors[ImGuiCol_TitleBgCollapsed] = ImVec4(0.02f, 0.05f, 0.04f, 1.00f);
        colors[ImGuiCol_MenuBarBg] = ImVec4(0.03f, 0.11f, 0.09f, 1.00f);

        colors[ImGuiCol_ScrollbarBg] = ImVec4(0.02f, 0.06f, 0.05f, 1.00f);
        colors[ImGuiCol_ScrollbarGrab] = ImVec4(0.18f, 0.45f, 0.38f, 1.00f);
        colors[ImGuiCol_ScrollbarGrabHovered] = ImVec4(0.25f, 0.58f, 0.48f, 1.00f);
        colors[ImGuiCol_ScrollbarGrabActive] = ImVec4(0.32f, 0.70f, 0.58f, 1.00f);

        colors[ImGuiCol_CheckMark] = ImVec4(0.45f, 0.95f, 0.72f, 1.00f);
        colors[ImGuiCol_SliderGrab] = ImVec4(0.20f, 0.62f, 0.50f, 1.00f);
        colors[ImGuiCol_SliderGrabActive] = ImVec4(0.35f, 0.85f, 0.68f, 1.00f);

        colors[ImGuiCol_Button] = ImVec4(0.08f, 0.24f, 0.21f, 1.00f);
        colors[ImGuiCol_ButtonHovered] = ImVec4(0.12f, 0.36f, 0.30f, 1.00f);
        colors[ImGuiCol_ButtonActive] = ImVec4(0.16f, 0.46f, 0.38f, 1.00f);

        colors[ImGuiCol_Header] = ImVec4(0.10f, 0.30f, 0.26f, 1.00f);
        colors[ImGuiCol_HeaderHovered] = ImVec4(0.14f, 0.40f, 0.34f, 1.00f);
        colors[ImGuiCol_HeaderActive] = ImVec4(0.18f, 0.50f, 0.42f, 1.00f);

        colors[ImGuiCol_Separator] = ImVec4(0.20f, 0.45f, 0.38f, 1.00f);
        colors[ImGuiCol_SeparatorHovered] = ImVec4(0.30f, 0.65f, 0.54f, 1.00f);
        colors[ImGuiCol_SeparatorActive] = ImVec4(0.40f, 0.85f, 0.68f, 1.00f);

        colors[ImGuiCol_ResizeGrip] = ImVec4(0.16f, 0.40f, 0.34f, 1.00f);
        colors[ImGuiCol_ResizeGripHovered] = ImVec4(0.25f, 0.58f, 0.48f, 1.00f);
        colors[ImGuiCol_ResizeGripActive] = ImVec4(0.35f, 0.82f, 0.66f, 1.00f);

        colors[ImGuiCol_Tab] = ImVec4(0.05f, 0.16f, 0.14f, 1.00f);
        colors[ImGuiCol_TabHovered] = ImVec4(0.16f, 0.45f, 0.38f, 1.00f);
        colors[ImGuiCol_TabActive] = ImVec4(0.11f, 0.32f, 0.27f, 1.00f);
        colors[ImGuiCol_TabUnfocused] = ImVec4(0.03f, 0.10f, 0.09f, 1.00f);
        colors[ImGuiCol_TabUnfocusedActive] = ImVec4(0.07f, 0.22f, 0.19f, 1.00f);

        colors[ImGuiCol_PlotLines] = ImVec4(0.35f, 0.85f, 0.68f, 1.00f);
        colors[ImGuiCol_PlotLinesHovered] = ImVec4(0.55f, 1.00f, 0.85f, 1.00f);
        colors[ImGuiCol_PlotHistogram] = ImVec4(0.35f, 0.85f, 0.68f, 1.00f);
        colors[ImGuiCol_PlotHistogramHovered] = ImVec4(0.55f, 1.00f, 0.85f, 1.00f);

        colors[ImGuiCol_TableHeaderBg] = ImVec4(0.07f, 0.22f, 0.19f, 1.00f);
        colors[ImGuiCol_TableBorderStrong] = ImVec4(0.30f, 0.55f, 0.47f, 1.00f);
        colors[ImGuiCol_TableBorderLight] = ImVec4(0.15f, 0.35f, 0.30f, 1.00f);
        colors[ImGuiCol_TableRowBg] = ImVec4(0.0f, 0.0f, 0.0f, 0.0f);
        colors[ImGuiCol_TableRowBgAlt] = ImVec4(1.0f, 1.0f, 1.0f, 0.04f);

        colors[ImGuiCol_TextSelectedBg] = ImVec4(0.35f, 0.85f, 0.68f, 0.30f);
        colors[ImGuiCol_DragDropTarget] = ImVec4(0.45f, 0.95f, 0.72f, 0.90f);

        colors[ImGuiCol_NavHighlight] = ImVec4(0.45f, 1.00f, 0.80f, 0.80f);
        colors[ImGuiCol_NavWindowingHighlight] = ImVec4(0.45f, 1.00f, 0.80f, 0.70f);
        colors[ImGuiCol_NavWindowingDimBg] = ImVec4(0.0f, 0.0f, 0.0f, 0.60f);
        colors[ImGuiCol_ModalWindowDimBg] = ImVec4(0.0f, 0.0f, 0.0f, 0.60f);
    }

    inline void applyWine(ImGuiStyle &style)
    {
        applyMetrics(style);

        ImVec4 *colors = style.Colors;

        colors[ImGuiCol_Text] = ImVec4(0.98f, 0.90f, 0.92f, 1.00f);
        colors[ImGuiCol_TextDisabled] = ImVec4(0.65f, 0.45f, 0.50f, 1.00f);

        colors[ImGuiCol_WindowBg] = ImVec4(0.10f, 0.03f, 0.05f, 1.00f);
        colors[ImGuiCol_ChildBg] = ImVec4(0.08f, 0.02f, 0.04f, 1.00f);
        colors[ImGuiCol_PopupBg] = ImVec4(0.10f, 0.03f, 0.05f, 0.98f);
        colors[ImGuiCol_Border] = ImVec4(0.45f, 0.15f, 0.20f, 1.00f);
        colors[ImGuiCol_BorderShadow] = ImVec4(0.0f, 0.0f, 0.0f, 0.0f);

        colors[ImGuiCol_FrameBg] = ImVec4(0.20f, 0.06f, 0.09f, 1.00f);
        colors[ImGuiCol_FrameBgHovered] = ImVec4(0.30f, 0.10f, 0.14f, 1.00f);
        colors[ImGuiCol_FrameBgActive] = ImVec4(0.40f, 0.14f, 0.20f, 1.00f);

        colors[ImGuiCol_TitleBg] = ImVec4(0.08f, 0.02f, 0.03f, 1.00f);
        colors[ImGuiCol_TitleBgActive] = ImVec4(0.20f, 0.06f, 0.09f, 1.00f);
        colors[ImGuiCol_TitleBgCollapsed] = ImVec4(0.06f, 0.02f, 0.03f, 1.00f);
        colors[ImGuiCol_MenuBarBg] = ImVec4(0.12f, 0.04f, 0.06f, 1.00f);

        colors[ImGuiCol_ScrollbarBg] = ImVec4(0.07f, 0.02f, 0.04f, 1.00f);
        colors[ImGuiCol_ScrollbarGrab] = ImVec4(0.42f, 0.16f, 0.22f, 1.00f);
        colors[ImGuiCol_ScrollbarGrabHovered] = ImVec4(0.55f, 0.22f, 0.29f, 1.00f);
        colors[ImGuiCol_ScrollbarGrabActive] = ImVec4(0.68f, 0.28f, 0.36f, 1.00f);

        colors[ImGuiCol_CheckMark] = ImVec4(0.98f, 0.65f, 0.70f, 1.00f);
        colors[ImGuiCol_SliderGrab] = ImVec4(0.60f, 0.22f, 0.30f, 1.00f);
        colors[ImGuiCol_SliderGrabActive] = ImVec4(0.80f, 0.32f, 0.42f, 1.00f);

        colors[ImGuiCol_Button] = ImVec4(0.28f, 0.09f, 0.12f, 1.00f);
        colors[ImGuiCol_ButtonHovered] = ImVec4(0.40f, 0.14f, 0.18f, 1.00f);
        colors[ImGuiCol_ButtonActive] = ImVec4(0.50f, 0.18f, 0.24f, 1.00f);

        colors[ImGuiCol_Header] = ImVec4(0.36f, 0.12f, 0.16f, 1.00f);
        colors[ImGuiCol_HeaderHovered] = ImVec4(0.46f, 0.16f, 0.21f, 1.00f);
        colors[ImGuiCol_HeaderActive] = ImVec4(0.56f, 0.20f, 0.26f, 1.00f);

        colors[ImGuiCol_Separator] = ImVec4(0.45f, 0.16f, 0.22f, 1.00f);
        colors[ImGuiCol_SeparatorHovered] = ImVec4(0.65f, 0.26f, 0.34f, 1.00f);
        colors[ImGuiCol_SeparatorActive] = ImVec4(0.85f, 0.36f, 0.46f, 1.00f);

        colors[ImGuiCol_ResizeGrip] = ImVec4(0.38f, 0.14f, 0.19f, 1.00f);
        colors[ImGuiCol_ResizeGripHovered] = ImVec4(0.55f, 0.22f, 0.29f, 1.00f);
        colors[ImGuiCol_ResizeGripActive] = ImVec4(0.75f, 0.30f, 0.40f, 1.00f);

        colors[ImGuiCol_Tab] = ImVec4(0.20f, 0.06f, 0.09f, 1.00f);
        colors[ImGuiCol_TabHovered] = ImVec4(0.46f, 0.16f, 0.21f, 1.00f);
        colors[ImGuiCol_TabActive] = ImVec4(0.34f, 0.11f, 0.15f, 1.00f);
        colors[ImGuiCol_TabUnfocused] = ImVec4(0.12f, 0.04f, 0.06f, 1.00f);
        colors[ImGuiCol_TabUnfocusedActive] = ImVec4(0.22f, 0.07f, 0.10f, 1.00f);

        colors[ImGuiCol_PlotLines] = ImVec4(0.95f, 0.55f, 0.62f, 1.00f);
        colors[ImGuiCol_PlotLinesHovered] = ImVec4(1.00f, 0.75f, 0.80f, 1.00f);
        colors[ImGuiCol_PlotHistogram] = ImVec4(0.95f, 0.55f, 0.62f, 1.00f);
        colors[ImGuiCol_PlotHistogramHovered] = ImVec4(1.00f, 0.75f, 0.80f, 1.00f);

        colors[ImGuiCol_TableHeaderBg] = ImVec4(0.26f, 0.08f, 0.11f, 1.00f);
        colors[ImGuiCol_TableBorderStrong] = ImVec4(0.55f, 0.20f, 0.27f, 1.00f);
        colors[ImGuiCol_TableBorderLight] = ImVec4(0.35f, 0.12f, 0.17f, 1.00f);
        colors[ImGuiCol_TableRowBg] = ImVec4(0.0f, 0.0f, 0.0f, 0.0f);
        colors[ImGuiCol_TableRowBgAlt] = ImVec4(1.0f, 1.0f, 1.0f, 0.04f);

        colors[ImGuiCol_TextSelectedBg] = ImVec4(0.95f, 0.55f, 0.62f, 0.30f);
        colors[ImGuiCol_DragDropTarget] = ImVec4(0.98f, 0.65f, 0.70f, 0.90f);

        colors[ImGuiCol_NavHighlight] = ImVec4(0.95f, 0.55f, 0.62f, 0.80f);
        colors[ImGuiCol_NavWindowingHighlight] = ImVec4(0.95f, 0.55f, 0.62f, 0.70f);
        colors[ImGuiCol_NavWindowingDimBg] = ImVec4(0.0f, 0.0f, 0.0f, 0.60f);
        colors[ImGuiCol_ModalWindowDimBg] = ImVec4(0.0f, 0.0f, 0.0f, 0.60f);
    }

    inline void applyMonokai(ImGuiStyle &style)
    {
        applyMetrics(style);

        ImVec4 *colors = style.Colors;

        colors[ImGuiCol_Text] = ImVec4(0.97f, 0.97f, 0.95f, 1.00f);
        colors[ImGuiCol_TextDisabled] = ImVec4(0.46f, 0.44f, 0.37f, 1.00f);

        colors[ImGuiCol_WindowBg] = ImVec4(0.15f, 0.15f, 0.13f, 1.00f);
        colors[ImGuiCol_ChildBg] = ImVec4(0.12f, 0.12f, 0.11f, 1.00f);
        colors[ImGuiCol_PopupBg] = ImVec4(0.15f, 0.15f, 0.13f, 0.98f);
        colors[ImGuiCol_Border] = ImVec4(0.29f, 0.28f, 0.24f, 1.00f);
        colors[ImGuiCol_BorderShadow] = ImVec4(0.0f, 0.0f, 0.0f, 0.0f);

        colors[ImGuiCol_FrameBg] = ImVec4(0.24f, 0.24f, 0.20f, 1.00f);
        colors[ImGuiCol_FrameBgHovered] = ImVec4(0.31f, 0.30f, 0.26f, 1.00f);
        colors[ImGuiCol_FrameBgActive] = ImVec4(0.37f, 0.36f, 0.32f, 1.00f);

        colors[ImGuiCol_TitleBg] = ImVec4(0.11f, 0.12f, 0.10f, 1.00f);
        colors[ImGuiCol_TitleBgActive] = ImVec4(0.20f, 0.21f, 0.17f, 1.00f);
        colors[ImGuiCol_TitleBgCollapsed] = ImVec4(0.09f, 0.10f, 0.08f, 1.00f);
        colors[ImGuiCol_MenuBarBg] = ImVec4(0.12f, 0.13f, 0.11f, 1.00f);

        colors[ImGuiCol_ScrollbarBg] = ImVec4(0.11f, 0.12f, 0.10f, 1.00f);
        colors[ImGuiCol_ScrollbarGrab] = ImVec4(0.29f, 0.28f, 0.24f, 1.00f);
        colors[ImGuiCol_ScrollbarGrabHovered] = ImVec4(0.36f, 0.36f, 0.31f, 1.00f);
        colors[ImGuiCol_ScrollbarGrabActive] = ImVec4(0.46f, 0.44f, 0.37f, 1.00f);

        colors[ImGuiCol_CheckMark] = ImVec4(0.65f, 0.89f, 0.18f, 1.00f);
        colors[ImGuiCol_SliderGrab] = ImVec4(0.68f, 0.51f, 1.00f, 1.00f);
        colors[ImGuiCol_SliderGrabActive] = ImVec4(0.40f, 0.85f, 0.94f, 1.00f);

        colors[ImGuiCol_Button] = ImVec4(0.24f, 0.24f, 0.20f, 1.00f);
        colors[ImGuiCol_ButtonHovered] = ImVec4(0.31f, 0.30f, 0.26f, 1.00f);
        colors[ImGuiCol_ButtonActive] = ImVec4(0.37f, 0.36f, 0.32f, 1.00f);

        colors[ImGuiCol_Header] = ImVec4(0.29f, 0.28f, 0.24f, 1.00f);
        colors[ImGuiCol_HeaderHovered] = ImVec4(0.36f, 0.36f, 0.31f, 1.00f);
        colors[ImGuiCol_HeaderActive] = ImVec4(0.46f, 0.44f, 0.37f, 1.00f);

        colors[ImGuiCol_Separator] = ImVec4(0.29f, 0.28f, 0.24f, 1.00f);
        colors[ImGuiCol_SeparatorHovered] = ImVec4(0.46f, 0.44f, 0.37f, 1.00f);
        colors[ImGuiCol_SeparatorActive] = ImVec4(0.65f, 0.89f, 0.18f, 1.00f);

        colors[ImGuiCol_ResizeGrip] = ImVec4(0.29f, 0.28f, 0.24f, 1.00f);
        colors[ImGuiCol_ResizeGripHovered] = ImVec4(0.40f, 0.85f, 0.94f, 1.00f);
        colors[ImGuiCol_ResizeGripActive] = ImVec4(0.65f, 0.89f, 0.18f, 1.00f);

        colors[ImGuiCol_Tab] = ImVec4(0.11f, 0.12f, 0.10f, 1.00f);
        colors[ImGuiCol_TabHovered] = ImVec4(0.29f, 0.28f, 0.24f, 1.00f);
        colors[ImGuiCol_TabActive] = ImVec4(0.20f, 0.21f, 0.17f, 1.00f);
        colors[ImGuiCol_TabUnfocused] = ImVec4(0.12f, 0.13f, 0.11f, 1.00f);
        colors[ImGuiCol_TabUnfocusedActive] = ImVec4(0.17f, 0.18f, 0.15f, 1.00f);

        colors[ImGuiCol_PlotLines] = ImVec4(0.40f, 0.85f, 0.94f, 1.00f);
        colors[ImGuiCol_PlotLinesHovered] = ImVec4(0.65f, 0.89f, 0.18f, 1.00f);
        colors[ImGuiCol_PlotHistogram] = ImVec4(0.65f, 0.89f, 0.18f, 1.00f);
        colors[ImGuiCol_PlotHistogramHovered] = ImVec4(0.99f, 0.59f, 0.12f, 1.00f);

        colors[ImGuiCol_TableHeaderBg] = ImVec4(0.20f, 0.21f, 0.17f, 1.00f);
        colors[ImGuiCol_TableBorderStrong] = ImVec4(0.46f, 0.44f, 0.37f, 1.00f);
        colors[ImGuiCol_TableBorderLight] = ImVec4(0.29f, 0.28f, 0.24f, 1.00f);
        colors[ImGuiCol_TableRowBg] = ImVec4(0.0f, 0.0f, 0.0f, 0.0f);
        colors[ImGuiCol_TableRowBgAlt] = ImVec4(1.0f, 1.0f, 1.0f, 0.04f);

        colors[ImGuiCol_TextSelectedBg] = ImVec4(0.99f, 0.59f, 0.12f, 0.35f);
        colors[ImGuiCol_DragDropTarget] = ImVec4(0.99f, 0.59f, 0.12f, 0.90f);

        colors[ImGuiCol_NavHighlight] = ImVec4(0.99f, 0.59f, 0.12f, 0.80f);
        colors[ImGuiCol_NavWindowingHighlight] = ImVec4(0.99f, 0.59f, 0.12f, 0.70f);
        colors[ImGuiCol_NavWindowingDimBg] = ImVec4(0.0f, 0.0f, 0.0f, 0.60f);
        colors[ImGuiCol_ModalWindowDimBg] = ImVec4(0.0f, 0.0f, 0.0f, 0.60f);
    }

    inline void applyTerminalBlue(ImGuiStyle &style)
    {
        applyMetrics(style);

        ImVec4 *colors = style.Colors;

        colors[ImGuiCol_Text] = ImVec4(0.75f, 0.87f, 1.00f, 1.00f);
        colors[ImGuiCol_TextDisabled] = ImVec4(0.42f, 0.55f, 0.70f, 1.00f);

        colors[ImGuiCol_WindowBg] = ImVec4(0.02f, 0.05f, 0.10f, 1.00f);
        colors[ImGuiCol_ChildBg] = ImVec4(0.015f, 0.035f, 0.075f, 1.00f);
        colors[ImGuiCol_PopupBg] = ImVec4(0.02f, 0.05f, 0.10f, 0.98f);
        colors[ImGuiCol_Border] = ImVec4(0.15f, 0.30f, 0.50f, 1.00f);
        colors[ImGuiCol_BorderShadow] = ImVec4(0.0f, 0.0f, 0.0f, 0.0f);

        colors[ImGuiCol_FrameBg] = ImVec4(0.05f, 0.12f, 0.22f, 1.00f);
        colors[ImGuiCol_FrameBgHovered] = ImVec4(0.08f, 0.18f, 0.32f, 1.00f);
        colors[ImGuiCol_FrameBgActive] = ImVec4(0.12f, 0.25f, 0.42f, 1.00f);

        colors[ImGuiCol_TitleBg] = ImVec4(0.01f, 0.03f, 0.07f, 1.00f);
        colors[ImGuiCol_TitleBgActive] = ImVec4(0.05f, 0.12f, 0.24f, 1.00f);
        colors[ImGuiCol_TitleBgCollapsed] = ImVec4(0.01f, 0.025f, 0.05f, 1.00f);
        colors[ImGuiCol_MenuBarBg] = ImVec4(0.03f, 0.07f, 0.14f, 1.00f);

        colors[ImGuiCol_ScrollbarBg] = ImVec4(0.01f, 0.03f, 0.07f, 1.00f);
        colors[ImGuiCol_ScrollbarGrab] = ImVec4(0.18f, 0.40f, 0.65f, 1.00f);
        colors[ImGuiCol_ScrollbarGrabHovered] = ImVec4(0.25f, 0.52f, 0.80f, 1.00f);
        colors[ImGuiCol_ScrollbarGrabActive] = ImVec4(0.32f, 0.65f, 0.95f, 1.00f);

        colors[ImGuiCol_CheckMark] = ImVec4(0.40f, 0.85f, 1.00f, 1.00f);
        colors[ImGuiCol_SliderGrab] = ImVec4(0.25f, 0.60f, 0.90f, 1.00f);
        colors[ImGuiCol_SliderGrabActive] = ImVec4(0.40f, 0.80f, 1.00f, 1.00f);

        colors[ImGuiCol_Button] = ImVec4(0.07f, 0.17f, 0.30f, 1.00f);
        colors[ImGuiCol_ButtonHovered] = ImVec4(0.10f, 0.24f, 0.42f, 1.00f);
        colors[ImGuiCol_ButtonActive] = ImVec4(0.14f, 0.32f, 0.55f, 1.00f);

        colors[ImGuiCol_Header] = ImVec4(0.09f, 0.22f, 0.38f, 1.00f);
        colors[ImGuiCol_HeaderHovered] = ImVec4(0.13f, 0.30f, 0.50f, 1.00f);
        colors[ImGuiCol_HeaderActive] = ImVec4(0.17f, 0.38f, 0.62f, 1.00f);

        colors[ImGuiCol_Separator] = ImVec4(0.15f, 0.32f, 0.52f, 1.00f);
        colors[ImGuiCol_SeparatorHovered] = ImVec4(0.25f, 0.52f, 0.80f, 1.00f);
        colors[ImGuiCol_SeparatorActive] = ImVec4(0.40f, 0.80f, 1.00f, 1.00f);

        colors[ImGuiCol_ResizeGrip] = ImVec4(0.15f, 0.34f, 0.55f, 1.00f);
        colors[ImGuiCol_ResizeGripHovered] = ImVec4(0.25f, 0.52f, 0.80f, 1.00f);
        colors[ImGuiCol_ResizeGripActive] = ImVec4(0.40f, 0.80f, 1.00f, 1.00f);

        colors[ImGuiCol_Tab] = ImVec4(0.05f, 0.12f, 0.24f, 1.00f);
        colors[ImGuiCol_TabHovered] = ImVec4(0.13f, 0.30f, 0.50f, 1.00f);
        colors[ImGuiCol_TabActive] = ImVec4(0.10f, 0.26f, 0.45f, 1.00f);
        colors[ImGuiCol_TabUnfocused] = ImVec4(0.02f, 0.06f, 0.12f, 1.00f);
        colors[ImGuiCol_TabUnfocusedActive] = ImVec4(0.06f, 0.15f, 0.29f, 1.00f);

        colors[ImGuiCol_PlotLines] = ImVec4(0.40f, 0.85f, 1.00f, 1.00f);
        colors[ImGuiCol_PlotLinesHovered] = ImVec4(0.70f, 0.95f, 1.00f, 1.00f);
        colors[ImGuiCol_PlotHistogram] = ImVec4(0.40f, 0.85f, 1.00f, 1.00f);
        colors[ImGuiCol_PlotHistogramHovered] = ImVec4(0.70f, 0.95f, 1.00f, 1.00f);

        colors[ImGuiCol_TableHeaderBg] = ImVec4(0.06f, 0.16f, 0.30f, 1.00f);
        colors[ImGuiCol_TableBorderStrong] = ImVec4(0.22f, 0.45f, 0.70f, 1.00f);
        colors[ImGuiCol_TableBorderLight] = ImVec4(0.12f, 0.26f, 0.44f, 1.00f);
        colors[ImGuiCol_TableRowBg] = ImVec4(0.0f, 0.0f, 0.0f, 0.0f);
        colors[ImGuiCol_TableRowBgAlt] = ImVec4(1.0f, 1.0f, 1.0f, 0.04f);

        colors[ImGuiCol_TextSelectedBg] = ImVec4(0.25f, 0.55f, 0.95f, 0.35f);
        colors[ImGuiCol_DragDropTarget] = ImVec4(0.40f, 0.85f, 1.00f, 0.90f);

        colors[ImGuiCol_NavHighlight] = ImVec4(0.45f, 0.85f, 1.00f, 0.80f);
        colors[ImGuiCol_NavWindowingHighlight] = ImVec4(0.45f, 0.85f, 1.00f, 0.70f);
        colors[ImGuiCol_NavWindowingDimBg] = ImVec4(0.0f, 0.0f, 0.0f, 0.60f);
        colors[ImGuiCol_ModalWindowDimBg] = ImVec4(0.0f, 0.0f, 0.0f, 0.60f);
    }

    // The ImGui built-in theme: default metrics and the stock dark colors.
    inline void applyImGuiDefault(ImGuiStyle &style)
    {
        style = ImGuiStyle();
        ImGui::StyleColorsDark();
    }

    // Applies a bundled theme by name. Unknown names fall back to Grimsolf,
    // which is also the default.
    inline void apply(const std::string &name = "Grimsolf")
    {
        ImGuiStyle &style = ImGui::GetStyle();

        // Set first: the gray helpers below depend on it.
        lightModeState() = (name == "Light");

        if (name == "Light")
            applyLight(style);
        else if (name == "Navy Green")
            applyNavyGreen(style);
        else if (name == "Wine")
            applyWine(style);
        else if (name == "Monokai")
            applyMonokai(style);
        else if (name == "Terminal Blue")
            applyTerminalBlue(style);
        else if (name == "ImGui Default")
            applyImGuiDefault(style);
        else
            applyGrimsolf(style);

        // Remember the theme foreground so the frame loop can restore it.
        textColorState() = style.Colors[ImGuiCol_Text];
        textDisabledColorState() = style.Colors[ImGuiCol_TextDisabled];
    }

    // Theme gray packed for the ImDrawList calls (syntax highlighting, custom
    // rows), so those colors follow the active theme too.
    inline ImU32 u32(float value, float alpha = 1.0f)
    {
        return ImGui::ColorConvertFloat4ToU32(gray(value, alpha));
    }

    inline ImU32 textU32(float alpha = 1.0f)
    {
        ImVec4 color = textColorState();
        color.w *= alpha;
        return ImGui::ColorConvertFloat4ToU32(color);
    }

    // Theme saved by the World Editor in data/options/realm-editor.json. The
    // tools run as separate processes, so they read that file to follow the
    // theme picked in the editor's Options window. Falls back to Grimsolf when
    // the file/key is missing or holds an unknown name.
    inline std::string savedName()
    {
        json file = Json::loadFromFile("data/options/realm-editor.json");
        if (!file.is_object())
            return "Grimsolf";

        std::string saved = file.value("hud-theme", "Grimsolf");
        for (const auto &name : names())
            if (name == saved)
                return saved;
        return "Grimsolf";
    }

    // Applies the theme saved by the World Editor (Grimsolf when none is saved).
    inline void applySaved()
    {
        apply(savedName());
    }
}
