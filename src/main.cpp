#include <iostream>

#include "window/window_manager.h"

int main() {
  window::WindowManager window_manager;

  window::Window* window = window_manager.CreateWindow(1920, 1080, "My Window");

  while (!window->ShouldClose()) {
    window->Tick();
    window->SwapBuffers();
  }
  
  return 0;
}