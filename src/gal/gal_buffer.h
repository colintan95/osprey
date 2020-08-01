#ifndef GAL_GAL_BUFFER_H_
#define GAL_GAL_BUFFER_H_

#include <vulkan/vulkan.h>

#include <memory>
#include <optional>
#include "gal/gal_platform.h"

namespace gal {

enum class BufferType {
  Vertex,
  Uniform
};

class GALBuffer {
// Forward declaration
class Builder;

public:
  GALBuffer(Builder& builder);
  ~GALBuffer();

  static Builder BeginBuild(GALPlatform* gal_platform) {
    return Builder(gal_platform);
  }

  VkBuffer GetVkBuffer() { return vk_buffer_; }

private:
  struct BufferInfo {
    VkBuffer vk_buffer;
    VkDeviceMemory vk_buffer_memory;
  };

  std::optional<BufferInfo> CreateBuffer(VkDeviceSize size, VkBufferUsageFlags usage, 
                                         VkMemoryPropertyFlags properties);

private:
  VkPhysicalDevice vk_physical_device_;
  VkDevice vk_device_;
  VkBuffer vk_buffer_;
  VkDeviceMemory vk_buffer_memory_;

public:
  class Builder {
  friend class GALBuffer;

  public:
    Builder(GALPlatform* gal_platform) : gal_platform_(gal_platform) {}

    Builder& SetType(BufferType type);
    Builder& SetBufferData(uint8_t* data, size_t size);

    std::unique_ptr<GALBuffer> Create();

  private:
    GALPlatform* gal_platform_; 

    BufferType buffer_type_;
    uint8_t* data_;
    size_t data_size_;
  };

};

} // namespace gal

#endif // GAL_GAL_BUFFER_H_