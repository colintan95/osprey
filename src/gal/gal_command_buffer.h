#ifndef GAL_GAL_COMMAND_BUFFER_H_
#define GAL_GAL_COMMAND_BUFFER_H_

#include <vulkan/vulkan.h>

#include <vector>
#include "gal/gal_commands.h"
#include "gal/gal_platform.h"

namespace gal {

class GALCommandBuffer {
  // Forward declaration
class Builder;

public:
  GALCommandBuffer(GALPlatform* gal_platform);
  ~GALCommandBuffer();

  bool BeginRecording();
  bool EndRecording();

  void SubmitCommand(const CommandVariant& command_variant);

private:
  GALPlatform* gal_platform_;

  VkDevice vk_device_;
  std::vector<VkCommandBuffer> vk_command_buffers_;
};

} // namespace gal

#endif // GAL_GAL_COMMAND_BUFFER_H_