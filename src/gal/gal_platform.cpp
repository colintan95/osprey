#include "gal/gal_platform.h"

#include <iostream>
#include <optional>
#include <vector>

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
}

GALPlatform::~GALPlatform() {
  
}

} // namespace gal