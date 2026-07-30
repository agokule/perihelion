#include "ui/RightClickMenu.hpp"
#include <imgui.h>
#include <optional>

std::optional<RightClickActionSelected> RightClickMenu(Vector2 coordinates) {
    std::optional<RightClickActionSelected> selected = std::nullopt;

    bool popup_open = ImGui::BeginPopup("Right Click Menu");
    if (!popup_open)
        return std::nullopt;

    if (ImGui::Button("New Object"))
        selected = RightClickActionSelected::CreateObject;

    if (ImGui::Button("Edit Object"))
        selected = RightClickActionSelected::EditObject;

    if (ImGui::Button("Focus on Object"))
        selected = RightClickActionSelected::FocusOnObject;

    ImGui::EndPopup();

    if (selected.has_value())
        ImGui::CloseCurrentPopup();

    return selected;
}

