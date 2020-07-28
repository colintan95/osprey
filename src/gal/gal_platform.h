#ifndef GAL_GAL_PLATFORM_H_
#define GAL_GAL_PLATFORM_H_

#include <vulkan/vulkan.h>

#include <optional>
#include <vector>
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

  VkSwapchainKHR vk_swapchain_;
  VkFormat vk_swapchain_image_format_;
  VkExtent2D vk_swapchain_extent_;

  VkCommandPool vk_command_pool_;

  std::vector<VkImage> vk_swapchain_images_;
  std::vector<VkImageView> vk_swapchain_image_views_;

  std::vector<VkSemaphore> vk_image_available_semaphores_;
  std::vector<VkSemaphore> vk_render_finished_semaphores_;
  std::vector<VkFence> vk_in_flight_fences_;
  std::vector<VkFence> vk_images_in_flight_;
};

} // namespace gal

#endif GAL_GAL_PLATFORM_H_