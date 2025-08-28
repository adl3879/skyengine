cmake_minimum_required(VERSION 3.20)
project(sky)

# Set C++ standard BEFORE adding subdirectories
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_subdirectory(vendor)

file(GLOB_RECURSE SOURCES "src/*.cpp" 
    "vendor/stb_image/stb_image.cpp"
    "vendor/stb_image/stb_image_write.cpp"
    "vendor/ImGuizmo/ImGuizmo.cpp"
    "vendor/tracy/TracyClient.cpp")
file(GLOB_RECURSE HEADERS "src/*.h")

add_library(sky STATIC ${SOURCES} ${HEADERS})

# precompiled header
target_precompile_headers(sky PUBLIC src/skypch.h)

find_package(Vulkan REQUIRED)

target_include_directories(sky
    PUBLIC
    src
    ${Vulkan_INCLUDE_DIR}
    vendor/glfw/include
    vendor/spdlog/include
    vendor/vma
    vendor/stb_image
    vendor/filewatch
    vendor/vk-bootstrap/src
    vendor/entt/single_include
    vendor/yaml-cpp/include
    vendor/assimp/include
    vendor/tracy/public
    vendor/ImGuizmo
    vendor/jolt)

target_link_libraries(sky
    PRIVATE 
        Vulkan::Vulkan 
        glfw 
        vk-bootstrap::vk-bootstrap
        EnTT::EnTT
        yaml-cpp::yaml-cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/vendor/assimp/assimp-vc143-mt.lib
        Jolt
    PUBLIC 
        glm 
        imgui
        Tracy::TracyClient)
