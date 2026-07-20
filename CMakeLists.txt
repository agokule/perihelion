include(FetchContent)
cmake_minimum_required(VERSION 3.20)

project(Perihelion LANGUAGES CXX)

# Fetch Version 6.0 of Raylib
set(RAYLIB_VERSION 6.0)
FetchContent_Declare(
    raylib
    GIT_REPOSITORY https://github.com/raysan5/raylib.git
    GIT_TAG dbc56a87da87d973a9c5baa4e7438a9d20121d28
    PATCH_COMMAND git restore . && git apply --ignore-whitespace "${CMAKE_CURRENT_SOURCE_DIR}/patches/raylib-increase-camera-pan-speed.patch"
)
FetchContent_GetProperties(raylib)
if (NOT raylib_POPULATED) # Have we downloaded raylib yet?
    set(FETCHCONTENT_QUIET NO)
    FetchContent_MakeAvailable(raylib)
endif()

# Fetch Version 1.92.7 of Dear ImGui
FetchContent_Declare(
    ImGui
    GIT_REPOSITORY https://github.com/ocornut/imgui.git
    GIT_TAG dac07199cfd761113d966eb8ad739254e10df2fe
    FIND_PACKAGE_ARGS
)
FetchContent_MakeAvailable(ImGui)

# Fetch Version 1.92.7 of Dear ImGui
FetchContent_Declare(
    rlImGui
    GIT_REPOSITORY https://github.com/raylib-extras/rlImGui.git
    GIT_TAG 3bc5731c4216bb8caa67fbea24aa85ce80d57ccb
)
FetchContent_MakeAvailable(rlImGui)

# Set output directories
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

# Set C++ standard
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

file(GLOB_RECURSE SOURCES "src/*.cpp" "src/*.h")

add_executable(Perihelion
    ${SOURCES}
    dependencies/adjust/adjust.cpp
    "${imgui_SOURCE_DIR}/imgui.cpp"
    "${imgui_SOURCE_DIR}/imgui_draw.cpp"
    "${imgui_SOURCE_DIR}/imgui_tables.cpp"
    "${imgui_SOURCE_DIR}/imgui_widgets.cpp"
    "${imgui_SOURCE_DIR}/imgui_demo.cpp"
    "${rlimgui_SOURCE_DIR}/rlImGui.cpp"
)
if(MINGW)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -static-libstdc++ -static-libgcc")
endif()

target_compile_definitions(Perihelion PRIVATE $<$<CONFIG:Debug>:DEBUG>)

target_include_directories(Perihelion PRIVATE 
    src
    dependencies/adjust/
    ${imgui_SOURCE_DIR}
    "${imgui_SOURCE_DIR}/misc/cpp/"
    "${rlimgui_SOURCE_DIR}"
)

target_link_libraries(Perihelion PRIVATE raylib)

# on Web targets, we need CMake to generate a HTML webpage. 
if(EMSCRIPTEN)
    # makes the output name index.html
    set(CMAKE_EXECUTABLE_SUFFIX ".html" CACHE INTERNAL "")
    set_target_properties(Perihelion PROPERTIES OUTPUT_NAME "index")

    message("${CMAKE_SOURCE_DIR}html/shell-file.html")
    set(EMMAKE_CXX_CFLAGS "${EMMAKE_CXX_CFLAGS} --shell-file ${CMAKE_SOURCE_DIR}/html/shell-file.html")
    if (DISTRIBUTION_MODE)
        set(EMMAKE_CXX_CFLAGS "${EMMAKE_CXX_CFLAGS} -03")
    endif()
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -s USE_GLFW=3 -s ASSERTIONS=1 -s WASM=1 -s ASYNCIFY -s GL_ENABLE_GET_PROC_ADDRESS=1")
    set_target_properties(Perihelion PROPERTIES LINK_FLAGS "${EMMAKE_CXX_CFLAGS}")
endif()

