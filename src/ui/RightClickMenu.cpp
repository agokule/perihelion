#include "ui/RightClickMenu.hpp"
#include <imgui.h>
#include <optional>

std::optional<RightClickActionSelected> RightClickMenu(Vector2 coordinates) {
    std::optional<RightClickActionSelected> selected = std::nullopt;

    // put the menu where the click happened. ImGui only applies this to the
    // window opened next, so it has to come before BeginPopup; Appearing means
    // it is placed each time the popup opens rather than fighting ImGui's own
    // clamping while it is up.
    ImGui::SetNextWindowPos({coordinates.x, coordinates.y}, ImGuiCond_Appearing);

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

