#include "gal/gal_buffer.h"

namespace gal {

GALBuffer::Builder& GALBuffer::Builder::SetType(BufferType type) {
  buffer_type_ = type;
  return *this;
}

GALBuffer::Builder& GALBuffer::Builder::SetBufferData(uint8_t* data, size_t size) {
  data_ = data;
  data_size_ = size;
  return *this;
}

} // namespace