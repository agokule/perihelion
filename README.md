# Perihelion

A Newtonian gravity simulator of the solar system, written in C++23 with [raylib](https://www.raylib.com/) and [Dear ImGui](https://github.com/ocornut/imgui). It runs natively on the desktop and in the browser through Emscripten.

https://github.com/user-attachments/assets/3da515ea-ae87-4aed-8ccd-2482502588c6

**[▶ Try it in your browser](https://agokule.github.io/perihelion/)**

The project was inspired by [this YouTube video](https://www.youtube.com/watch?v=_YbGWoUaZg0).

## Features

- **Real-scale solar system.** The Sun, the eight planets and the Moon start with real-world masses, radii, positions and orbital velocities, and orbit each other under Newton's law of universal gravitation.
- **Build your own systems.** Spawn new bodies anywhere in space, then edit their name, mass, radius, color and velocity while the simulation runs.
- **Drag velocities in 3D.** Grab a body's velocity arrow and pull it around with the mouse. You can lock the drag to the X, Y or Z axis.
- **One-click orbits.** Pick any other body and Perihelion gives the selected one the velocity it needs for a circular orbit around it.
- **Spacetime grid.** A grid warped by every body's mass (based on Flamm's paraboloid) shows the gravity wells. You can switch to a flat grid or hide it.
- **Playback controls.** Pause, resume, and run the simulation from 0.25× to 2.25× speed.
- **Two camera modes.** Fly freely around the system, or select a body and the camera smoothly turns toward it and then orbits it as it moves.
- **Live settings panel.** Tune object scale, grid density, spacing and curvature from the UI.

## Controls

### Camera

Left-click in the 3D view to capture the mouse for camera control, and press <kbd>Esc</kbd> to release it so you can use the UI.

| Input | Free camera (nothing selected) | Orbit camera (body selected) |
| --- | --- | --- |
| Mouse | Look around | Orbit around the body |
| Scroll wheel | Zoom | Zoom in / out |
| <kbd>W</kbd> <kbd>A</kbd> <kbd>S</kbd> <kbd>D</kbd> | Fly forward / left / back / right | — |
| <kbd>Space</kbd> / <kbd>Left Ctrl</kbd> | Fly up / down | — |
| <kbd>Q</kbd> / <kbd>E</kbd> | Roll | — |
| Middle mouse drag | Pan | — |
| Hold <kbd>Left Shift</kbd> | Move 3× slower | Move 3× slower |

### Simulation

| Input | Action |
| --- | --- |
| <kbd>K</kbd> | Pause / resume |
| Playback bar (bottom center) | Play / pause, change simulation speed |
| Object picker (bottom) | Select a body, step to the previous or next one, or deselect it with **X** |
| Click a body's outline | Select that body |
| Grid buttons (top right) | Switch between the curved spacetime grid, a flat grid, and no grid |
| Gear icon (top right) | Open the settings panel |

### Creating and editing bodies

1. **Create:** with the cursor released, right-click in the 3D view and choose **New Object**. The simulation pauses and a green placement grid appears.
2. **Place:** move the mouse to position the body. Hold <kbd>↑</kbd> / <kbd>↓</kbd> to raise or lower the placement plane (hold <kbd>Left Shift</kbd> to move it more slowly), then press <kbd>Enter</kbd> to drop the body into the simulation.
3. **Edit:** select a body to open the editor panel. There you can change its name, mass, radius, velocity and color, reverse its velocity, or put it into a circular orbit around another body around the X, Y or Z axis.
4. **Drag its velocity:** with a body selected and the cursor released, press <kbd>V</kbd> to grab its velocity arrow and move the mouse to reshape it. Press <kbd>X</kbd>, <kbd>Y</kbd> or <kbd>Z</kbd> to lock the drag to that axis, then left-click to confirm.

## Building from source

### Prerequisites

- [CMake](https://cmake.org/) 3.20 or newer
- A C++23-capable compiler (GCC, Clang or MSVC)
- Git. CMake fetches raylib 6.0, Dear ImGui and rlImGui automatically on the first configure, so that first run takes a while.
- [just](https://github.com/casey/just) (optional; it wraps the CMake commands below)
- On Linux, the X11/OpenGL development packages that raylib needs. See [raylib's Linux guide](https://github.com/raysan5/raylib/wiki/Working-on-GNU-Linux).
- For the web build only: [Emscripten](https://emscripten.org/docs/getting_started/downloads.html)

### Desktop

```sh
just build            # Debug build
just build Release    # Release build
```

Without `just`:

```sh
cmake -DCMAKE_BUILD_TYPE=Release -S . -B build
cmake --build build --config Release
```

The executable is written to `build/bin/`. Run it **from the repository root**, because textures, shaders and fonts are loaded from `./assets` relative to the working directory:

```sh
./build/bin/Perihelion
```

### Web

```sh
just build-web Release   # build into build-web/ with Emscripten
just web-server          # serve build-web/ on http://localhost:8000
just web-open            # open http://localhost:8000/bin/ in your browser
```

The web build uses its own `build-web/` directory, so it can sit alongside the native `build/`. `just clean` deletes both.

Pushing a version tag such as `1.2.3` or `1.2.3-rc.1` triggers the GitHub Actions workflow, which builds the web version and deploys it to GitHub Pages.

## How it works

- **Units.** Distances are measured in light-seconds and velocities in light-seconds per second. The gravitational constant is stored in those units (≈ 2.477 × 10⁻³⁶ ls³ kg⁻¹ s⁻²), so the physics never converts to or from meters. Positions and velocities use double precision. They are converted to single precision only when drawn.
- **Integration.** Every pair of bodies attracts each other (an O(n²) brute-force sum). Each step first updates every body's velocity and then every position (semi-implicit Euler). Each rendered frame runs several of these substeps for stability.
- **Spacetime grid.** Each grid vertex is pulled down according to every body's Schwarzschild radius and its distance from that body, following Flamm's paraboloid. This is a visualization only: the motion itself is purely Newtonian.

## Project layout

```
src/
├── main.cpp          # window setup, main loop, input handling
├── physics/          # Object, SimulationScreen (stepping, camera follow, drawing), settings, constants
├── ui/               # ImGui widgets: object picker/editor, playback bar, settings, right-click menu
├── viewport/         # camera controller, skybox, 3D drawing helpers
└── Preset.hpp        # the built-in solar system preset
assets/               # planet textures, skybox panorama, shaders, icon font
dependencies/         # vendored single-header libraries (adjust.h, reasings)
html/shell-file.html  # Emscripten page template for the web build
```

## Credits

- **Planet textures:** [Solar System Scope](https://www.solarsystemscope.com/textures/), licensed under [CC BY 4.0](https://creativecommons.org/licenses/by/4.0/). No modifications.
- **Skybox panorama:** [Image](/assets/images/myersalex216-space-2638158.jpg) by <a href="https://pixabay.com/users/myersalex216-4979749/?utm_source=link-attribution&utm_medium=referral&utm_campaign=image&utm_content=2638158">Alex Myers</a> from <a href="https://pixabay.com//?utm_source=link-attribution&utm_medium=referral&utm_campaign=image&utm_content=2638158">Pixabay</a>.
- **Icons:** [Symbols Nerd Font](https://www.nerdfonts.com/) (see [`assets/fonts/nerdfont/LICENSE`](assets/fonts/nerdfont/LICENSE)).
- **Libraries:** [raylib](https://github.com/raysan5/raylib), [Dear ImGui](https://github.com/ocornut/imgui), [rlImGui](https://github.com/raylib-extras/rlImGui), [reasings](https://github.com/raylib-extras/reasings), and [adjust.h](dependencies/adjust/adjust.h) by Colan Biemer.
