#include "gal/gal_platform.h"

#include <algorithm>
#include <iostream>
#include <optional>
#include <unordered_set>
#include <vector>

#include "gal/gal_command_buffer.h"
#include "gal/gal_exception.h"
#include "window/window.h"

namespace gal {

namespace {

VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(
    VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity, 
    VkDebugUtilsMessageTypeFlagsEXT messageType, 
    const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData, 
    void* pUserData) {
  std::cerr << "Vulkan: " << pCallbackData->pMessage << std::endl;

  return VK_FALSE;
}

const int kMaxFramesInFlight = 2;

} // namespace

GALPlatform::GALPlatform(window::Window* window) {
  if (window == nullptr) {
    throw Exception("window parameter cannot be nullptr.");
  }
  window_ = window;

  VkApplicationInfo app_info = {};
  app_info.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
  app_info.apiVersion = VK_MAKE_VERSION(1, 0, 0);

  VkDebugUtilsMessengerCreateInfoEXT debug_create_info= {};
  debug_create_info.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
  debug_create_info.messageSeverity = 
      VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT | 
      VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | 
      VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
  debug_create_info.messageType =
      VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
      VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
      VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
  debug_create_info.pfnUserCallback = debugCallback;

  uint32_t glfw_extension_count = 0;
  const char** glfw_extensions = glfwGetRequiredInstanceExtensions(&glfw_extension_count);

  std::vector<const char*> extensions(glfw_extensions, glfw_extensions + glfw_extension_count);
  extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);

  std::vector<const char*> validation_layers;
  validation_layers.push_back("VK_LAYER_KHRONOS_validation");

  VkInstanceCreateInfo instance_create_info = {};
  instance_create_info.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
  instance_create_info.pNext = &debug_create_info;
  instance_create_info.pApplicationInfo = &app_info;
  instance_create_info.enabledLayerCount = validation_layers.size();
  instance_create_info.ppEnabledLayerNames = validation_layers.data();
  instance_create_info.enabledExtensionCount = extensions.size();
  instance_create_info.ppEnabledExtensionNames = extensions.data();

  if (vkCreateInstance(&instance_create_info, nullptr, &vk_instance_) != VK_SUCCESS) {
    throw Exception("Could not create VkInstance.");
  }

  auto create_debug_utils_messenger_func = 
      (PFN_vkCreateDebugUtilsMessengerEXT) vkGetInstanceProcAddr(
            vk_instance_, "vkCreateDebugUtilsMessengerEXT");
  if (create_debug_utils_messenger_func == nullptr) {
    throw Exception("Could not find create debug utils messenger func.");
  }
  if (create_debug_utils_messenger_func(
        vk_instance_, &debug_create_info, nullptr, &vk_debug_messenger_) != VK_SUCCESS) {
    throw Exception("Could not create debug utils messenger.");
  }

  vk_surface_ = window->CreateVkSurface(vk_instance_);

  std::optional<PhysicalDeviceInfo> physical_device_info = ChoosePhysicalDevice();
  if (!physical_device_info.has_value()) {
    throw Exception("Could not find a suitable physical device.");
  }

  uint32_t graphics_queue_family_index = physical_device_info.value().graphics_queue_family_index;
  uint32_t present_queue_family_index = physical_device_info.value().present_queue_family_index;

  std::vector<VkDeviceQueueCreateInfo> queue_create_infos;
  std::unordered_set<uint32_t> queue_family_indices_set = { graphics_queue_family_index, 
                                                            present_queue_family_index };
  for (uint32_t queue_family_index : queue_family_indices_set) {
    float queue_properties[] = { 1.f };

    VkDeviceQueueCreateInfo queue_create_info{};
    queue_create_info.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
    queue_create_info.queueFamilyIndex = queue_family_index;
    queue_create_info.queueCount = 1;
    queue_create_info.pQueuePriorities = queue_properties;

    queue_create_infos.push_back(std::move(queue_create_info));
  }

  VkPhysicalDeviceFeatures device_enabled_features{};
  std::vector<const char*> device_extensions = { VK_KHR_SWAPCHAIN_EXTENSION_NAME };

