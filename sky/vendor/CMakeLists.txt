cmake_minimum_required(VERSION 3.17)

add_library(vma INTERFACE)
add_library(filewatch INTERFACE)
add_library(stb_image INTERFACE)

add_subdirectory(glfw)
add_subdirectory(glm)
add_subdirectory(imgui)
add_subdirectory(spdlog)
add_subdirectory(vk-bootstrap)
add_subdirectory(entt)
add_subdirectory(yaml-cpp)