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
            0.001,
            &min_velocity,
            &max_velocity,
            "%le c",
            ImGuiSliderFlags_AlwaysClamp
    );
}

void DragVelocity(Spherical& velocity) {
    RAIIID id {"velocity drag polar"};

    constexpr int components = 3;
    ImGui::PushMultiItemsWidths(components, ImGui::CalcItemWidth());

    // Each drag ends with the PopItemWidth matching one of the widths pushed above.
    auto end_component = [] {
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
                &velocity.radius,
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
                &velocity.longitude,
                angle_drag_speed,
                &min,
                &max,
                "%.3f rad",
                ImGuiSliderFlags_AlwaysClamp | ImGuiSliderFlags_WrapAround
        );
        end_component();
    }

    {
        RAIIID colatitude_id {2};
        double min = 0.0;
        double max = pi;
        ImGui::DragScalar(
                "",
                ImGuiDataType_Double,
                &velocity.colatitude,
                angle_drag_speed,
                &min,
                &max,
                "%.3f rad",
                ImGuiSliderFlags_AlwaysClamp
        );
        end_component();
    }

    ImGui::Text(NF_FA_PERSON_RUNNING " Velocity:");
}
