#include "ObjectEditor.hpp"
#include <imgui.h>
#include <imgui_stdlib.h>

bool ObjectEditor(Object& obj) {
    bool done_editiing = false;
    ImGui::Begin("Object Editor");

    double max_mass = 1e40;
    double min_mass = 1e18;

    double max_radius = 5;
    double min_radius = 0.001;

    double max_velocity = 1;
    double min_velocity = 0;

    ImGui::InputText("Name:", &obj.name);
    ImGui::DragScalar(
            "Mass:",
            ImGuiDataType_Double,
            &obj.mass,
            1e19,
            &min_mass,
            &max_mass,
            "%le kg",
            ImGuiSliderFlags_AlwaysClamp
    );
    ImGui::DragScalar(
            "Radius:",
            ImGuiDataType_Double,
            &obj.radius,
            0.001,
            &min_radius,
            &max_radius,
            "%le light-seconds",
            ImGuiSliderFlags_AlwaysClamp
    );
    ImGui::DragScalarN(
            "Velocity:",
            ImGuiDataType_Double,
            &obj.velocity.x,
            3,
            0.001,
            &min_velocity,
            &max_velocity,
            "%le * speed of light",
            ImGuiSliderFlags_AlwaysClamp
    );

    if (ImGui::Button("Done Editing"))
        done_editiing = true;

    ImGui::End();

    return done_editiing;
}

