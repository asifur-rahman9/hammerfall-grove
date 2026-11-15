# Design notes

Some notes on how the game is put together.

## The idea

The original version was a hammer arm in a field that you had to avoid. I wanted to turn it into an actual game with a goal and a way to lose:

- **The arm** became the guardian of the clearing. Its core glows blue, then amber, then red as it gets ready to attack.
- **The target cube** became an ember you collect.
- **The orbiting lights** became fireflies.
- **Score and knockback** became a score multiplier and three lives.

## Rendering

Each frame is drawn in two passes:

1. **Shadow pass:** draw the scene from the moon's point of view to get a shadow map.
2. **Scene pass:** draw everything to the window with lighting and shadows, then the sky, then the glowing sprites (fireflies, sparks) with additive blending. `GL_FRAMEBUFFER_SRGB` handles gamma correction.

The HUD is drawn on top with Dear ImGui.

Things that helped:

- **One shadow-casting light.** The first version had five lights that each cast shadows, which was slow and looked off. One moonlight with soft shadows looks better and is much cheaper.
- **Instancing.** The grass was originally thousands of separate draw calls. Drawing it with instancing brought that down to a couple of draw calls.

## The arm

- The arm is a chain of four parts, each positioned relative to the one before it.
- When it attacks, it locks onto where you are. There's a small table of strike poses (for distances 12, 16, 20, 24 and 28 from the base), and the arm blends between the two closest ones so the hammer lands close to where you were standing.
- Its behaviour is a simple state machine: idle → tracking → wind-up → strike → recover.

## Code layout

- `src/core` and `src/game` have no OpenGL code, so the game logic can be unit tested.
- `src/gfx` handles shaders, meshes, textures and rendering.
- `src/app` runs the window, main loop and input.

## Bugs fixed from the first version

- Models were lit black because the shader read normals from the wrong attribute.
- The field of view was passed in degrees where radians were expected.
- Memory was allocated every frame and never freed.
- The makefile didn't compile every file, so the build moved to CMake.

## What I'd add next

- Sharper shadows over a bigger area
- Sound effects
- Better models and animation for the arm
