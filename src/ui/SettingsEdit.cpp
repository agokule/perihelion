#include "ui/SettingsEdit.hpp"
#include "FontIcons.hpp"
#include "imgui.h"
#include "ui/HelpMarker.hpp"
#include "ui/imgui_ui_utils.hpp"

void SettingsEdit(SimulationSettings& settings, bool& p_open) {
    ImGuiSetNextWindowPos({ .top = 40, .right = 0 });
    RAIIWindow win { NF_COD_SETTINGS_GEAR " Settings", &p_open, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoSavedSettings };

    ImGui::PushItemWidth(40.0f);
    ImGui::DragFloat("Object Scale", &settings.objects_scale, 0.1f, 0.1f, 30.0f, "%.1f");
    HelpMarker("Visually scales objects to be bigger or smaller by this amount, "
               "set to 1 to see things as they are in real life");

    ImGui::SeparatorText("Grid Settings");

    ImGui::DragInt("Flat Grid Lines", &settings.grid.flat_grid_slices, 1, 5, 250);
    HelpMarker("Dimensions of the flat grid when it is displayed");

    ImGui::DragInt("Curved Grid Lines", &settings.grid.curved_grid_slices, 1, 5, 75);
    HelpMarker("Dimensions of the curved grid when it is displayed");

    ImGui::DragInt("Spacing Between Lines", &settings.grid.spacing_between_slices, 1, 1, 50);
    HelpMarker("The space between grid lines (in light-seconds)");

    ImGui::DragInt("Spacetime Curvature Factor", &settings.grid.space_time_curve_factor, 10, 1, 1000);
    HelpMarker("The calculated Spacetime curvature gets scaled by this amount so it is visible");

    ImGui::PopItemWidth();
}

