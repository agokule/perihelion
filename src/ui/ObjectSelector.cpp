#include "ui/ObjectSelector.hpp"

#include "FontIcons.hpp"
#include "physics/Object.hpp"
#include "imgui.h"
#include "ui/ObjectSelectorCombo.hpp"
#include "ui/imgui_ui_utils.hpp"

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

    selected = ObjectSelectorCombo(objects, selected);

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
