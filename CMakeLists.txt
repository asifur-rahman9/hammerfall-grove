cmake_minimum_required(VERSION 3.20)
project(HammerfallGrove VERSION 2.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
  set(CMAKE_BUILD_TYPE Release CACHE STRING "Build type" FORCE)
endif()

option(EW_BUILD_GAME  "Build the game executable (needs OpenGL, GLEW, GLFW)" ON)
option(EW_BUILD_TESTS "Build the unit tests" ON)

include(FetchContent)
set(FETCHCONTENT_QUIET ON)

# ---------------------------------------------------------------------------
# Dependencies: prefer system packages, fall back to fetching pinned versions.
# ---------------------------------------------------------------------------
find_package(glm CONFIG QUIET)
if(NOT TARGET glm::glm)
  FetchContent_Declare(glm GIT_REPOSITORY https://github.com/g-truc/glm.git GIT_TAG 1.0.1 GIT_SHALLOW TRUE)
  FetchContent_MakeAvailable(glm)
endif()

function(ew_warnings target)
  if(MSVC)
    target_compile_options(${target} PRIVATE /W4)
  else()
    target_compile_options(${target} PRIVATE -Wall -Wextra -Wpedantic -Wshadow)
  endif()
endfunction()

# GL-free game logic: kinematics, AI, physics, rules, OBJ parsing. Unit tested.
add_library(ew_core STATIC
  src/core/ObjLoader.cpp
  src/game/GameState.cpp
  src/game/Kinematics.cpp
  src/game/Player.cpp
  src/game/Sparks.cpp
  src/game/Warden.cpp
  src/game/World.cpp
)
target_include_directories(ew_core PUBLIC src)
target_link_libraries(ew_core PUBLIC glm::glm)
ew_warnings(ew_core)

# ---------------------------------------------------------------------------
# Game
# ---------------------------------------------------------------------------
if(EW_BUILD_GAME)
  find_package(OpenGL REQUIRED)
  find_package(GLEW REQUIRED)

  find_package(glfw3 3.3 CONFIG QUIET)
  if(NOT TARGET glfw)
    set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
    set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
    set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
    set(GLFW_BUILD_WAYLAND OFF CACHE BOOL "" FORCE)
    FetchContent_Declare(glfw GIT_REPOSITORY https://github.com/glfw/glfw.git GIT_TAG 3.4 GIT_SHALLOW TRUE)
    FetchContent_MakeAvailable(glfw)
  endif()

  FetchContent_Declare(imgui GIT_REPOSITORY https://github.com/ocornut/imgui.git GIT_TAG v1.91.9b GIT_SHALLOW TRUE)
  FetchContent_MakeAvailable(imgui)
  add_library(imgui STATIC
    ${imgui_SOURCE_DIR}/imgui.cpp
    ${imgui_SOURCE_DIR}/imgui_draw.cpp
    ${imgui_SOURCE_DIR}/imgui_tables.cpp
    ${imgui_SOURCE_DIR}/imgui_widgets.cpp
    ${imgui_SOURCE_DIR}/backends/imgui_impl_glfw.cpp
    ${imgui_SOURCE_DIR}/backends/imgui_impl_opengl3.cpp
  )
  target_include_directories(imgui PUBLIC ${imgui_SOURCE_DIR} ${imgui_SOURCE_DIR}/backends)
  target_link_libraries(imgui PUBLIC glfw OpenGL::GL)

  add_executable(hammerfall_grove
    src/main.cpp
    src/stb_impl.cpp
    src/app/App.cpp
    src/gfx/Framebuffer.cpp
    src/gfx/Mesh.cpp
    src/gfx/Renderer.cpp
    src/gfx/Shader.cpp
    src/gfx/Texture.cpp
    src/ui/Hud.cpp
  )
  target_include_directories(hammerfall_grove SYSTEM PRIVATE third_party/stb)
  target_link_libraries(hammerfall_grove PRIVATE ew_core imgui glfw GLEW::GLEW OpenGL::GL)
  target_compile_definitions(hammerfall_grove PRIVATE
    EW_SOURCE_DIR="${CMAKE_CURRENT_SOURCE_DIR}"
    EW_FONT_DIR="${imgui_SOURCE_DIR}/misc/fonts"
    GL_SILENCE_DEPRECATION)
  ew_warnings(hammerfall_grove)
  # stb is third-party code: don't let its warnings drown ours.
  set_source_files_properties(src/stb_impl.cpp PROPERTIES COMPILE_OPTIONS "-w")
endif()

# ---------------------------------------------------------------------------
# Tests
# ---------------------------------------------------------------------------
if(EW_BUILD_TESTS)
  enable_testing()
  FetchContent_Declare(doctest GIT_REPOSITORY https://github.com/doctest/doctest.git GIT_TAG v2.5.3 GIT_SHALLOW TRUE)
  FetchContent_MakeAvailable(doctest)

  add_executable(ew_tests
    tests/main.cpp
    tests/test_kinematics.cpp
    tests/test_obj_loader.cpp
    tests/test_game_state.cpp
    tests/test_player.cpp
    tests/test_warden.cpp
  )
  target_link_libraries(ew_tests PRIVATE ew_core doctest::doctest)
  target_compile_definitions(ew_tests PRIVATE EW_SOURCE_DIR="${CMAKE_CURRENT_SOURCE_DIR}")
  ew_warnings(ew_tests)
  add_test(NAME ew_tests COMMAND ew_tests)
endif()
