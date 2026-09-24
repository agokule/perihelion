#version 100

// A copy of raylib's built-in default vertex shader for GRAPHICS_API_OPENGL_ES2
// (see rlLoadShaderDefault in rlgl.h) with one change: `mediump` -> `highp`.
//
// raylib's actual default declares `precision mediump float;`, and browsers'
// WebGL implementations can genuinely compute the vertex transform below at
// that reduced precision (~10-bit mantissa minimum per the GLSL ES spec,
// vs. 23 bits for a full float). Perihelion draws in absolute world-space
// light-seconds that reach into the tens of thousands (Neptune is ~15,000),
// so mediump's ~0.1% relative error on `mvp * vertexPosition` shows up as
// tens of world units of jitter -- worse the farther a vertex is from the
// origin. Desktop GL33 has no precision qualifiers at all (always full
// float), which is why this only shows up on the web build.
//
// WebGL's spec requires implementations to support highp in the vertex
// shader (unlike the fragment shader, where it's optional), so this is safe
// on every conformant browser. Attribute/uniform names match raylib's
// defaults (RL_DEFAULT_SHADER_ATTRIB_NAME_*/RL_DEFAULT_SHADER_UNIFORM_NAME_*)
// so LoadShader's automatic location binding wires it up as a drop-in
// replacement; pass fsFileName = NULL to keep raylib's own default fragment
// shader (its precision doesn't matter here -- it does no position math).
precision highp float;

attribute vec3 vertexPosition;
attribute vec2 vertexTexCoord;
attribute vec4 vertexColor;

varying vec2 fragTexCoord;
varying vec4 fragColor;

uniform mat4 mvp;

void main()
{
    fragTexCoord = vertexTexCoord;
    fragColor = vertexColor;
    gl_Position = mvp*vec4(vertexPosition, 1.0);
}
