#include "gal/gal_shader.h"

#include <cstdint>

namespace gal {

bool GALShader::CreateFromBinary(GALPlatform* gal_platform, ShaderType type, 
                                 const std::vector<std::byte>& shader_binary) {
  VkShaderModuleCreateInfo create_info{};
  create_info.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
  create_info.codeSize = shader_binary.size();
  create_info.pCode = reinterpret_cast<const uint32_t*>(shader_binary.data());

  vk_device_ = gal_platform->GetVkDevice();
  if (vkCreateShaderModule(vk_device_, &create_info, nullptr, &vk_shader_) != VK_SUCCESS) {
    return false;
  }
  return true;
}

} // namespace gal