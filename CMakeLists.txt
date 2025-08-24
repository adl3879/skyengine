cmake_minimum_required(VERSION 3.17)

project(skyengine VERSION 0.1.0 LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_MSVC_RUNTIME_LIBRARY MultiThreadedDebug)

add_subdirectory(sky)
add_subdirectory(editor)