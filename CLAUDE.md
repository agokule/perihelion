# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project overview

Perihelion is a C++20 Newtonian gravity simulator of the solar system, rendered with raylib + Dear ImGui (via rlImGui). It targets both native desktop builds and the web (via Emscripten).

## Build commands

The project uses `just` (see `justfile`) as the command runner, wrapping CMake:

- `just build` — configure + build in Debug mode (default). Pass `Release` for a release build: `just build Release`.
- `just build-web` — same, but cross-compiled with `emcmake`/Emscripten for the web target.
- `just clean` — delete the `build/` directory.
- `just web-server` — serve `build/` over `python -m http.server` (run after `build-web`).
- `just web-open` — open the served web build in a browser.

There is no separate lint step and no test suite in this repo.

CMake uses `FetchContent` to pull raylib 6.0, Dear ImGui, and rlImGui at configure time (see `CMakeLists.txt`); expect the first configure to take a while. raylib is patched at fetch time via `patches/raylib-increase-camera-pan-speed.patch` (bumps `CAMERA_MOVE_SPEED`/`CAMERA_PAN_SPEED`).

Sources are picked up via `file(GLOB_RECURSE SOURCES "src/*.cpp" "src/*.h")`, so a new `.cpp` file anywhere under `src/` is automatically included in the build — no CMakeLists.txt edits needed. This glob matches `.cpp`/`.h` only; header-only types use `.hpp` and are pulled in transitively via `#include`.

## Architecture

### Entry point and app states

`src/main.cpp` owns the window/camera setup and the main loop. It holds an `AppState` (`src/AppState.hpp`) that the loop switches on each frame; currently only `AppState::Simulation` is implemented (`AppState::PresetSelection` is a stubbed-out TODO case for a future preset-picker screen, not yet built).

All simulation-specific state and behavior (physics stepping, camera-follow, 3D drawing, object selection) lives in `SimulationScreen` (`src/SimulationScreen.hpp`/`.cpp`), which `main.cpp` drives explicitly per frame in this order: `simulate_physics` → `update_camera` → `draw_world` (inside `BeginMode3D`/`EndMode3D`, alongside the `Skybox` draw and the in-progress "adding object" preview and velocity-arrow drawing) → `draw_object_selection_ui`. Object selection is an explicit two-step handshake: `draw_object_selection_ui` returns an `optional<ObjectSelection>` describing what the user picked (via the bottom `ObjectSelector` combo or by clicking a 3D outline), and `main.cpp` is responsible for calling `select_object` with it — this keeps the actual state transition visible in `main` rather than buried in UI-drawing code.

`main.cpp` has grown beyond just the loop skeleton: it now also owns the right-click context-menu handshake, the "place a new object" flow, and the click-and-drag velocity-arrow editing (see "Object creation, editing, and velocity dragging" below), plus dev-only toggles for ImGui's demo/metrics windows (`Z`/`X`). 3D-view input handling (right-click, velocity-arrow dragging) is gated behind `!ImGui::GetIO().WantCaptureKeyboard/WantCaptureMouse` so it doesn't fire while a widget has focus.

### UI widgets

`src/ui/` holds standalone ImGui widget functions decoupled from `SimulationScreen` (a free function taking plain data in and returning the (possibly updated) selection, with no dependency on `SimulationScreen` internals). Current widgets:

- `ObjectSelector.hpp`/`.cpp` — the bottom-anchored combo + Previous/Next picker; takes `const std::vector<Object>&` (not just names) so it can show each object's type icon via `object_type_to_icon`.
- `ObjectEditor.hpp`/`.cpp` — a panel for editing the currently-selected `Object`'s name/mass/radius/velocity (via `ImGui::DragScalar`/`DragScalarN`) and color (if it's a solid-color object); returns `true` when the user clicks "Done Editing".
- `RightClickMenu.hpp`/`.cpp` — a popup opened at the right-click position offering `RightClickActionSelected::CreateObject`/`EditObject`/`FocusOnObject` (only `CreateObject` is wired up in `main.cpp` so far; the other two are TODO).
- `SimulationControls.hpp`/`.cpp` — a small bottom-centered play/pause/fast-forward/fast-backward bar (currently mid-integration, uncommitted); fast-forward/backward just halve/double the `delta_time` tunable rather than sub-stepping differently.

