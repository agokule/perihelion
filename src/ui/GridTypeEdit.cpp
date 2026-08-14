#include "ui/GridTypeEdit.hpp"
#include "imgui.h"
#include "ui/imgui_ui_utils.hpp"
#include "FontIcons.hpp"

void GridTypeEdit(GridSettings& grid_settings) {
    RAIIStyleVar s {ImGuiStyleVar_WindowPadding, {4.0f, 4.0f}};
    RAIIStyleVar s2 {ImGuiStyleVar_WindowMinSize, {0, 0}};
    RAIIStyleVar s3 {ImGuiStyleVar_ButtonTextAlign, {0.0f, 0.5f}};
    ImGuiSetNextWindowPos({ .top = 10, .right = 40 });
    RAIIWindow win {"GridTypeEdit", nullptr, ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoDecoration};

    {
        RAIIDisabled d {grid_settings.type == GridType::Flat};
        if (ImGui::Button(NF_MD_GRID))
            grid_settings.type = GridType::Flat;
        ImGui::SetItemTooltip("Display a flat grid");
    }

    ImGui::SameLine();

    {
        RAIIDisabled d {grid_settings.type == GridType::SpacetimeCurved};
        if (ImGui::Button(NF_MD_VECTOR_CURVE))
            grid_settings.type = GridType::SpacetimeCurved;
        ImGui::SetItemTooltip("Display a grid that represents the curvature of spacetime");
    }

    ImGui::SameLine();

    {
        RAIIDisabled d {grid_settings.type == GridType::None};
        if (ImGui::Button(NF_MD_GRID_OFF))
            grid_settings.type = GridType::None;
        ImGui::SetItemTooltip("Don't display any grid, just pure emptiness");
    }
}
