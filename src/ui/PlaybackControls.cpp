#include "PlaybackControls.hpp"
#include "imgui.h"
#include "FontIcons.hpp"
#include "ui/imgui_ui_utils.hpp"
#include <algorithm>
#include <array>
#include <format>
#include <iterator>
#include <string_view>

bool PlaybackControls(bool playing, float& speed) {
    static std::array<float, 9> speeds {
        0.25f,
        0.50f,
        0.75f,
        1.00f,
        1.25f,
        1.50f,
        1.75f,
        2.00f,
        2.25f
    };
    auto it = std::find(speeds.begin(), speeds.end(), speed);

    RAIIStyleVar s1 {ImGuiStyleVar_WindowPadding, {4.0f, 4.0f}};
    RAIIStyleVar s2 {ImGuiStyleVar_WindowMinSize, {0, 0}};
    ImGuiSetNextWindowPos(cent_horiz({ .bottom = 20 }));

    RAIIWindow win {"play", nullptr,
            ImGuiWindowFlags_NoResize
            | ImGuiWindowFlags_NoDecoration
            | ImGuiWindowFlags_NoFocusOnAppearing
            | ImGuiWindowFlags_NoSavedSettings
    };

    {
        RAIIDisabled d {it == speeds.begin()};
        if (ImGui::Button(NF_FA_FAST_BACKWARD))
            speed = *std::prev(it);
        ImGui::SetItemTooltip("Make it go slower");
    }
    ImGui::SameLine();

    std::string_view text = playing ? NF_FA_PAUSE : NF_FA_PLAY;
    if (ImGui::Button(text.data()))
        return !playing;
    ImGui::SetItemTooltip("Play/Pause (or press K)");

    {
        RAIIDisabled d {it == std::prev(speeds.end())};
        ImGui::SameLine();
        if (ImGui::Button(NF_FA_FAST_FORWARD))
            speed = *std::next(it);
        ImGui::SetItemTooltip("Make it go faster");
    }

    ImGui::Text("%s", std::format("{}x", speed).c_str());

    return playing;
}
