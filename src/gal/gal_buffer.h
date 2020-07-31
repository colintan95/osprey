#ifndef GAL_GAL_BUFFER_H_
#define GAL_GAL_BUFFER_H_

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

  class Builder {
  friend class GALBuffer;

  public:
    Builder(GALPlatform* gal_platform) : gal_platform_(gal_platform) {}

    Builder& SetType(BufferType type);
    Builder& SetBufferData(uint8_t* data, size_t size);

  private:
    GALPlatform* gal_platform_; 

    BufferType buffer_type_;
    uint8_t* data_;
    size_t data_size_;
  };
};

} // namespace gal

#endif // GAL_GAL_BUFFER_H_