  VkDeviceCreateInfo device_create_info{};
  device_create_info.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
  device_create_info.queueCreateInfoCount = queue_create_infos.size();
  device_create_info.pQueueCreateInfos = queue_create_infos.data();
  device_create_info.enabledLayerCount = static_cast<uint32_t>(validation_layers.size());
  device_create_info.ppEnabledLayerNames = validation_layers.data();
  device_create_info.enabledExtensionCount = device_extensions.size();
  device_create_info.ppEnabledExtensionNames = device_extensions.data();
  device_create_info.pEnabledFeatures = &device_enabled_features;

  if (vkCreateDevice(vk_physical_device_, &device_create_info, nullptr, 
                     &vk_device_) != VK_SUCCESS) {
    throw Exception("Could not create VkDevice.");
  }

  vkGetDeviceQueue(vk_device_, graphics_queue_family_index, 0, &vk_graphics_queue_);
  vkGetDeviceQueue(vk_device_, present_queue_family_index, 0, &vk_present_queue_);

  VkSurfaceFormatKHR surface_format = ChooseSurfaceFormat();
  VkPresentModeKHR present_mode = ChoosePresentMode();
  VkExtent2D extent = ChooseSwapExtent();

  VkSurfaceCapabilitiesKHR surface_capabilities;
  vkGetPhysicalDeviceSurfaceCapabilitiesKHR(vk_physical_device_, vk_surface_, 
                                            &surface_capabilities);

  uint32_t requested_image_count = surface_capabilities.minImageCount + 1;
  if (surface_capabilities.maxImageCount != 0) {
    requested_image_count = std::min(requested_image_count, surface_capabilities.maxImageCount);
  }

  VkSwapchainCreateInfoKHR swapchain_create_info{};
  swapchain_create_info.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
  swapchain_create_info.surface = vk_surface_;
  swapchain_create_info.minImageCount = requested_image_count;
  swapchain_create_info.imageFormat = surface_format.format;
  swapchain_create_info.imageColorSpace = surface_format.colorSpace;
  swapchain_create_info.imageExtent = extent;
  swapchain_create_info.imageArrayLayers = 1;
  swapchain_create_info.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

  if (graphics_queue_family_index != present_queue_family_index) {
    uint32_t queue_family_indices[] = { graphics_queue_family_index, present_queue_family_index };
    swapchain_create_info.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
    swapchain_create_info.queueFamilyIndexCount = 2;
    swapchain_create_info.pQueueFamilyIndices = queue_family_indices;
  } else {
    swapchain_create_info.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
    swapchain_create_info.queueFamilyIndexCount = 0;
    swapchain_create_info.pQueueFamilyIndices = nullptr;
  }

  swapchain_create_info.preTransform = surface_capabilities.currentTransform;
  swapchain_create_info.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
  swapchain_create_info.presentMode = present_mode;
  swapchain_create_info.clipped = VK_TRUE;
  swapchain_create_info.oldSwapchain = VK_NULL_HANDLE;

  if (vkCreateSwapchainKHR(vk_device_, &swapchain_create_info, nullptr, &vk_swapchain_) 
          != VK_SUCCESS) {
    throw Exception("Could not create VkSwapchain.");
  }

  uint32_t image_count = 0;
  vkGetSwapchainImagesKHR(vk_device_, vk_swapchain_, &image_count, nullptr);

  vk_swapchain_images_.resize(image_count);
  vkGetSwapchainImagesKHR(vk_device_, vk_swapchain_, &image_count, vk_swapchain_images_.data());

  vk_swapchain_image_format_ = surface_format.format;
  vk_swapchain_extent_ = extent;

  vk_swapchain_image_views_.resize(image_count);

