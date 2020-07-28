#ifndef GAL_GAL_PLATFORM_H_
#define GAL_GAL_PLATFORM_H_

#include <vulkan/vulkan.h>

#include <optional>
#include "gal/gal_exception.h"
#include "window/window.h"

namespace gal {

class GALPlatform {
public:
  GALPlatform(window::Window* window);
  ~GALPlatform();

private:
  struct PhysicalDeviceInfo {
    uint32_t graphics_queue_family_index;
    uint32_t present_queue_family_index;
  };

  std::optional<PhysicalDeviceInfo> ChoosePhysicalDevice();

  VkSurfaceFormatKHR ChooseSurfaceFormat();
  VkPresentModeKHR ChoosePresentMode();
  VkExtent2D ChooseSwapExtent();

private:
  window::Window* window_;

  VkInstance vk_instance_;
  VkDebugUtilsMessengerEXT vk_debug_messenger_;
  VkSurfaceKHR vk_surface_;

  VkPhysicalDevice vk_physical_device_;
  VkDevice vk_device_;
  VkQueue vk_graphics_queue_;
  VkQueue vk_present_queue_;
};

} // namespace gal

#endif GAL_GAL_PLATFORM_H_