`src/ui/imgui_ui_utils.hpp`/`.cpp` underpins all of the above with two things:
- RAII wrappers for ImGui's paired Begin/End calls — `RAIIWindow`, `RAIIDisabled`, `RAIICombo`, `RAIIStyleVar` — so widget code can't forget the matching `End*`/`Pop*` call.
- A CSS-like positioning system for `ImGui::SetNextWindowPos`/`SetNextWindowSize`: build a `NextWindowPosition` with up to four edge offsets (`top`/`bottom`/`left`/`right`, each an `int` pixel offset, `float` percent-of-display, or `AutoPosition{}`) and pass it to `ImGuiSetNextWindowPos`. Setting one edge on an axis anchors to that edge (like CSS `position: absolute` with one of `top`/`bottom` set); setting both edges stretches the window to fill the gap (calls `SetNextWindowSize` internally, so don't combine with `ImGuiWindowFlags_AlwaysAutoResize` on that axis). Helpers `cent_horiz()`/`cent_vert()` wrap the common "centered" case. See the doc comment on `NextWindowPosition` in `imgui_ui_utils.hpp` for the full rules.

### Icon fonts

`src/FontIcons.hpp` defines `NF_*` macros for individual Nerd Font glyphs (look them up at nerdfonts.com/cheat-sheet), used inline in widget labels (e.g. `NF_MD_WEIGHT_KILOGRAM " Mass:"`). `main.cpp` merges `assets/fonts/nerdfont/SymbolsNerdFont-Regular.ttf` into the default ImGui font at startup via `ImFontConfig::MergeMode`, covering the Unicode Supplementary Private Use Area (glyph range `0xF0000`–`0xF0FFF`). This requires `IMGUI_USE_WCHAR32` (defined in `src/ImConfig.hpp`, ImGui's `imconfig.h` override) since those codepoints are above the 16-bit `ImWchar` range.

### Physics model

- `Object` (`src/Object.hpp`/`.cpp`) is a celestial body: `ObjectType` (`Planet`/`Star`/`BlackHole`/`Moon`, used for `ObjectSelector`'s icon and via `object_type_to_icon`), mass (kg), radius (light-seconds), position/velocity (`Vector3Double`, light-seconds and light-seconds/s), a trail of previous positions for rendering, and `drawing_info` — a `std::variant<ObjectTextureInfo, Color>` so an object can either be textured (loaded presets) or a flat solid color (e.g. objects created via the right-click menu, which start out `WHITE`).
- `ObjectTextureInfo` owns the raylib `Texture`/`Model` for a textured object and defines its own copy/move constructors/assignment; `load_model()` (re)generates the sphere mesh and (re)loads the texture. This matters because `SimulationScreen::add_object` pushes into `scene.objects` — a reallocation there copies (not moves) existing `Object`s and drops their loaded texture/model, so `add_object` calls `load_model()` on every object afterward to restore them. `scene.objects` reserves capacity for 100 up front (`load_preset`) specifically to make that reallocation rare.
- Distances/positions are stored in **light-seconds** everywhere except inside the gravity calculation itself, where they're converted to meters (`convert_light_seconds_to_meters`/`convert_meters_to_light_seconds` in `src/utils.hpp`) so Newton's law of gravitation can be applied with SI units, then converted back.
- `Vector3Double` (`src/Vector3Double.hpp`/`.cpp`) is a double-precision 3D vector used for physics/position math, distinct from raylib's single-precision `Vector3` used for rendering; `to_vector3()` bridges the two at the render boundary.
- Gravity is simulated with a brute-force O(n²) pairwise integrator, substepped `substeps_per_frame` times per rendered frame (see `SimulationScreen::simulate_physics`), and is a no-op while `SimulationSettings::paused` is set.

### Object creation, editing, and velocity dragging

Right-clicking in the 3D view (when nothing else has ImGui focus) deselects the current object and opens `RightClickMenu` at the click location. Choosing "New Object" builds a template `Object` (white, `ObjectType::Planet`) that then tracks the mouse along the y=0 plane (`ray_y_plane_intersection`) every frame until the user presses Enter, which calls `SimulationScreen::add_object` and unpauses; this preview-then-confirm object is owned by `main.cpp` as `std::optional<Object> adding_object`, not by `SimulationScreen`. `ObjectEditor` is shown whenever an object is selected, editing it in place by reference.

The selected object's velocity can also be dragged directly in the 3D view: `draw_3d_arrow` (`src/utils.hpp`/`.cpp`) draws an arrow from the object out to `position + velocity * velocity_arrow_scale` and returns a `Cone` (tip/base points + radius) describing its arrowhead. `main.cpp`'s `change_velocity_using_cone` hit-tests the mouse against the cone's screen-space tip/base and, while dragging, re-derives velocity from where the mouse ray hits the y=0 plane. That hit test is done against a *local* camera built from `SimulationScreen::camera_offset_from_selected()` rather than the real `Camera3D`, because raylib's `Camera3D` is single-precision and loses too much precision for accurate mouse-ray math once the followed object is far from the world origin — `camera_offset_from_selected()` keeps that offset in `Vector3Double` instead.

### Presets

`src/Preset.hpp` defines `Preset` (a name + vector of `Object`s) and a static `presets` vector — currently just `"The Solar System"` with real-ish mass/radius/position/velocity values for the Sun through Neptune plus the Moon. `SimulationScreen::load_preset` swaps in a new `Preset` and calls `load_model()` on each `Object` to (re)load its texture and generate its sphere mesh. `src/Preset.cpp` is currently empty (the preset data is header-only).

### Rendering: skybox and spacetime curvature

`Skybox` (`src/Skybox.hpp`/`.cpp`) renders a single equirectangular panorama (`assets/images/myersalex216-space-2638158.jpg`) sampled by view direction in a fragment shader (`assets/shaders/glsl{100,330}/skybox.{vs,fs}`, GLSL version picked by platform), drawn as a unit cube with backface-culling/depth-write disabled so it stays a static backdrop. `main.cpp` calls `skybox.draw()` first thing inside `BeginMode3D`.

`SimulationScreen::draw_world` no longer draws a flat `DrawGrid`; instead `draw_spacetime_curvature` renders a grid warped by Flamm's paraboloid — for each grid vertex it sums each object's contribution to `y` based on Schwarzschild radius and distance (clamped so the paraboloid stays defined), then connects vertices with `DrawLine3D`. The grid follows the camera (floored to grid spacing so it doesn't jitter) and is vertically re-centered each frame around the scene's center of mass, using a one-frame-lagged `max_y_val` to avoid feedback between the recentering and the height calculation it's based on. Tunables live in `GridSettings` (`src/SimulationScreen.hpp`): `curved_grid_slices`, `spacing_between_slices`, `space_time_curve_factor`, and a `GridType` enum (`SpacetimeCurved`/`Flat`) for a possible future flat-grid mode — only `SpacetimeCurved` is implemented.

### Live-tunable values (`adjust.h`)

`dependencies/adjust/adjust.h` is a vendored single-header library that lets tunable values be edited while the app is running: it re-parses the declaring source file at its exact line number and pokes the new literal into the live variable, triggered by `adjust_update()` (which `main.cpp` calls while holding `adjust_live` or pressing `R`). `main.cpp` currently declares its tunables (`delta_time`, `substeps_per_frame`, `text`, `adjust_live`, `selected_sensitivity`, `objects_scale`, `velocity_arrow_scale`) with `ADJUST_CONST_*`.

`SimulationSettings` (`src/SimulationScreen.hpp`) holds these as **references**, not copies — it's constructed once before the main loop rather than freshly every frame, so writes made through it (e.g. `SimulationControls` halving/doubling `delta_time`, or the pause toggle) persist. When adding a new per-frame-tunable value that other code needs to mutate (not just read), add it to `SimulationSettings` by reference alongside the existing fields.

`ADJUST_CONST_*` vs. `ADJUST_VAR_*` only matters for release/production builds (`-DCMAKE_BUILD_TYPE=Release`, i.e. `MODE_PRODUCTION`): `CONST` collapses to a real `const` so the value is fixed in shipped builds, while `VAR` stays a plain mutable variable and remains adjustable/writable even in production. In debug builds both behave identically (a live-editable variable). Use `CONST` for values that should be locked down once tuned (the current codebase uses this for everything); use `VAR` only for something that must stay adjustable (or otherwise mutated at runtime) in a production build too.

A variable declared this way must be long-lived (a local in `main`'s outer scope, a class member, or global) — `adjust.h` stores a pointer to it and writes through that pointer later, so it can't be a short-lived local inside a function called every frame. For that case the header offers `ADJUST_FLOAT(v)`/`ADJUST_INT(v)`/etc., which look up (or create) storage keyed by file+line rather than declaring a variable at all; unused in this codebase currently, but useful for one-off tunables inside a function body. When adding a new tunable, follow the existing `ADJUST_CONST_*` pattern rather than a hardcoded literal.

### Input/camera modes

The camera has two modes driven by `SimulationScreen::current_selected_object`: free-fly (`CAMERA_FREE`, raylib's built-in `UpdateCamera`) when nothing is selected, and a custom orbit-follow mode (`CAMERA_CUSTOM`, hand-rolled spherical-coordinates orbit with `EaseSineIn`-based lerping from `dependencies/reasings`) when an object is selected. Cursor lock state (`DisableCursor`/`EnableCursor`) gates whether mouse movement pans the camera vs. interacts with ImGui; the cursor is also force-locked while placing a new object (`adding_object`), and left unlocked (camera panning suppressed) while dragging the velocity arrow (`changing_velocity_of_obj`).

`K` toggles `SimulationSettings::paused`, which short-circuits `simulate_physics` (the world stops advancing but stays fully drawn/interactive); the in-progress `SimulationControls` widget exposes the same pause state plus fast-forward/fast-backward as a play/pause bar. `Z`/`X` toggle ImGui's demo/metrics windows for development.
