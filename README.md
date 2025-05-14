# SkyEngine

SkyEngine is a high-performance 3D game engine built from scratch, leveraging Vulkan for rendering. This engine provides a flexible and efficient platform for building and rendering complex 3D scenes.

## Features

- **Custom Renderer**: Uses Vulkan for high-performance, low-level rendering.
- **Entity Component System (ECS)**: Efficient management of game objects and components.
- **Real-time Lighting and Shadows**: Supports dynamic lighting and shadow mapping.
- **Resource Management**: Handles textures, shaders, models, and other assets with efficient memory management.
- **Physics Integration**: (Planned) Integration with a physics engine for realistic simulations.
- **Input System**: Abstraction for keyboard and mouse input.

## Getting Started

### Prerequisites

- **C++ Compiler**: Requires C++17 or newer.
- **Vulkan SDK**: Ensure the [Vulkan SDK](https://vulkan.lunarg.com/) is installed and configured.
- **CMake**: Used for building the project. Make sure you have CMake 3.12 or higher.
- **Dependencies**:
  - **GLFW**: For window and input management.
  - **GLM**: Mathematics library for vector and matrix operations.
  - **Assimp**: For model loading.
  - **ImGui**: Immediate mode GUI library (optional, for debugging and editor UI).

### Installation

Clone the repository:

```bash
git clone --recursive https://github.com/yourusername/skyengine.git
cd skyengine
```
