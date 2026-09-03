#include "ObjectEditor.hpp"
#include "physics/Constants.hpp"
#include "FontIcons.hpp"
#include "physics/Object.hpp"
#include "Spherical.hpp"
#include "Vector3Double.hpp"
#include "ui/DragVelocity.hpp"
#include "ui/ObjectSelectorCombo.hpp"
#include "ui/imgui_ui_utils.hpp"
#include "viewport/utils.hpp"
#include <cmath>
#include <imgui.h>
#include <imgui_stdlib.h>
#include <variant>

using std::holds_alternative;
using std::get;

namespace {

// Direction tangent to the obj->other separation vector, in the plane
// perpendicular to `axis` -- obtained by rotating that separation direction
// 90 degrees about `axis`. Shared by both orbit functions below.
Vector3Double orbit_tangent_direction(const Object& obj, const Object& other, Axis axis) {
    Vector3Double direction = (other.position - obj.position).normalize();
    Vector3 axis_vec = axis_unit_vector(axis).to_vector3();
    // Vector3RotateByAxisAngle takes radians, not degrees.
    return Vector3RotateByAxisAngle(direction.to_vector3(), axis_vec, 90.0f * DEG2RAD);
}

// Circular-orbit velocity for `obj` around `other`, treating `other` as a
// fixed anchor (only correct when other.mass >> obj.mass) and boosting by
// other's own velocity so the result is valid in other's existing frame.
Vector3Double velocity_to_orbit_other(const Object& obj, const Object& other, Axis axis) {
    double standard_gravitational_parameter = gravitational_constant * other.mass;
    double distance = obj.position.distance(other.position);
    double velocity_magnitude = sqrt(standard_gravitational_parameter / distance);

    Vector3Double new_velocity = orbit_tangent_direction(obj, other, axis) * velocity_magnitude;
    new_velocity += other.velocity;
    return new_velocity;
}

struct MutualOrbitVelocities {
    Vector3Double obj_velocity;
    Vector3Double other_velocity;
};

// Circular MUTUAL orbit for a comparable-mass pair: the relative separation
// obeys Kepler's problem with the SUMMED mass (standard two-body reduction),
// then splits that relative velocity around the pair's current combined
// momentum, so an already-drifting pair keeps drifting together instead of
// being reset to rest. Reduces to velocity_to_orbit_other's result (for obj)
// / a barely-perturbed other.velocity (for other) when other.mass >> obj.mass.
MutualOrbitVelocities velocities_to_orbit_eachother(const Object& obj, const Object& other, Axis axis) {
    double total_mass = obj.mass + other.mass;
    double distance = obj.position.distance(other.position);

    Vector3Double com_velocity = (obj.velocity * obj.mass + other.velocity * other.mass) / total_mass;

    double relative_speed = sqrt(gravitational_constant * total_mass / distance);
    Vector3Double relative_velocity = orbit_tangent_direction(obj, other, axis) * relative_speed;

    return MutualOrbitVelocities {
        .obj_velocity   = com_velocity + relative_velocity * (other.mass / total_mass),
        .other_velocity = com_velocity - relative_velocity * (obj.mass / total_mass),
    };
}

} // namespace

bool ObjectEditor(int obj_idx, Object& obj, std::vector<Object>& objs) {
    static VelocityType velocity_type = VelocityType::Polar;

    RAIIWindow win {"Object Editor"};

    double max_mass = 1e40;
    double min_mass = 1e18;

    double max_radius = 5;
    double min_radius = 0.001;

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

    ImGui::RadioButton("Cartesian", (int*)&velocity_type, (int)VelocityType::Cartesian);
    ImGui::SameLine();
    ImGui::RadioButton("Polar", (int*)&velocity_type, (int)VelocityType::Polar);

    if (velocity_type == VelocityType::Polar) {
        Spherical velocity_polar = to_spherical(obj.velocity);
        DragVelocity(velocity_polar);
        obj.velocity = to_cartesian(velocity_polar);
    } else {
        DragVelocity(obj.velocity);
    }

    if (ImGui::Button("Reverse Velocity"))
        obj.velocity = -obj.velocity;
    ImGui::SameLine();
    if (ImGui::Button("Set Velocity to Zero"))
        obj.velocity = {0, 0, 0};

    static int selected = -1;
    ImGui::Text("Select an object (not this one!): ");
    ImGui::SameLine();
    selected = ObjectSelectorCombo(objs, selected);
    if (selected != -1 && selected != obj_idx) {
        Object& other = objs.at(selected);
        static Axis axis = Axis::Y;
        ImGui::Text("Select an Axis:");
        ImGui::RadioButton("X", (int*)&axis, 0);
        ImGui::SameLine();
        ImGui::RadioButton("Y", (int*)&axis, 1);
        ImGui::SameLine();
        ImGui::RadioButton("Z", (int*)&axis, 2);

        if (ImGui::Button("Orbit this object"))
            obj.velocity = velocity_to_orbit_other(obj, other, axis);
        ImGui::SameLine();
        if (ImGui::Button("Orbit eachother")) {
            MutualOrbitVelocities result = velocities_to_orbit_eachother(obj, other, axis);
            obj.velocity = result.obj_velocity;
            other.velocity = result.other_velocity;
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

