#pragma once

#include "imgui.h"
#include <variant>

struct RAIIWindow {
    RAIIWindow(const char* title, bool* p_open = nullptr, ImGuiWindowFlags flags = 0) {
        ImGui::Begin(title, p_open, flags);
    }

    ~RAIIWindow() {
        ImGui::End();
    }
};

struct RAIIDisabled {
    RAIIDisabled(bool disabled) {
        ImGui::BeginDisabled(disabled);
    }

    ~RAIIDisabled() {
        ImGui::EndDisabled();
    }
};

struct RAIICombo {
    bool should_end;

    RAIICombo(const char* label, const char* preview_value, ImGuiComboFlags flags = 0) {
        should_end = ImGui::BeginCombo(label, preview_value, flags);
    }
    ~RAIICombo() {
        if (should_end)
            ImGui::EndCombo();
    }

    explicit operator bool() const {
        return should_end;
    }
};

struct RAIIStyleVar {
    RAIIStyleVar(ImGuiStyleVar idx, float val) {
        ImGui::PushStyleVar(idx, val);
    }
    RAIIStyleVar(ImGuiStyleVar idx, const ImVec2& val) {
        ImGui::PushStyleVar(idx, val);
    }
    ~RAIIStyleVar() {
        ImGui::PopStyleVar();
    }
};

struct AutoPosition {};

// int's represent pixel values,
// float's are percentages of the DisplaySize
// AutoPosition is the same as auto in CSS
using Position = std::variant<int, float, AutoPosition>;

// A CSS-like positioning system for ImGuiSetNextWindowPos, driven by up to
// four edge offsets (top/bottom/left/right) plus a pivot per axis.
//
// - One edge set on an axis (e.g. only `bottom`): positions the window that
//   many pixels (int) or % of DisplaySize (float) from that edge, anchored
//   using pivot_x/pivot_y as normal. This is like CSS `position: absolute`
//   with only one of `top`/`bottom` set.
// - Both edges set on an axis (e.g. `top` AND `bottom`): like CSS
//   `position: absolute` with both edges set -- the window is stretched to
//   exactly fill the gap between them, and the pivot for that axis is
//   ignored (forced to the near edge). This calls ImGui::SetNextWindowSize
//   internally, so it will fight ImGuiWindowFlags_AlwaysAutoResize on the
//   same axis -- don't set both edges on an axis the window also
//   auto-resizes.
// - Neither edge set on an axis: that axis is left at 0, same as never
//   touching it.
struct NextWindowPosition {
    Position top = AutoPosition {};
    Position bottom = AutoPosition {};
    Position right = AutoPosition {};
    Position left = AutoPosition {};

    float pivot_x = 0.5f;
    float pivot_y = 0.5f;

    // Sets top = 50%, pivot_y = 0.5f. Also resets `bottom` to AutoPosition
    NextWindowPosition& center_vertically();

    // Sets left = 50%, pivot_x = 0.5f. Also resets `right` to AutoPosition
    NextWindowPosition& center_horizontally();
};

// see NextWindowPosition's comment for more details
void ImGuiSetNextWindowPos(NextWindowPosition pos, ImGuiCond cond = 0);

inline NextWindowPosition& cent_horiz(NextWindowPosition&& pos) {
    return pos.center_horizontally();
}
inline NextWindowPosition& cent_vert(NextWindowPosition&& pos) {
    return pos.center_vertically();
}