  for (size_t i = 0; i < vk_swapchain_images_.size(); ++i) {
    VkImageViewCreateInfo image_view_create_info{};
    image_view_create_info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    image_view_create_info.image = vk_swapchain_images_[i];
    image_view_create_info.viewType = VK_IMAGE_VIEW_TYPE_2D;
    image_view_create_info.format = vk_swapchain_image_format_;
    image_view_create_info.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
    image_view_create_info.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
    image_view_create_info.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
    image_view_create_info.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
    image_view_create_info.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    image_view_create_info.subresourceRange.baseMipLevel = 0;
    image_view_create_info.subresourceRange.levelCount = 1;
    image_view_create_info.subresourceRange.baseArrayLayer = 0;
    image_view_create_info.subresourceRange.layerCount = 1;

    if (vkCreateImageView(vk_device_, &image_view_create_info, nullptr, 
            &vk_swapchain_image_views_[i]) != VK_SUCCESS) {
      throw Exception("Could not create image view for swapchain image.");
    }
  }

  VkCommandPoolCreateInfo command_pool_create_info{};
  command_pool_create_info.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
  command_pool_create_info.queueFamilyIndex = graphics_queue_family_index;

  if (vkCreateCommandPool(vk_device_, &command_pool_create_info, nullptr,
                          &vk_command_pool_) != VK_SUCCESS) {
    throw Exception("Could not create command pool.");
  }

  vk_image_available_semaphores_.resize(kMaxFramesInFlight);
  vk_render_finished_semaphores_.resize(kMaxFramesInFlight);
  vk_in_flight_fences_.resize(kMaxFramesInFlight);
  vk_images_in_flight_.resize(vk_swapchain_images_.size(), VK_NULL_HANDLE);

  VkSemaphoreCreateInfo semaphore_create_info{};
  semaphore_create_info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

  VkFenceCreateInfo fence_create_info{};
  fence_create_info.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
  fence_create_info.flags = VK_FENCE_CREATE_SIGNALED_BIT;

  for (size_t i = 0; i < kMaxFramesInFlight; ++i) {
    if (vkCreateSemaphore(vk_device_, &semaphore_create_info, nullptr,
                          &vk_image_available_semaphores_[i]) != VK_SUCCESS) {
      throw Exception("Could not create semaphore.");
    }

    if (vkCreateSemaphore(vk_device_, &semaphore_create_info, nullptr,
                          &vk_render_finished_semaphores_[i]) != VK_SUCCESS) {
      throw Exception("Could not create semaphore.");
    }

    if (vkCreateFence(vk_device_, &fence_create_info, nullptr, &vk_in_flight_fences_[i]) 
            != VK_SUCCESS) {
      throw Exception("Could not create fence." );
    }
  }
}

GALPlatform::~GALPlatform() {
  // TODO(colintan): Should this be here?
  vkDeviceWaitIdle(vk_device_);

  for (size_t i = 0; i < kMaxFramesInFlight; ++i) {
    vkDestroyFence(vk_device_, vk_in_flight_fences_[i], nullptr);
    vkDestroySemaphore(vk_device_, vk_render_finished_semaphores_[i], nullptr);
    vkDestroySemaphore(vk_device_, vk_image_available_semaphores_[i], nullptr);
  }

  vkDestroyCommandPool(vk_device_, vk_command_pool_, nullptr);

  for (VkImageView image_view : vk_swapchain_image_views_) {
    vkDestroyImageView(vk_device_, image_view, nullptr);
  }

  vkDestroySwapchainKHR(vk_device_, vk_swapchain_, nullptr);

  vkDestroyDevice(vk_device_, nullptr);

  vkDestroySurfaceKHR(vk_instance_, vk_surface_, nullptr);

  auto destroy_debug_utils_messenger_func = 
      (PFN_vkDestroyDebugUtilsMessengerEXT) vkGetInstanceProcAddr(vk_instance_, 
          "vkDestroyDebugUtilsMessengerEXT");
  if (destroy_debug_utils_messenger_func != nullptr) {
    destroy_debug_utils_messenger_func(vk_instance_, vk_debug_messenger_, nullptr);
  }

  vkDestroyInstance(vk_instance_, nullptr);
}

