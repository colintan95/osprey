#include "gal/gal_buffer.h"

#include <memory>

namespace gal {

GALBuffer::GALBuffer(GALBuffer::Builder& builder) {

}

GALBuffer::~GALBuffer() {
  
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