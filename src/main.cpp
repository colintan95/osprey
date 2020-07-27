#include <iostream>

#include "window/window.h"

int main() {
  window::Window window(1920, 1080, "My Window");

  while (!window.ShouldClose()) {
    window.Tick();
    window.SwapBuffers();
  }
  
  return 0;
}