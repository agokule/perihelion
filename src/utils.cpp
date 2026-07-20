#include "utils.hpp"
#include "math.h"

void draw_text_centered(const std::string& text, Vector2 pos, int font_size, Color color) {
    auto [width, height] = MeasureTextEx(GetFontDefault(), text.c_str(), font_size, 2);

    Vector2 new_pos = { pos.x - width / 2, pos.y - height / 2 };
    DrawText(text.c_str(), new_pos.x, new_pos.y, font_size, color);
}

bool IsObjectInCamera(Vector3 objectPos, const Camera3D& camera) {
    // 1. Calculate the camera's local coordinate axes
    Vector3 cameraForward = Vector3Normalize(Vector3Subtract(camera.target, camera.position));
    Vector3 cameraRight = Vector3Normalize(Vector3CrossProduct(cameraForward, camera.up));
    Vector3 cameraUp = Vector3CrossProduct(cameraRight, cameraForward); // True orthogonal up

    // 2. Vector from camera to the target object
    Vector3 toObject = Vector3Subtract(objectPos, camera.position);

    // 3. Project the object position onto the camera's axes (Local/View Space coordinates)
    float depth = Vector3DotProduct(toObject, cameraForward);
    float hOffset = Vector3DotProduct(toObject, cameraRight); // Horizontal distance from center
    float vOffset = Vector3DotProduct(toObject, cameraUp);    // Vertical distance from center

    // Behind or exactly on the plane of the camera lens? Not visible.
    if (depth <= 0.0f) return false;

    // 4. Calculate aspect ratio (e.g., 1920/1080 = 1.7777)
    float aspectRatio = (float)GetScreenWidth() / (float)GetScreenHeight();

    // 5. Calculate visible boundaries at this specific object depth
    float fovRadians = camera.fovy * DEG2RAD;
    
    // Half-height of the screen at this distance
    float halfHeightLimit = depth * tanf(fovRadians * 0.5f);
    // Half-width of the screen scaled by aspect ratio
    float halfWidthLimit = halfHeightLimit * aspectRatio;

    // 6. Final boundary checks (with absolute values)
    bool visibleVertically = (fabsf(vOffset) <= halfHeightLimit);
    bool visibleHorizontally = (fabsf(hOffset) <= halfWidthLimit);

    return (visibleVertically && visibleHorizontally);
}

