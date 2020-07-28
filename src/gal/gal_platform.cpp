#include "gal/gal_platform.h"

#include <algorithm>
#include <iostream>
#include <optional>
#include <unordered_set>
#include <vector>

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
}

GALPlatform::~GALPlatform() {
  
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