#include "Skybox.hpp"
#include "raylib.h"

#include <raymath.h>
#include <string>

Skybox::Skybox(std::string_view panorama_path) {
    Mesh cube = GenMeshCube(1.0f, 1.0f, 1.0f);
    skybox_model = LoadModelFromMesh(cube);

    skybox_model.materials[0].shader = LoadShader(
        TextFormat("./assets/shaders/glsl%i/skybox.vs", GLSL_VERSION),
        TextFormat("./assets/shaders/glsl%i/skybox.fs", GLSL_VERSION)
    );

    // texture unit the panorama will be bound to below (MATERIAL_MAP_ALBEDO,
    // matching the material map slot it's assigned to)
    int equirectangular_slot = MATERIAL_MAP_ALBEDO;
    SetShaderValue(skybox_model.materials[0].shader, GetShaderLocation(skybox_model.materials[0].shader, "equirectangularMap"), &equirectangular_slot, SHADER_UNIFORM_INT);

    std::string path { panorama_path };
    Image panorama_image = LoadImage(path.c_str());
    skybox_model.materials[0].maps[MATERIAL_MAP_ALBEDO].texture = LoadTextureFromImage(panorama_image);
    UnloadImage(panorama_image);
}

Skybox::~Skybox() {
    UnloadShader(skybox_model.materials[0].shader);
    UnloadTexture(skybox_model.materials[0].maps[MATERIAL_MAP_ALBEDO].texture);
    UnloadModel(skybox_model);
}

void Skybox::draw() const {
    // we're inside the cube, so the usual backface culling/depth write
    // would hide it or let it clobber depth for everything drawn after
    rlDisableBackfaceCulling();
    rlDisableDepthMask();
    DrawModel(skybox_model, Vector3Zero(), 1.0f, WHITE);
    rlEnableBackfaceCulling();
    rlEnableDepthMask();
}