void GALPlatform::StartTick() {
  vkWaitForFences(vk_device_, 1, &vk_in_flight_fences_[current_frame_], VK_TRUE, UINT64_MAX);

  vkAcquireNextImageKHR(vk_device_, vk_swapchain_, UINT64_MAX, 
                        vk_image_available_semaphores_[current_frame_], VK_NULL_HANDLE, 
                        &current_image_index_);

  if (vk_images_in_flight_[current_image_index_] != VK_NULL_HANDLE) {
    vkWaitForFences(vk_device_, 1, &vk_images_in_flight_[current_image_index_], VK_TRUE, 
                    UINT64_MAX);
  }

  vk_images_in_flight_[current_image_index_] = vk_in_flight_fences_[current_frame_];
}

void GALPlatform::EndTick() {
  current_frame_ = (current_frame_ + 1) % kMaxFramesInFlight;
}

bool GALPlatform::ExecuteCommandBuffer(GALCommandBuffer* command_buffer) {
  VkSemaphore wait_semaphores[] = { vk_image_available_semaphores_[current_frame_] };
  VkSemaphore signal_semaphores[] = { vk_render_finished_semaphores_[current_frame_] };
  VkPipelineStageFlags wait_stages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };

  VkSubmitInfo submit_info{};
  submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
  submit_info.waitSemaphoreCount = 1;
  submit_info.pWaitSemaphores = wait_semaphores;
  submit_info.pWaitDstStageMask = wait_stages;
  submit_info.commandBufferCount = 1;
  submit_info.pCommandBuffers = 
      &(command_buffer->GetVkCommandBuffers()[current_image_index_]);
  submit_info.signalSemaphoreCount = 1;
  submit_info.pSignalSemaphores = signal_semaphores;

  vkResetFences(vk_device_, 1, &vk_in_flight_fences_[current_frame_]);

  if (vkQueueSubmit(vk_graphics_queue_, 1, &submit_info, vk_in_flight_fences_[current_frame_]) 
          != VK_SUCCESS) {
    return false;
  }

  VkSwapchainKHR swapchains[] = { vk_swapchain_ };

  VkPresentInfoKHR present_info{};
  present_info.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
  present_info.waitSemaphoreCount = 1;
  present_info.pWaitSemaphores = signal_semaphores;
  present_info.swapchainCount = 1;
  present_info.pSwapchains = swapchains;
  present_info.pImageIndices = &current_image_index_;

  vkQueuePresentKHR(vk_present_queue_, &present_info);

  return true;
}

std::optional<GALPlatform::PhysicalDeviceInfo> GALPlatform::ChoosePhysicalDevice() {
  uint32_t physical_devices_count = 0;
  vkEnumeratePhysicalDevices(vk_instance_, &physical_devices_count, nullptr);

  std::vector<VkPhysicalDevice> physical_devices(physical_devices_count);
  vkEnumeratePhysicalDevices(vk_instance_, &physical_devices_count, physical_devices.data());

  bool found_physical_device = false;
  PhysicalDeviceInfo result;

  for (const VkPhysicalDevice& device : physical_devices) {

    // Discrete GPU

    VkPhysicalDeviceProperties device_props;
    vkGetPhysicalDeviceProperties(device, &device_props);

    if (device_props.deviceType != VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU) {
      continue;
    }

    uint32_t queue_families_count = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(device, &queue_families_count, nullptr);

    std::vector<VkQueueFamilyProperties> queue_families(queue_families_count);
    vkGetPhysicalDeviceQueueFamilyProperties(device, &queue_families_count, queue_families.data());

    // Graphics queue

    bool found_graphics_queue = false;

    int index = 0;
    for (const VkQueueFamilyProperties& queue_family : queue_families) {
      if (queue_family.queueFlags & VK_QUEUE_GRAPHICS_BIT) {
        result.graphics_queue_family_index = index;
        found_graphics_queue = true;
        break;
      }
      ++index;
    }
    if (!found_graphics_queue) {
      continue;
    }

    // Present queue

    bool found_present_queue = false;

    index = 0;
    for (const VkQueueFamilyProperties& queue_family : queue_families) {
      VkBool32 supports_present = false;
      vkGetPhysicalDeviceSurfaceSupportKHR(device, index, vk_surface_, 
                                            &supports_present);
      if (supports_present) {
        result.present_queue_family_index = index;
        found_present_queue = true;
        break;
      }
      ++index;
    }
    if (!found_present_queue) {
      continue;
    }

    // Extensions

    uint32_t extensions_count = 0;
    vkEnumerateDeviceExtensionProperties(device, nullptr, &extensions_count, nullptr);

    std::vector<VkExtensionProperties> extensions(extensions_count);
    vkEnumerateDeviceExtensionProperties(device, nullptr, &extensions_count, extensions.data());

    // TODO(colintan): Enumerate through a list of requested extensions and see if the physical
    // device supports each.
    if (std::find_if(extensions.begin(), extensions.end(), 
            [](const VkExtensionProperties& props) { 
              return strncmp(props.extensionName, VK_KHR_SWAPCHAIN_EXTENSION_NAME,
                             sizeof(VK_KHR_SWAPCHAIN_EXTENSION_NAME)) == 0;
            }) == extensions.end()) {
      continue;
    }

    // Surface formats and present modes - whether they are nonempty

    uint32_t surface_formats_count = 0;
    vkGetPhysicalDeviceSurfaceFormatsKHR(device, vk_surface_, &surface_formats_count, nullptr);
    if (surface_formats_count == 0) {
      continue;
    }

    uint32_t present_modes_count = 0;
    vkGetPhysicalDeviceSurfacePresentModesKHR(device, vk_surface_, &present_modes_count, nullptr);
    if (present_modes_count == 0) {
      continue;
    }

    found_physical_device = true;
    vk_physical_device_ = device;
  }
  
  if (!found_physical_device) {
    return std::nullopt;
  }

  return result;
}

