add_executable(gfx_engine
    "main.cpp")

target_link_libraries(gfx_engine PRIVATE glfw)

# So that source files can specify the full path to header files.
# e.g. 
#   In the window.cpp file, we can use:
#     #include "window/window.h" 
#   instead of:
#     #include "window.h"
#
target_include_directories(gfx_engine PRIVATE "${SRC_INCLUDE_DIRS}")

add_subdirectory(window)