#pragma once

#include "raylib.h"
#include <optional>

enum class RightClickActionSelected {
    CreateObject,
    EditObject,
    FocusOnObject,
};

std::optional<RightClickActionSelected> RightClickMenu(Vector2 coordinates);

