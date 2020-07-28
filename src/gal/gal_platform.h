#ifndef GAL_GAL_PLATFORM_H_
#define GAL_GAL_PLATFORM_H_

#include <vulkan/vulkan.h>

#include "gal/gal_exception.h"
#include "window/window.h"

namespace gal {

class GALPlatform {
public:
  GALPlatform(window::Window* window);
  ~GALPlatform();

private:
  window::Window* window_;

  VkInstance vk_instance_;
  VkDebugUtilsMessengerEXT vk_debug_messenger_;
  VkSurfaceKHR vk_surface_;
};

} // namespace gal

#endif GAL_GAL_PLATFORM_H_