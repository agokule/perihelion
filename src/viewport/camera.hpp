#pragma once

#include <raylib.h>

// modified version of raylib's UpdateCamera function
// so that we have more control over it
void update_camera(Camera *camera, int mode);

