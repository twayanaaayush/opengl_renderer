cmake_minimum_required(VERSION 3.16)
project(SoftBodyDynamics LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Suppress macOS OpenGL deprecation warnings
add_definitions(-DGL_SILENCE_DEPRECATION)

find_package(glfw3 3.3 REQUIRED)
find_package(OpenGL REQUIRED)
find_package(assimp REQUIRED)

# ImGui sources
set(IMGUI_DIR ${CMAKE_SOURCE_DIR}/Linking/imgui-1.87)
set(IMGUI_SOURCES
    ${IMGUI_DIR}/imgui.cpp
    ${IMGUI_DIR}/imgui_draw.cpp
    ${IMGUI_DIR}/imgui_tables.cpp
    ${IMGUI_DIR}/imgui_widgets.cpp
    ${IMGUI_DIR}/backends/imgui_impl_glfw.cpp
    ${IMGUI_DIR}/backends/imgui_impl_opengl3.cpp
)

set(SOURCES
    src/main.cpp
    src/glad.c
    src/stb_image.cpp

    src/app/Application.cpp
    src/app/InputHandler.cpp

    src/core/Camera.cpp
    src/core/GameObject.cpp
    src/core/Transform.cpp

    src/rendering/Shader.cpp
    src/rendering/Mesh.cpp
    src/rendering/Renderer.cpp
    src/core/Light.cpp
    src/rendering/Material.cpp
    src/rendering/VertexArray.cpp
    src/rendering/VertexBuffer.cpp
    src/rendering/IndexBuffer.cpp
    src/rendering/TextureLoader.cpp
    src/rendering/Model.cpp

    src/simulation/Softbody.cpp
    src/simulation/Particle.cpp
    src/simulation/Spring.cpp
    src/simulation/PhysicsEngine.cpp

    src/scene/Scene.cpp

    src/ui/ImGuiLayer.cpp
    src/ui/SimulationUI.cpp
)

add_executable(${PROJECT_NAME} ${SOURCES} ${IMGUI_SOURCES})

target_include_directories(${PROJECT_NAME} PRIVATE
    ${CMAKE_SOURCE_DIR}/Linking/include
    ${CMAKE_SOURCE_DIR}/Linking/include/stb
    ${CMAKE_SOURCE_DIR}/src/core
    ${CMAKE_SOURCE_DIR}/src/rendering
    ${CMAKE_SOURCE_DIR}/src/simulation
    ${CMAKE_SOURCE_DIR}/src/scene
    ${CMAKE_SOURCE_DIR}/src/app
    ${CMAKE_SOURCE_DIR}/src/ui
    ${IMGUI_DIR}
    ${IMGUI_DIR}/backends
)

target_link_libraries(${PROJECT_NAME} PRIVATE
    glfw
    OpenGL::GL
    assimp::assimp
)

# Copy shader resources next to executable
add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
    ${CMAKE_SOURCE_DIR}/res ${CMAKE_BINARY_DIR}/res
)
