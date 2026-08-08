#include "imgui_ui_utils.hpp"
#include "imgui.h"
#include <optional>
#include <variant>

using std::get;
using std::holds_alternative;

namespace {

void normalize_position(Position& pos, float size) {
    if (holds_alternative<float>(pos))
        pos = (int)(get<float>(pos) * size);
}

// After normalize_position, a Position is always AutoPosition or a raw pixel
// offset (int) from its respective edge. from_end measures that offset from
// the far edge instead of the near one (used for bottom/right).
std::optional<float> resolve_position(const Position& pos, float display_size, bool from_end) {
    if (holds_alternative<AutoPosition>(pos))
        return std::nullopt;

    float pixels = static_cast<float>(get<int>(pos));
    return from_end ? display_size - pixels : pixels;
}

struct AxisResult {
    float position;
    float pivot;
    std::optional<float> size;
};

// Resolves one axis (top/bottom or left/right) into a position + pivot. If
// both edges are set, the pivot collapses to the near edge and a size is
// returned that stretches the window to exactly fill the gap between them --
// like CSS position: absolute with both `top` and `bottom` set.
AxisResult resolve_axis(const Position& near, const Position& far, float display_size, std::optional<float> pivot) {
    if (!holds_alternative<AutoPosition>(near) && !holds_alternative<AutoPosition>(far)) {
        float near_px = static_cast<float>(get<int>(near));
        float far_px = static_cast<float>(get<int>(far));
        return {near_px, 0.0f, display_size - near_px - far_px};
    }

    AxisResult result {0.0f, 0.0f, std::nullopt};
    if (auto p = resolve_position(near, display_size, false)) {
        result.position = *p;
        if (!pivot)
            result.pivot = 0.0f;
    } else if (auto p = resolve_position(far, display_size, true)) {
        result.position = *p;
        if (!pivot)
            result.pivot = 1.0f;
    }

    if (pivot)
        result.pivot = *pivot;
    return result;
}

}

void ImGuiSetNextWindowPos(NextWindowPosition& pos, ImGuiCond cond) {
    const auto& io = ImGui::GetIO();

    normalize_position(pos.top, io.DisplaySize.y);
    normalize_position(pos.bottom, io.DisplaySize.y);
    normalize_position(pos.left, io.DisplaySize.x);
    normalize_position(pos.right, io.DisplaySize.x);

    AxisResult y = resolve_axis(pos.top, pos.bottom, io.DisplaySize.y, pos.pivot_y);
    AxisResult x = resolve_axis(pos.left, pos.right, io.DisplaySize.x, pos.pivot_x);

    ImGui::SetNextWindowPos({x.position, y.position}, cond, {x.pivot, y.pivot});

    // 0.0f on an axis tells ImGui to leave that axis auto-fit; only override
    // the axes that are actually being stretched between two edges.
    if (x.size || y.size)
        ImGui::SetNextWindowSize({x.size.value_or(0.0f), y.size.value_or(0.0f)}, cond);
}

NextWindowPosition& NextWindowPosition::center_vertically() {
    top = 0.5f;
    bottom = AutoPosition{};
    pivot_y = 0.5f;
    return *this;
}

NextWindowPosition& NextWindowPosition::center_horizontally() {
    left = 0.5f;
    right = AutoPosition{};
    pivot_x = 0.5f;
    return *this;
}