VkSurfaceFormatKHR GALPlatform::ChooseSurfaceFormat() {
  uint32_t surface_formats_count = 0;
  vkGetPhysicalDeviceSurfaceFormatsKHR(vk_physical_device_, vk_surface_, &surface_formats_count, 
                                       nullptr);
  
  std::vector<VkSurfaceFormatKHR> surface_formats(surface_formats_count);
  vkGetPhysicalDeviceSurfaceFormatsKHR(vk_physical_device_, vk_surface_, &surface_formats_count,
                                       surface_formats.data());

  for (const VkSurfaceFormatKHR& surface_format : surface_formats) {
    if (surface_format.format == VK_FORMAT_B8G8R8A8_SRGB && 
        surface_format.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
      return surface_format;
    }
  }

  return surface_formats[0];
}

VkPresentModeKHR GALPlatform::ChoosePresentMode() {
  uint32_t present_modes_count = 0;
  vkGetPhysicalDeviceSurfacePresentModesKHR(vk_physical_device_, vk_surface_, &present_modes_count, 
                                            nullptr);

  std::vector<VkPresentModeKHR> present_modes(present_modes_count);
  vkGetPhysicalDeviceSurfacePresentModesKHR(vk_physical_device_, vk_surface_, &present_modes_count,
                                            present_modes.data());

  for (const VkPresentModeKHR& present_mode : present_modes) {
    if (present_mode == VK_PRESENT_MODE_MAILBOX_KHR) {
      return present_mode;
    }
  }
    
  return VK_PRESENT_MODE_FIFO_KHR;
}

VkExtent2D GALPlatform::ChooseSwapExtent() {
  VkSurfaceCapabilitiesKHR surface_capabilities;
  vkGetPhysicalDeviceSurfaceCapabilitiesKHR(vk_physical_device_, vk_surface_, 
                                            &surface_capabilities);

  if (surface_capabilities.currentExtent.width == UINT32_MAX) {
    VkExtent2D extent{};

    uint32_t min_width = surface_capabilities.minImageExtent.width;
    uint32_t max_width = surface_capabilities.maxImageExtent.width;
    uint32_t window_width = window_->GetWidth();

    uint32_t min_height = surface_capabilities.minImageExtent.height;
    uint32_t max_height = surface_capabilities.maxImageExtent.height;
    uint32_t window_height = window_->GetHeight();

    extent.width = std::max(min_width, std::min(window_height, max_height));
    extent.height = std::max(min_height, std::min(window_height, max_height));

    return extent;
  } else {
    return surface_capabilities.currentExtent;
  }
}

} // namespace gal