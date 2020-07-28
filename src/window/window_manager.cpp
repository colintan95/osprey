#include "window/window_manager.h"

#include <GLFW/glfw3.h>

namespace window {

WindowManager::WindowManager() {
  if (!glfwInit()) {
    throw Exception("Failed to initialize GLFW.");
  }
}

WindowManager::~WindowManager() {
  glfwTerminate();
}

Window* WindowManager::CreateWindow(int width, int height, const std::string& title) {
  window_ = std::make_unique<Window>(width, height, title);
  return window_.get();
}

} // namespace window