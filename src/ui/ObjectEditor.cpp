#include "ObjectEditor.hpp"
#include "FontIcons.hpp"
#include "ui/imgui_ui_utils.hpp"
#include <imgui.h>
#include <imgui_stdlib.h>
#include <variant>

using std::holds_alternative;
using std::get;

bool ObjectEditor(Object& obj) {
    RAIIWindow win {"Object Editor"};

    double max_mass = 1e40;
    double min_mass = 1e18;

    double max_radius = 5;
    double min_radius = 0.001;

    double max_velocity = 1;
    double min_velocity = -1;

    ImGui::InputText("Name:", &obj.name);
    ImGui::DragScalar(
            NF_MD_WEIGHT_KILOGRAM " Mass:",
            ImGuiDataType_Double,
            &obj.mass,
            1e19,
            &min_mass,
            &max_mass,
            "%le kg",
            ImGuiSliderFlags_AlwaysClamp
    );
    ImGui::DragScalar(
            NF_MD_RADIUS_OUTLINE " Radius:",
            ImGuiDataType_Double,
            &obj.radius,
            0.001,
            &min_radius,
            &max_radius,
            "%le light-seconds",
            ImGuiSliderFlags_AlwaysClamp
    );
    ImGui::DragScalarN(
            NF_FA_PERSON_RUNNING " Velocity:",
            ImGuiDataType_Double,
            &obj.velocity.x,
            3,
            0.001,
            &min_velocity,
            &max_velocity,
            "%le c",
            ImGuiSliderFlags_AlwaysClamp
    );
    if (holds_alternative<Color>(obj.drawing_info)) {
        Color& color = get<Color>(obj.drawing_info);
        float color_float[4] = {
            color.r / 255.0f,
            color.g / 255.0f,
            color.b / 255.0f,
            color.a / 255.0f
        };
        ImGui::ColorEdit4("Color:", color_float);

        color.r = color_float[0] * 255;
        color.g = color_float[1] * 255;
        color.b = color_float[2] * 255;
        color.a = color_float[3] * 255;
    }

    if (ImGui::Button("Done Editing"))
        return true;

    return false;
}

