#include "ui/DragVelocity.hpp"
#include "imgui.h"
#include "imgui_internal.h"
#include "FontIcons.hpp"
#include "ui/imgui_ui_utils.hpp"
#include <numbers>

namespace {
constexpr double max_velocity = 1;
constexpr double min_velocity = -1;
constexpr double pi = std::numbers::pi;

constexpr float angle_drag_speed = 0.01f;
}

void DragVelocity(Vector3Double& velocity) {
    ImGui::DragScalarN(
            NF_FA_PERSON_RUNNING " Velocity:",
            ImGuiDataType_Double,
            &velocity.x,
            3,
            0.0001f,
            &min_velocity,
            &max_velocity,
            "%le c",
            ImGuiSliderFlags_AlwaysClamp
    );
}

void DragVelocity(Spherical& velocity) {
    // The spherical form is re-derived from the object's Cartesian velocity every
    // frame, and that round trip folds colatitude back into [0, pi] and loses both
    // angles where the direction is degenerate (at a pole, or at zero magnitude).
    // So hold the edited value for as long as a drag is running, and remember the
    // pole-crossing parity: after crossing a pole the colatitude is reflected, and
    // the drag speed has to be negated to keep sweeping the same way in space.
    // None of this state means anything once no drag is active, so it is reset then.
    static Spherical held;
    static bool holding = false;
    static bool flipped = false;

    RAIIID id {"velocity drag polar"};

    Spherical value = holding ? held : velocity;
    bool active = false;

    constexpr int components = 3;
    ImGui::PushMultiItemsWidths(components, ImGui::CalcItemWidth());

    // Each drag ends with the PopItemWidth matching one of the widths pushed above.
    auto end_component = [&] {
        active |= ImGui::IsItemActive();
        ImGui::PopItemWidth();
        ImGui::SameLine(0, ImGui::GetStyle().ItemInnerSpacing.x);
    };

    {
        RAIIID radius_id {0};
        double min = 0.0;
        double max = max_velocity;
        ImGui::DragScalar(
                "",
                ImGuiDataType_Double,
                &value.radius,
                0.0001f,
                &min,
                &max,
                "%le c",
                ImGuiSliderFlags_AlwaysClamp
        );
        end_component();
    }

    {
        RAIIID longitude_id {1};
        double min = -pi;
        double max = pi;
        ImGui::DragScalar(
                "",
                ImGuiDataType_Double,
                &value.longitude,
                angle_drag_speed,
                &min,
                &max,
                "%.3f rad",
                ImGuiSliderFlags_AlwaysClamp | ImGuiSliderFlags_WrapAround
        );
        end_component();
    }

    {
        // Unbounded on purpose: letting the drag run past a pole is what makes the
        // crossing continuous. It is folded back into [0, pi] below.
        RAIIID colatitude_id {2};
        ImGui::DragScalar(
                "",
                ImGuiDataType_Double,
                &value.colatitude,
                flipped ? -angle_drag_speed : angle_drag_speed,
                nullptr,
                nullptr,
                "%.3f rad",
                ImGuiSliderFlags_None
        );
        end_component();
    }

    ImGui::Text(NF_FA_PERSON_RUNNING " Velocity:");

    if (fold_colatitude(value.colatitude)) {
        value.longitude += pi;
        flipped = !flipped;
    }
    value.longitude = wrap_angle(value.longitude);

    held = value;
    holding = active;
    if (!active)
        flipped = false;

    velocity = value;
}
