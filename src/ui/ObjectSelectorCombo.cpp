#include "ui/ObjectSelectorCombo.hpp"
#include "imgui.h"
#include "ui/imgui_ui_utils.hpp"
#include <format>

std::string get_preview_text(const Object& obj) {
    return std::format("{} {}", object_type_to_icon(obj.type), obj.name);
}

int ObjectSelectorCombo(const std::vector<Object>& objs, int currently_selected) {
    int selected = currently_selected;
    bool has_objects = !objs.empty();

    // size the combo to the longest name rather than an arbitrary fixed width
    float combo_width = ImGui::CalcTextSize("None").x;
    for (const auto& obj : objs)
        combo_width = std::max(combo_width, ImGui::CalcTextSize(get_preview_text(obj).c_str()).x);
    combo_width += ImGui::GetFrameHeight() + ImGui::GetStyle().FramePadding.x * 2.0f;

    ImGui::SetNextItemWidth(combo_width);

    std::string preview = (selected >= 0 && selected < (int)objs.size())
        ? get_preview_text(objs[selected]).c_str()
        : "None";

    {
        RAIIDisabled d {!has_objects};
        const RAIICombo c {"##ObjectSelectorCombo", preview.c_str()};
        if (c) {
            bool none_selected = (selected == -1);
            if (ImGui::Selectable("None", none_selected)) {
                selected = -1;
                none_selected = true;
            }
            if (none_selected)
                ImGui::SetItemDefaultFocus();

            for (int i = 0; i < (int)objs.size(); i++) {
                bool is_selected = (i == selected);
                if (ImGui::Selectable(get_preview_text(objs[i]).c_str(), is_selected))
                    selected = i;
                if (is_selected)
                    ImGui::SetItemDefaultFocus();
            }
        }
    }

    return selected;
}

