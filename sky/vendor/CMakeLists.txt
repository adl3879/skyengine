cmake_minimum_required(VERSION 3.17)

add_library(vma INTERFACE)

add_subdirectory(glfw)
add_subdirectory(glm)
add_subdirectory(imgui)
add_subdirectory(spdlog)
add_subdirectory(vk-bootstrap)
