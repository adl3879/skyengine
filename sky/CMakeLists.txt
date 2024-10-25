project(sky)

set(CMAKE_CXX_STANDARD 20)

add_subdirectory(vendor)

file(GLOB_RECURSE SOURCES "src/*.cpp")
file(GLOB_RECURSE HEADERS "src/*.h")

add_library(sky STATIC ${SOURCES} ${HEADERS})

# precompiled header
target_precompile_headers(sky PUBLIC src/skypch.h)

find_package(Vulkan REQUIRED)

target_include_directories(sky
    PUBLIC
    src
    ${Vulkan_INCLUDE_DIR}
    vendor/spdlog/include
    vendor/vma
    vendor/vk-bootstrap/src
    vendor/assimp/include)

target_link_libraries(sky
    PRIVATE 
        Vulkan::Vulkan 
        glfw 
        vk-bootstrap::vk-bootstrap
        ${CMAKE_CURRENT_SOURCE_DIR}/vendor/assimp/assimp-vc143-mt.lib
    PUBLIC glm imgui)