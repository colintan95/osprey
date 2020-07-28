#ifndef WINDOW_WINDOW_H_
#define WINDOW_WINDOW_H_

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <vulkan/vulkan.h>

#include <exception>
#include <string>

namespace window { 

class Exception : public std::exception {
public:
  Exception(const std::string msg) : msg_(msg) {}
  const char* what() const final { return msg_.c_str(); }

private:
  std::string msg_;
};

// Created from the WindowManager class.
class Window {
public:
  // TODO(colintan): Consider making this private (see abseil lib's WrapUnique) -
  // https://abseil.io/tips/134
  Window(int width, int height, const std::string& title);
  ~Window();

  void Tick();

  void SwapBuffers();

  bool ShouldClose();

  VkSurfaceKHR CreateVkSurface(VkInstance vk_instance);

  int GetWidth() const { return width_; }
  int GetHeight() const { return height_; }
  const std::string& GetTitle() const { return title_; }

private:
  GLFWwindow* glfw_window_;

  int width_;
  int height_;
  std::string title_;
};

} // namespace window

#endif // WINDOW_WINDOW_H_