#include "window/window.h"

namespace window {

Window::Window(int width, int height, const std::string& title)
    : width_(width), height_(height), title_(title) {
  glfwInit();

  glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);

  glfw_window_ = glfwCreateWindow(width, height, title.c_str(), nullptr, nullptr);
  if (glfw_window_ == nullptr) {
    // TODO(colintan): Throw an exception here
  }
}

Window::~Window() {
  if (glfw_window_ != nullptr) {
    glfwDestroyWindow(glfw_window_);
  }
}

void Window::Tick() {
  glfwPollEvents();
}

void Window::SwapBuffers() {
  glfwSwapBuffers(glfw_window_);
}

bool Window::ShouldClose() {
  return glfwWindowShouldClose(glfw_window_);
}

} // namespace window