#include "ui/ObjectSelector.hpp"

#include "imgui.h"
#include <algorithm>

int ObjectSelector(const std::vector<std::string>& object_names, int current_selected) {
    const bool has_objects = !object_names.empty();
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

    ImGui::BeginDisabled(!has_objects || selected <= 0);
    if (ImGui::Button("Previous"))
        selected--;
    ImGui::EndDisabled();

    ImGui::SameLine();

    // size the combo to the longest name rather than an arbitrary fixed width
    float combo_width = 0.0f;
    for (const std::string& name : object_names)
        combo_width = std::max(combo_width, ImGui::CalcTextSize(name.c_str()).x);
    combo_width += ImGui::GetFrameHeight() + ImGui::GetStyle().FramePadding.x * 2.0f;

    ImGui::SetNextItemWidth(combo_width);

    const char* preview = (selected >= 0 && selected < (int)object_names.size())
        ? object_names[selected].c_str()
        : "None";

    ImGui::BeginDisabled(!has_objects);
    if (ImGui::BeginCombo("##ObjectSelectorCombo", preview)) {
        for (int i = 0; i < (int)object_names.size(); i++) {
            bool is_selected = (i == selected);
            if (ImGui::Selectable(object_names[i].c_str(), is_selected))
                selected = i;
            if (is_selected)
                ImGui::SetItemDefaultFocus();
        }
        ImGui::EndCombo();
    }
    ImGui::EndDisabled();

    ImGui::SameLine();

    ImGui::BeginDisabled(!has_objects || selected >= (int)object_names.size() - 1);
    if (ImGui::Button("Next"))
        selected++;
    ImGui::EndDisabled();

    ImGui::End();

    return selected;
}
