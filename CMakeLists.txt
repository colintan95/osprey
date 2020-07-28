cmake_minimum_required(VERSION 3.15)
project(osprey)

set(CMAKE_CXX_STANDARD 17)

# Vulkan
find_package(Vulkan)
if(Vulkan_FOUND)
  message(STATUS "Found Vulkan")
else()
  message(FATAL_ERROR "Unable to find Vulkan")
endif()

# GLFW
add_library(glfw STATIC IMPORTED)
set_property(TARGET glfw PROPERTY IMPORTED_LOCATION
    "${CMAKE_SOURCE_DIR}/third_party/glfw-3.3.2/lib-vc2019/glfw3.lib")
set_property(TARGET glfw PROPERTY INTERFACE_INCLUDE_DIRECTORIES 
    "${CMAKE_SOURCE_DIR}/third_party/glfw-3.3.2/include")

# GLM
add_library(glm INTERFACE IMPORTED)
set_property(TARGET glm PROPERTY INTERFACE_INCLUDE_DIRECTORIES 
    "${CMAKE_SOURCE_DIR}/third_party/glm-0.9.9.8/include")

# tinyobjloader
add_library(tinyobjloader INTERFACE IMPORTED)
set_property(TARGET tinyobjloader PROPERTY INTERFACE_INCLUDE_DIRECTORIES 
    "${CMAKE_SOURCE_DIR}/third_party/tinyobjloader-2.0.0/include")

# STB
add_library(stb INTERFACE IMPORTED)
set_property(TARGET stb PROPERTY INTERFACE_INCLUDE_DIRECTORIES
    "${CMAKE_SOURCE_DIR}/third_party/stb")

enable_testing()

set(SRC_INCLUDE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/src")

add_subdirectory(shaders)
add_subdirectory(src)