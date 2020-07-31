#include "gal/gal_buffer.h"

#include <iostream>
#include <memory>
#include <optional>

namespace gal {

GALBuffer::GALBuffer(GALBuffer::Builder& builder) {
  vk_physical_device_ = builder.gal_platform_->GetVkPhysicalDevice();
  vk_device_ = builder.gal_platform_->GetVkDevice();
  
  std::optional<BufferInfo> staging_buf_info_opt = 
      CreateBuffer(builder.data_size_, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                   VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
  if (!staging_buf_info_opt.has_value()) {
    throw Exception("Could not create staging buffer.");
  }

  VkBuffer staging_buf = staging_buf_info_opt.value().vk_buffer;
  VkDeviceMemory staging_buf_mem = staging_buf_info_opt.value().vk_buffer_memory;

  void* device_data;
  vkMapMemory(vk_device_, staging_buf_mem, 0, builder.data_size_, 0, &device_data);
  memcpy(device_data, builder.data_, builder.data_size_);
  vkUnmapMemory(vk_device_, staging_buf_mem);

  std::optional<BufferInfo> vert_buf_info_opt;

  if (builder.buffer_type_ == BufferType::Vertex) {
      vert_buf_info_opt = CreateBuffer(
          builder.data_size_, 
          VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
          VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
  } else {
    throw Exception("Buffer type not supported.");
  }

  if (!vert_buf_info_opt.has_value()) {
    throw Exception("Could not create VkBuffer.");
  }

  vk_buffer_ = vert_buf_info_opt.value().vk_buffer;
  vk_buffer_memory_ = vert_buf_info_opt.value().vk_buffer_memory;

  // Copies data from the staging buffer to the vertex buffer

  VkCommandPool cmd_pool = builder.gal_platform_->GetVkCommandPool();

  VkCommandBufferAllocateInfo cmd_buf_alloc_info{};
  cmd_buf_alloc_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
  cmd_buf_alloc_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
  cmd_buf_alloc_info.commandPool = cmd_pool;
  cmd_buf_alloc_info.commandBufferCount = 1;

  VkCommandBuffer cmd_buf;
  vkAllocateCommandBuffers(vk_device_, &cmd_buf_alloc_info, &cmd_buf);

  VkCommandBufferBeginInfo begin_info{};
  begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
  begin_info.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
  vkBeginCommandBuffer(cmd_buf, &begin_info);

  VkBufferCopy buf_copy{};
  buf_copy.size = builder.data_size_;
  vkCmdCopyBuffer(cmd_buf, staging_buf, vk_buffer_, 1, &buf_copy);

  vkEndCommandBuffer(cmd_buf);

  VkSubmitInfo submit_info{};
  submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
  submit_info.commandBufferCount = 1;
  submit_info.pCommandBuffers = &cmd_buf;

  VkQueue graphics_queue = builder.gal_platform_->GetVkGraphicsQueue();
  vkQueueSubmit(graphics_queue, 1, &submit_info, VK_NULL_HANDLE);
  vkQueueWaitIdle(graphics_queue);

  vkFreeCommandBuffers(vk_device_, cmd_pool, 1, &cmd_buf);

  vkFreeMemory(vk_device_, staging_buf_mem, nullptr);
  vkDestroyBuffer(vk_device_, staging_buf, nullptr);
}

GALBuffer::~GALBuffer() {
  vkFreeMemory(vk_device_, vk_buffer_memory_, nullptr);
  vkDestroyBuffer(vk_device_, vk_buffer_, nullptr);
}

std::optional<GALBuffer::BufferInfo> 
    GALBuffer::CreateBuffer(VkDeviceSize size, VkBufferUsageFlags usage, 
                            VkMemoryPropertyFlags properties) {
  VkBufferCreateInfo buffer_create_info{};
  buffer_create_info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
  buffer_create_info.size = size;
  buffer_create_info.usage = usage;
  buffer_create_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

  BufferInfo buffer_info{};

  if (vkCreateBuffer(vk_device_, &buffer_create_info, nullptr, &buffer_info.vk_buffer) 
          != VK_SUCCESS) {
    std::cerr << "Could not create VkBuffer." << std::endl;
    return std::nullopt;
  }

  VkMemoryRequirements memory_req;
  vkGetBufferMemoryRequirements(vk_device_, buffer_info.vk_buffer, &memory_req);

  VkPhysicalDeviceMemoryProperties memory_props;
  vkGetPhysicalDeviceMemoryProperties(vk_physical_device_, &memory_props);

  bool found_memory_type = false;
  uint32_t memory_type_index = 0;

  for (uint32_t i = 0; i < memory_props.memoryTypeCount; ++i) {
    if (memory_req.memoryTypeBits & (1 << i) && 
        memory_props.memoryTypes[i].propertyFlags == properties) {
      found_memory_type = true;
      memory_type_index = i;
    }
  }

  if (!found_memory_type) {
    std::cerr << "Could not find memory for memory type." << std::endl;
    return std::nullopt;
  }

  VkMemoryAllocateInfo alloc_info{};
  alloc_info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
  alloc_info.allocationSize = memory_req.size;
  alloc_info.memoryTypeIndex = memory_type_index;

  if (vkAllocateMemory(vk_device_, &alloc_info, nullptr, &buffer_info.vk_buffer_memory) 
          != VK_SUCCESS) {
    std::cerr << "Could not allocate VkDeviceMemory." << std::endl;
    return std::nullopt;
  }

  vkBindBufferMemory(vk_device_, buffer_info.vk_buffer, buffer_info.vk_buffer_memory, 0);

  return buffer_info;
}

GALBuffer::Builder& GALBuffer::Builder::SetType(BufferType type) {
  buffer_type_ = type;
  return *this;
}

GALBuffer::Builder& GALBuffer::Builder::SetBufferData(uint8_t* data, size_t size) {
  data_ = data;
  data_size_ = size;
  return *this;
}

std::unique_ptr<GALBuffer> GALBuffer::Builder::Create() {
  return std::make_unique<GALBuffer>(*this);
}

} // namespace