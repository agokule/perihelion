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

struct AutoPosition {};

// int's represent pixel values,
// float's are percentages of the DisplaySize
// AutoPosition is the same as auto in CSS
using Position = std::variant<int, float, AutoPosition>;

// a positioning system that works like position: sticky;
// in CSS.
struct NextWindowPosition {
    Position top = AutoPosition {};
    Position bottom = AutoPosition {};
    Position right = AutoPosition {};
    Position left = AutoPosition {};

    float pivot_x = 0.5f;
    float pivot_y = 0.5f;

    NextWindowPosition& center_vertically();
    NextWindowPosition& center_horizontally();

};

void ImGuiSetNextWindowPos(NextWindowPosition pos, ImGuiCond cond = 0);

