#include "ObjectEditor.hpp"
#include "Constants.hpp"
#include "FontIcons.hpp"
#include "Object.hpp"
#include "Vector3Double.hpp"
#include "ui/ObjectSelectorCombo.hpp"
#include "ui/imgui_ui_utils.hpp"
#include <cmath>
#include <imgui.h>
#include <imgui_stdlib.h>
#include <variant>

using std::holds_alternative;
using std::get;

bool ObjectEditor(int obj_idx, Object& obj, const std::vector<Object>& objs) {
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
    ImGui::SeparatorText("Velocity things");
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
    if (ImGui::Button("Reverse Velocity"))
        obj.velocity = -obj.velocity;

    static int selected = -1;
    ImGui::Text("Select an object (not this one!): ");
    ImGui::SameLine();
    selected = ObjectSelectorCombo(objs, selected);
    if (selected != -1 && selected != obj_idx) {
        const Object& other = objs.at(selected);
        static int axis = 1;
        ImGui::Text("Select an Axis:");
        ImGui::RadioButton("X", &axis, 0);
        ImGui::SameLine();
        ImGui::RadioButton("Y", &axis, 1);
        ImGui::SameLine();
        ImGui::RadioButton("Z", &axis, 2);

        if (ImGui::Button("Orbit this object")) {
            double standard_gravitational_parameter = gravitational_constant * other.mass;
            double distance = obj.position.distance(other.position);
            double velocity_magnitude = sqrt(standard_gravitational_parameter / distance);
            Vector3Double direction = (other.position - obj.position).normalize();

            Vector3 axis_vec {0, 1, 0};
            if (axis == 0)
                axis_vec = {1, 0, 0};
            else if (axis == 2)
                axis_vec = {0, 0, 1};

            Vector3Double new_velocity {Vector3RotateByAxisAngle(direction.to_vector3(), axis_vec, 90) * velocity_magnitude};

            new_velocity += other.velocity;
            obj.velocity = new_velocity;
        }
    }

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

