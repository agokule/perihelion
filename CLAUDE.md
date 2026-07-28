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

All simulation-specific state and behavior (physics stepping, camera-follow, 3D drawing, object selection) lives in `SimulationScreen` (`src/SimulationScreen.hpp`/`.cpp`), which `main.cpp` drives explicitly per frame in this order: `simulate_physics` → `update_camera` → `draw_world` → `draw_object_selection_ui`. Object selection is an explicit two-step handshake: `draw_object_selection_ui` returns an `optional<ObjectSelection>` describing what the user picked (via the bottom `ObjectSelector` combo or by clicking a 3D outline), and `main.cpp` is responsible for calling `select_object` with it — this keeps the actual state transition visible in `main` rather than buried in UI-drawing code.

### UI widgets

`src/ui/` holds standalone ImGui widget functions decoupled from `SimulationScreen` (e.g. `ObjectSelector.hpp`/`.cpp`, the bottom-anchored combo + Previous/Next picker). New reusable UI pieces should follow this pattern: a free function taking plain data in and returning the (possibly updated) selection, with no dependency on `SimulationScreen` internals.

### Physics model

- `Object` (`src/Object.hpp`) is a celestial body: mass (kg), radius (light-seconds), position/velocity (`Vector3Double`, light-seconds and light-seconds/s), plus its raylib texture/model and a trail of previous positions for rendering.
- Distances/positions are stored in **light-seconds** everywhere except inside the gravity calculation itself, where they're converted to meters (`convert_light_seconds_to_meters`/`convert_meters_to_light_seconds` in `src/utils.hpp`) so Newton's law of gravitation can be applied with SI units, then converted back.
- `Vector3Double` (`src/Vector3Double.hpp`/`.cpp`) is a double-precision 3D vector used for physics/position math, distinct from raylib's single-precision `Vector3` used for rendering; `to_vector3()` bridges the two at the render boundary.
- Gravity is simulated with a brute-force O(n²) pairwise integrator, substepped `substeps_per_frame` times per rendered frame (see `SimulationScreen::simulate_physics`).

### Presets

`src/Preset.hpp` defines `Preset` (a name + vector of `Object`s) and a static `presets` vector — currently just `"The Solar System"` with real-ish mass/radius/position/velocity values for the Sun through Neptune plus the Moon. `SimulationScreen::load_preset` swaps in a new `Preset` and calls `load_model()` on each `Object` to (re)load its texture and generate its sphere mesh. `src/Preset.cpp` is currently empty (the preset data is header-only).

### Live-tunable values (`adjust.h`)

`dependencies/adjust/adjust.h` is a vendored single-header library that lets tunable values be edited while the app is running: it re-parses the declaring source file at its exact line number and pokes the new literal into the live variable, triggered by `adjust_update()` (which `main.cpp` calls while holding `adjust_live` or pressing `R`). `main.cpp` currently declares its tunables (`delta_time`, `substeps_per_frame`, `text`, `adjust_live`, `selected_sensitivity`, `objects_scale`) with `ADJUST_CONST_*`.

`ADJUST_CONST_*` vs. `ADJUST_VAR_*` only matters for release/production builds (`-DCMAKE_BUILD_TYPE=Release`, i.e. `MODE_PRODUCTION`): `CONST` collapses to a real `const` so the value is fixed in shipped builds, while `VAR` stays a plain mutable variable and remains adjustable/writable even in production. In debug builds both behave identically (a live-editable variable). Use `CONST` for values that should be locked down once tuned (the current codebase uses this for everything); use `VAR` only for something that must stay adjustable (or otherwise mutated at runtime) in a production build too.

A variable declared this way must be long-lived (a local in `main`'s outer scope, a class member, or global) — `adjust.h` stores a pointer to it and writes through that pointer later, so it can't be a short-lived local inside a function called every frame. For that case the header offers `ADJUST_FLOAT(v)`/`ADJUST_INT(v)`/etc., which look up (or create) storage keyed by file+line rather than declaring a variable at all; unused in this codebase currently, but useful for one-off tunables inside a function body. When adding a new tunable, follow the existing `ADJUST_CONST_*` pattern rather than a hardcoded literal.

### Input/camera modes

The camera has two modes driven by `SimulationScreen::current_selected_object`: free-fly (`CAMERA_FREE`, raylib's built-in `UpdateCamera`) when nothing is selected, and a custom orbit-follow mode (`CAMERA_CUSTOM`, hand-rolled spherical-coordinates orbit with `EaseSineIn`-based lerping from `dependencies/reasings`) when an object is selected. Cursor lock state (`DisableCursor`/`EnableCursor`) gates whether mouse movement pans the camera vs. interacts with ImGui.
