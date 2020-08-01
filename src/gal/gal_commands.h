#ifndef GAL_GAL_COMMANDS_H_
#define GAL_GAL_COMMANDS_H_

#include <cstdint>
#include <variant>
#include "gal/gal_buffer.h"
#include "gal/gal_pipeline.h"

namespace gal {

namespace command {

struct SetViewport {
  uint16_t x;
  uint16_t y;
  uint16_t width;
  uint16_t height;
};

struct SetPipeline {
  GALPipeline* pipeline;
};

struct SetVertexBuffer {
  GALBuffer* buffer;
  int buffer_idx;
};

struct DrawTriangles {
  uint32_t num_triangles;
};

} // namespace command

using CommandVariant = 
    std::variant<
        command::SetViewport,
        command::SetPipeline,
        command::SetVertexBuffer,
        command::DrawTriangles>;

} // namespace gal

#endif // GAL_GAL_COMMANDS_H_