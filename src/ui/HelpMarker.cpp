#include "HelpMarker.hpp"
#include "imgui.h"
#include "FontIcons.hpp"

void HelpMarker(std::string_view description) {
    ImGui::SameLine();
    ImGui::TextDisabled(NF_COD_QUESTION);
    if (ImGui::BeginItemTooltip())
    {
        ImGui::PushTextWrapPos(ImGui::GetFontSize() * 35.0f);
        ImGui::TextUnformatted(description.data());
        ImGui::PopTextWrapPos();
        ImGui::EndTooltip();
    }
}
