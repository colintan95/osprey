#include "gal/gal_command_buffer.h"

#include <vulkan/vulkan.h>

#include <iostream>
#include "gal/gal_commands.h"
#include "gal/gal_exception.h"
#include "gal/gal_platform.h"

namespace gal {

GALCommandBuffer::GALCommandBuffer(GALPlatform* gal_platform) {
  gal_platform_ = gal_platform;
  vk_device_ = gal_platform->GetVkDevice();

  uint32_t framebuffer_count = gal_platform->GetSwapchainImageViews().size();

  vk_command_buffers_.resize(framebuffer_count);

  VkCommandBufferAllocateInfo command_buffer_alloc_info{};
  command_buffer_alloc_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
  command_buffer_alloc_info.commandPool = gal_platform->GetVkCommandPool();
  command_buffer_alloc_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
  command_buffer_alloc_info.commandBufferCount = static_cast<uint32_t>(vk_command_buffers_.size());

  if (vkAllocateCommandBuffers(vk_device_, &command_buffer_alloc_info, 
                               vk_command_buffers_.data()) != VK_SUCCESS) {
    throw Exception("Could not create command buffers.");
  }
}

GALCommandBuffer::~GALCommandBuffer() {}

bool GALCommandBuffer::BeginRecording() {
  for (VkCommandBuffer command_buffer : vk_command_buffers_) {
    VkCommandBufferBeginInfo begin_info{};
    begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

    if (vkBeginCommandBuffer(command_buffer, &begin_info) != VK_SUCCESS) {
      std::cerr << "Could not begin command buffer." << std::endl;
      return false;
    }
  }
  return true;
}

bool GALCommandBuffer::EndRecording() {
  for (VkCommandBuffer command_buffer : vk_command_buffers_) {
    vkCmdEndRenderPass(command_buffer);

    if (vkEndCommandBuffer(command_buffer) != VK_SUCCESS) {
      std::cerr << "Could not end command buffer." << std::endl;
      return false;
    }
  }
  return true;
}

void GALCommandBuffer::SubmitCommand(const CommandVariant& command_variant) {
  if (std::holds_alternative<command::SetPipeline>(command_variant)) {
    const command::SetPipeline& command = std::get<command::SetPipeline>(command_variant);

    for (size_t i = 0; i < vk_command_buffers_.size(); ++i) {
      VkClearValue clear_color = {0.f, 0.f, 0.f, 1.f};

      VkRenderPassBeginInfo render_pass_begin_info{};
      render_pass_begin_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
      render_pass_begin_info.renderPass = command.pipeline->GetVkRenderPass();
      render_pass_begin_info.framebuffer = command.pipeline->GetVkFramebuffers()[i];
      render_pass_begin_info.renderArea.offset = {0, 0};
      render_pass_begin_info.renderArea.extent = gal_platform_->GetVkSwapchainExtent();
      render_pass_begin_info.clearValueCount = 1;
      render_pass_begin_info.pClearValues = &clear_color;

      vkCmdBeginRenderPass(vk_command_buffers_[i], &render_pass_begin_info, 
                           VK_SUBPASS_CONTENTS_INLINE);

      vkCmdBindPipeline(vk_command_buffers_[i], VK_PIPELINE_BIND_POINT_GRAPHICS, 
                        command.pipeline->GetVkPipeline());
    }
  } else if (std::holds_alternative<command::SetVertexBuffer>(command_variant)) {
    const command::SetVertexBuffer& command = std::get<command::SetVertexBuffer>(command_variant);

    for (VkCommandBuffer command_buffer : vk_command_buffers_) {
      VkBuffer buffers[] = { command.buffer->GetVkBuffer() };
      VkDeviceSize offsets[] = {0};
      vkCmdBindVertexBuffers(command_buffer, command.buffer_idx, 1, buffers, offsets);
    }

  } else if (std::holds_alternative<command::DrawTriangles>(command_variant)) {
    const command::DrawTriangles& command = std::get<command::DrawTriangles>(command_variant);

    for (VkCommandBuffer command_buffer : vk_command_buffers_) {
      vkCmdDraw(command_buffer, 3, command.num_triangles, 0, 0);
    }
  }
}

} // namespace gal