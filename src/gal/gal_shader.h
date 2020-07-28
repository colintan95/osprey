#ifndef GAL_GAL_SHADER_H_
#define GAL_GAL_SHADER_H_

#include <vulkan/vulkan.h>

#include <cstddef>
#include <vector>
#include "gal/gal_platform.h"

namespace gal {

enum class ShaderType {
  Invalid,
  Vertex,
  Fragment
};

class GALShader {
public:
  bool CreateFromBinary(GALPlatform* gal_platform, ShaderType type, 
                        const std::vector<std::byte>& binary);

  VkShaderModule GetShaderModule() const { return vk_shader_; }

private:
  VkShaderModule vk_shader_;
  VkDevice vk_device_;
};

} // namespace gal

#endif // GAL_GAL_SHADER_H_