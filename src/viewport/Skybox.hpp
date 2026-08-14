#pragma once

#include <raylib.h>
#include <rlgl.h>
#include <string_view>

#if defined(PLATFORM_DESKTOP)
    #define GLSL_VERSION            330
#else   // PLATFORM_ANDROID, PLATFORM_WEB
    #define GLSL_VERSION            100
#endif

// Renders a background skybox from a single equirectangular (panorama)
// image, sampled directly by view direction in the fragment shader (no
// intermediate cubemap, so no resolution loss from baking). Drawn as a
// unit cube every frame so it fills the background behind everything else
// in the scene.
class Skybox {
public:
    explicit Skybox(std::string_view panorama_path);
    ~Skybox();

    Skybox(const Skybox&) = delete;
    Skybox& operator=(const Skybox&) = delete;

    // draws the skybox; call first thing inside BeginMode3D so the rest of
    // the scene draws on top of it
    void draw() const;

private:
    Model skybox_model;
};