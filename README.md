# Perihelion

This is an n-body gravity simulator made in C++ using Raylib, Dear ImGui, and
Emscripten. It is currently somewhat limited in the features it has, but I have
big plans to turn it into a full on clone of Universe Sandbox on the web!

I was inspired by [this YouTube video](https://www.youtube.com/watch?v=_YbGWoUaZg0).

Watch the [demo video](https://youtu.be/h0ak9L7gRxo).

## How to use it

1. Open the app in your browser. [Here is the link.](https://agokule.github.io/perihelion/)
2. Click anywhere (except the sun) to make your cursor disappear, then you can look around with WASD, Shift, Ctrl, and moving your mouse.
3. You can click on planets in the distance to focus on them, then you can change their properties.
4. To deselect the object, press the X button on the top middle of the screen.
5. You can right click, then click "New Object" to create new objects.
6. You can press <kbd>V</kbd> while focused on a planet to change its velocity.
7. In the object panel, you can also select any object and then make it orbit that object or eachother. 
8. Have fun!

## Cool features

- It displays the **curvature of spacetime**! Can be configured in settings (top right corner)
- Edit the solar system however you want!
- Accurate gravity simulation using Newtonian physics
- Displays the solar system in real scale (have to set "Object Scale" in settings to 1.0 for this)
- Easily configure what is displayed through settings!

## Current limitations

- No black holes
- No collision detection, colliding planets just fly off at high speeds
- Planets and moons don't rotate
- There is only the solar system preset for now
- The simulation starts lagging if you add around 30 objects

## Full List of Controls

- <kbd>K</kbd>: Play/Pause the simulation

### Camera

- <kbd>W</kbd>, <kbd>A</kbd>, <kbd>S</kbd>, <kbd>D</kbd>: Move the camera around (only in free camera mode)
- <kbd>Q</kbd>/<kbd>E</kbd>: Roll the camera side to side (free camera mode)
- <kbd>Space</kbd>/<kbd>Left Control</kbd>: Move up/down
- Holding <kbd>Left Shift</kbd>: Move 3x slower
- <kbd>Esc</kbd>: Leave free camera mode, shows the cursor so that you can access the UI

### While object is selected

- <kbd>V</kbd>: Start changing the velocity (make sure the editor UI is not selected)

### While changing velocity of object

- <kbd>X</kbd>, <kbd>Y</kbd>, and <kbd>Z</kbd>: Lock it to an axis

### While adding new object

- <kbd>Up</kbd>/<kbd>Down</kbd>: Move the new object up/down (hold <kbd>Shift</kbd> to move it more slowly)
- <kbd>Enter</kbd>: Confirm object placement
- Mouse click: go into free camera mode, and you can move around and place it
- <kbd>Esc</kbd>: Leave free camera mode

## Building from Source

Requirements:

- CMake 3.20 or higher
- Git
- A C++23 compatible compiler
- [Just](https://github.com/casey/just), it makes running build commands easier, and if they change it will automatically update
- On Linux, the dependencies mentioned in [their documentation](https://github.com/raysan5/raylib/wiki/Working-on-GNU-Linux)

### Native desktop build

```bash
# builds in debug mode
just build

# builds in release mode
just build release
```

After that, the executable file will be in `./build/bin/Perihelion`.

### Emscripten web build

> [!NOTE]
> You need to have [Emscripten](https://emscripten.org/docs/getting_started/downloads.html) set up for this.

```bash
# builds in debug mode
just build-web

# builds in release mode
just build-web release
```

After that, you need to set up a local web server to serve the files.
If you have Python installed, an easy way to do this is to use the command:

```bash
just web-server
```

You can then view the app in your browser via [localhost](http://localhost:8000/bin/).

## Credits

- **Planet textures:** [Solar System Scope](https://www.solarsystemscope.com/textures/) ([CC BY 4.0](https://creativecommons.org/licenses/by/4.0/)). No modifications were made
- **Background Image:** [Image](/assets/images/myersalex216-space-2638158.jpg) by <a href="https://pixabay.com/users/myersalex216-4979749/?utm_source=link-attribution&utm_medium=referral&utm_campaign=image&utm_content=2638158">Alex Myers</a> from <a href="https://pixabay.com//?utm_source=link-attribution&utm_medium=referral&utm_campaign=image&utm_content=2638158">Pixabay</a>.
- **UI Icons:** [Symbols Nerd Font](https://www.nerdfonts.com/) (see [`assets/fonts/nerdfont/LICENSE`](assets/fonts/nerdfont/LICENSE)).
- **Libraries used:** [raylib](https://github.com/raysan5/raylib), [Dear ImGui](https://github.com/ocornut/imgui), [rlImGui](https://github.com/raylib-extras/rlImGui), [reasings](https://github.com/raylib-extras/reasings), and [adjust.h](https://github.com/bi3mer/adjust.h)
