#include "ui/ObjectSelector.hpp"

#include "FontIcons.hpp"
#include "Object.hpp"
#include "imgui.h"
#include <algorithm>
#include <format>

std::string get_preview_text(const Object& obj) {
    return std::format("{} {}", object_type_to_icon(obj.type), obj.name);
}

int ObjectSelector(const std::vector<Object>& objects, int current_selected) {
    const bool has_objects = !objects.empty();
    int selected = has_objects ? current_selected : -1;

    ImGuiIO& io = ImGui::GetIO();

    constexpr float bottom_margin = 20.0f;

    ImGui::SetNextWindowPos(ImVec2(io.DisplaySize.x * 0.5f, io.DisplaySize.y - bottom_margin),
                             ImGuiCond_Always, ImVec2(0.5f, 1.0f));

    ImGuiWindowFlags flags = ImGuiWindowFlags_NoDecoration
        | ImGuiWindowFlags_AlwaysAutoResize
        | ImGuiWindowFlags_NoMove
        | ImGuiWindowFlags_NoSavedSettings
        | ImGuiWindowFlags_NoFocusOnAppearing
        | ImGuiWindowFlags_NoNav;

    ImGui::Begin("##ObjectSelector", nullptr, flags);

    // -1 (None) is a selectable position: Previous steps down into it,
    // Next steps back out of it. It's the only way to get the free camera.
    ImGui::BeginDisabled(!has_objects || selected < 0);
    if (ImGui::Button(NF_FA_ARROW_LEFT_LONG))
        selected--;
    ImGui::EndDisabled();

    ImGui::SameLine();

    // size the combo to the longest name rather than an arbitrary fixed width
    float combo_width = ImGui::CalcTextSize("None").x;
    for (const auto& obj : objects)
        combo_width = std::max(combo_width, ImGui::CalcTextSize(get_preview_text(obj).c_str()).x);
    combo_width += ImGui::GetFrameHeight() + ImGui::GetStyle().FramePadding.x * 2.0f;

    ImGui::SetNextItemWidth(combo_width);

    std::string preview = (selected >= 0 && selected < (int)objects.size())
        ? get_preview_text(objects[selected]).c_str()
        : "None";

    ImGui::BeginDisabled(!has_objects);
    if (ImGui::BeginCombo("##ObjectSelectorCombo", preview.c_str())) {
        bool none_selected = (selected == -1);
        if (ImGui::Selectable("None", none_selected))
            selected = -1;
        if (none_selected)
            ImGui::SetItemDefaultFocus();

        for (int i = 0; i < (int)objects.size(); i++) {
            bool is_selected = (i == selected);
            if (ImGui::Selectable(get_preview_text(objects[i]).c_str(), is_selected))
                selected = i;
            if (is_selected)
                ImGui::SetItemDefaultFocus();
        }
        ImGui::EndCombo();
    }
    ImGui::EndDisabled();

    ImGui::SameLine();

    ImGui::BeginDisabled(!has_objects || selected >= (int)objects.size() - 1);
    if (ImGui::Button(NF_FA_ARROW_RIGHT_LONG))
        selected++;
    ImGui::EndDisabled();

    ImGui::End();

    return selected;
}
