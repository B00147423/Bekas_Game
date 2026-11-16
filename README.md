# Bekas_Game

C++20/raylib game with Box2D physics.

## Source organization

- `src/App/`: application lifecycle and screen/game coordination.
- `src/Game/`: game loop, player orchestration, and the existing camera.
- `src/World/`: world state, chunks, tiles, and world coordination. The existing
  `WorldFog.cpp` remains part of `World`.
- `src/Generation/`: procedural world and noise generation.
- `src/Rendering/`: tileset textures and atlas lookup.
- `src/Navigation/`: flow-field pathfinding.
- `src/Lighting/`: lighting buffers, shadow rendering, scheduling, and settings.
- `src/Physics/`, `src/Animation/`, `src/UI/`: existing physics, animation, and screens.
- `src/shaders/`: shader assets; `src/GameConfig.h`: shared game configuration.
- `assets/`, `external/`, `tools/`, and `lighting_docs/` retain their existing roles.

Cross-module includes use paths relative to `src`, such as
`"Rendering/Tileset.h"` and `"World/Chunk.h"`. Same-folder includes can use the
header filename. CMake exposes `src` as a private include directory and lists
each implementation file explicitly.

Folder names describe responsibility; existing namespaces are preserved.
In particular, the generators remain in `World::Generation`, and `Tileset` and
`FlowField` remain in `World`. Generation and navigation consume world tile/chunk
data; `World` coordinates those modules and physics; `Game` coordinates the game
loop and presentation. The existing `World` class keeps its public API, with
method implementations grouped into these files under `src/World/`:

- `World.cpp`: constructor, `ClearSpawnArea`, `Generate`.
- `WorldRendering.cpp`: `DrawGrassTile`, `isForest`, `GetForestMask`,
  `DrawGround`, `DrawTrees`, `DrawObjects`.
- `WorldShadows.cpp`: `DrawOccluders`, `DrawAutoShadows`.
- `WorldNavigation.cpp`: `DrawDebugGrid`, `DrawFlowField`, `SetFlowFieldTarget`,
  `GetFlowDirection`, `GetSlideDirection`.
- `WorldPhysics.cpp`: `ClearColliders`, `CreateTreeColliders`,
  `CreateObjectCollider`, `CanPlaceObject`, `AddObject`.
- `WorldFog.cpp`: existing destructor, `GenerateFogRegions`, and `UpdateFog`,
  with collider cleanup added to the destructor. `GetExplorationTexture`
  remains inline in `World.h`.

`World.h`, `Chunk.cpp`, `Chunk.h`, and `Tile.h` remain in the same directory.
All implementation files still define methods of `World::World`; the file split
introduced no new classes or algorithm changes.

World-owned tree and object body IDs are tracked by `World`. `ClearColliders`
removes only those bodies through `PhysicsWorld::DestroyBody`, which also erases
their debug records. `Generate` clears old bodies, regenerates trees, and rebuilds
retained object colliders using their stored sizes and offsets. It still resets
terrain and fog; it is not a world-resume operation.

The destructor clears colliders before its existing fog texture cleanup. A cached
Box2D world ID guards cleanup if the owning `PhysicsWorld` has already destroyed
the simulation. Normal generation and object insertion still require a live
`PhysicsWorld`; this guard does not make a destroyed simulation usable again.
The current `Game` member order destroys `World` before `PhysicsWorld`.

## Build

Use a Windows MinGW toolchain compatible with the bundled raylib and Box2D
static libraries. With CMake, Ninja, and MinGW on `PATH`:

```sh
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Debug
cmake --build build --parallel
```

Run `build/BEKAS_GAME.exe` from the project root so that the existing relative
asset and `src/shaders` paths resolve.

## Possible later responsibility splits

These are follow-up opportunities, not changes made by the implementation split:

- `World` still owns terrain/tree/object drawing, forest atlas selection, and
  shadow-caster submission. Rendering and shadow presentation could eventually
  become separate classes while `World` retains state and coordination.
- `World::GetFlowDirection` and `GetSlideDirection` implement steering policy
  alongside the flow-field module. Navigation could eventually own that policy;
  world-space queries and target coordination can remain on `World`.
- `WorldFog.cpp` combines exploration state and region discovery with GPU texture
  allocation/uploads. A future split could separate state from its presentation.
- Collider ownership and cleanup are explicit, but world activation and revisits
  still need a policy for rebuilding colliders without regenerating terrain or
  resetting exploration. Multiple active worlds sharing one `PhysicsWorld`
  still contribute colliders to the same simulation.
- World generation, exploration, and objects are per-instance state, but chunk
  dimensions and gameplay configuration remain shared constants. Future world
  lifecycle/persistence coordination could manage portal destinations without
  adding transition policy to drawing or navigation.
- `Game.cpp` owns player movement/animation/physics synchronization as well as
  render targets, shader uniforms, shadow passes, fog composition, and the
  day/night cycle. Player control and rendering are potential future modules,
  leaving `Game` responsible for sequencing them.

No class splits, namespace changes, or algorithm changes are needed for the
current layout.

