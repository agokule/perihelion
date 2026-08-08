#pragma once

#include "imgui.h"

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

