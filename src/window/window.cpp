#include "window/window.h"

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

namespace window {

Window::Window(int width, int height, const std::string& title)
    : width_(width), height_(height), title_(title) {
  if (!glfwInit()) {
    throw Exception("Could not initialize GLFW.");
  }

  glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);

  glfw_window_ = glfwCreateWindow(width, height, title.c_str(), nullptr, nullptr);
  if (glfw_window_ == nullptr) {
    throw Exception("Could not create GLFW window.");
  }
}

Window::~Window() {
  if (glfw_window_ != nullptr) {
    glfwDestroyWindow(glfw_window_);
  }

  glfwTerminate();
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

VkSurfaceKHR Window::CreateVkSurface(VkInstance vk_instance) {
  VkSurfaceKHR surface;
  if (glfwCreateWindowSurface(vk_instance, glfw_window_, nullptr, &surface) != VK_SUCCESS) {
    throw Exception("GLFW could not create a VkSurface.");
  }
  return surface;
}

} // namespace window