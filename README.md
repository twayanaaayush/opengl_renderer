# Soft Body Dynamics Simulation

A real-time soft body dynamics simulation built with C++ and OpenGL. Implements a pressure-based soft body model using a mass-spring system with Phong lighting and an ImGui control panel.

## Project Structure

```
src/
  main.cpp              Entry point
  app/                  Application + input handling
  core/                 Camera, GameObject, Transform, Geometry
  rendering/            Shader, Mesh, Renderer, Light, Material, buffers
  simulation/           Softbody, Particle, Spring, SimulationParams
  scene/                Scene (grid)
  ui/                   ImGuiLayer, SimulationUI
res/shaders/            GLSL shaders
Linking/                Vendored headers (GLM, GLAD, GLFW, imgui)
```

## Dependencies

- **GLFW 3.3+** - windowing and input
- **OpenGL 3.3** - rendering
- **CMake 3.16+** - build system

GLM, GLAD, GLFW headers, and Dear ImGui are vendored in `Linking/`.

### macOS

```bash
brew install cmake glfw
```

### Linux (Ubuntu/Debian)

```bash
sudo apt install cmake libglfw3-dev
```

## Build

```bash
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build
```

## Run

```bash
cd build
./SoftBodyDynamics
```
