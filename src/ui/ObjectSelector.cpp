#include "ui/ObjectSelector.hpp"

#include "FontIcons.hpp"
#include "Object.hpp"
#include "imgui.h"
#include "ui/imgui_ui_utils.hpp"
#include <algorithm>
#include <format>

std::string get_preview_text(const Object& obj) {
    return std::format("{} {}", object_type_to_icon(obj.type), obj.name);
}

int ObjectSelector(const std::vector<Object>& objects, int current_selected) {
    RAIIStyleVar s1 {ImGuiStyleVar_WindowPadding, {4.0f, 4.0f}};
    const bool has_objects = !objects.empty();
    int selected = has_objects ? current_selected : -1;

    ImGuiIO& io = ImGui::GetIO();

    ImGuiSetNextWindowPos(cent_horiz({ .top = 10 }));

    ImGuiWindowFlags flags = ImGuiWindowFlags_NoDecoration
        | ImGuiWindowFlags_AlwaysAutoResize
        | ImGuiWindowFlags_NoMove
        | ImGuiWindowFlags_NoSavedSettings
        | ImGuiWindowFlags_NoFocusOnAppearing
        | ImGuiWindowFlags_NoNav;

    RAIIWindow win {"##ObjectSelector", nullptr, flags};

    // -1 (None) is a selectable position: Previous steps down into it,
    // Next steps back out of it. It's the only way to get the free camera.
    {
        RAIIDisabled d {!has_objects || selected < 0};
        if (ImGui::Button(NF_FA_ARROW_LEFT_LONG))
            return selected - 1;
    }

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

            for (int i = 0; i < (int)objects.size(); i++) {
                bool is_selected = (i == selected);
                if (ImGui::Selectable(get_preview_text(objects[i]).c_str(), is_selected))
                    selected = i;
                if (is_selected)
                    ImGui::SetItemDefaultFocus();
            }
        }
    }

    ImGui::SameLine();
    {
        RAIIDisabled d {selected == -1};
        if (ImGui::Button(NF_OCT_X))
            return -1;
        ImGui::SetItemTooltip("Unselect the object, or let the camera be free");
    }


    ImGui::SameLine();
    {
        RAIIDisabled d {!has_objects || selected >= (int)objects.size() - 1};
        if (ImGui::Button(NF_FA_ARROW_RIGHT_LONG))
            return selected + 1;
    }

    return selected;
}
