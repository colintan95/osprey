add_executable(gfx_engine
    "app.cpp"
    "app.h"
    "main.cpp")

target_link_libraries(gfx_engine PRIVATE glfw)
target_link_libraries(gfx_engine PRIVATE glm)
target_link_libraries(gfx_engine PRIVATE Vulkan::Vulkan)

# So that source files can specify the full path to header files.
# e.g. 
#   In the window.cpp file, we can use:
#     #include "window/window.h" 
#   instead of:
#     #include "window.h"
#
target_include_directories(gfx_engine PRIVATE "${SRC_INCLUDE_DIRS}")

# TODO(colintan): Don't do this
# In the binary folder, create a symlink to the assets folder
add_custom_command(TARGET gfx_engine POST_BUILD COMMAND ${CMAKE_COMMAND}
    -E create_symlink "${CMAKE_SOURCE_DIR}/assets" 
    "$<TARGET_FILE_DIR:gfx_engine>/assets")
add_custom_command(TARGET gfx_engine POST_BUILD COMMAND ${CMAKE_COMMAND}
    -E create_symlink "${CMAKE_BINARY_DIR}/shaders" 
    "$<TARGET_FILE_DIR:gfx_engine>/shaders")

add_subdirectory(gal)
add_subdirectory(window)