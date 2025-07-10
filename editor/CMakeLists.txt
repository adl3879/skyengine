cmake_minimum_required(VERSION 3.17)

project(editor VERSION 0.1.0)
set(CMAKE_CXX_STANDARD 20)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")
include(TargetShaders)

file(GLOB_RECURSE SOURCES "src/*.cpp")
file(GLOB_RECURSE HEADERS "src/*.h")

add_executable(editor ${SOURCES} ${HEADERS})
#set_target_properties(editor PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})

include_directories(${VULKAN_INCLUDE_DIR})

target_include_directories(editor PRIVATE 
    ${CMAKE_CURRENT_SOURCE_DIR}/sky/src
    src) 
target_link_libraries(editor PRIVATE sky)

set(SHADERS
    "mesh.vert"
    "mesh.frag"
    "grid.vert"
    "grid.frag"
    "thumbnail_gradient.vert"
    "thumbnail_gradient.frag"
    "transmittance_lut.comp"
    "multiscatter_lut.comp"
    "sky_lut.comp"
    "sky.vert"
    "sky.frag"
    "sprite.vert"
    "sprite.frag"
    "fullscreen_triangle.vert"
    "depth_resolve.frag"
    "skybox.frag"
    "skybox.vert"
    "post_fx.frag"
    "format_converter.frag"
    "imgui.frag"
    "imgui.vert"
    "cubemap.vert"
    "irradiance.frag"
    "brdf_lut.frag"
    "prefilter_envmap.frag"
    "equirect_to_cubemap.vert"
    "equirect_to_cubemap.frag")

set(SKY_SHADERS_DIR "${CMAKE_CURRENT_SOURCE_DIR}/res/shaders/")
list(TRANSFORM SHADERS PREPEND "${SKY_SHADERS_DIR}")

target_shaders(editor ${SHADERS})

# Copy DLL next to the .exe
add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy
            ${CMAKE_SOURCE_DIR}/sky/vendor/assimp/assimp-vc143-mt.dll
            $<TARGET_FILE_DIR:${PROJECT_NAME}>/assimp-vc143-mt.dll
    COMMENT "Copying Assimp DLL")

# Copy 'res' folder next to the .exe
add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
            ${CMAKE_CURRENT_SOURCE_DIR}/res
            $<TARGET_FILE_DIR:${PROJECT_NAME}>/res
    COMMENT "Copying res folder")

# Copy 'shaders' folder next to the .exe
add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
            ${CMAKE_CURRENT_BINARY_DIR}/shaders
            $<TARGET_FILE_DIR:${PROJECT_NAME}>/shaders
    COMMENT "Copying res folder")
