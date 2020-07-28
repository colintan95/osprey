#include "gal/gal_platform.h"

#include <iostream>
#include <optional>
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

} // namespace gal