#include "gal/gal_pipeline.h"

#include <memory>
#include "gal/gal_exception.h"

namespace gal {

GALPipeline::GALPipeline(GALPipeline::Builder& builder) {

}

GALPipeline::Builder& GALPipeline::Builder::SetShader(ShaderType type, const GALShader& shader) {
  switch (type) {
  case ShaderType::Vertex:
    vert_shader_ = shader;
    break;
  case ShaderType::Fragment:
    frag_shader_ = shader;
    break;
  default:
    throw Exception("Shader type not supported.");
  }
  return *this;
}

GALPipeline::Builder& GALPipeline::Builder::SetViewport(const Viewport& viewport) {
  viewport_ = viewport;
  return *this;
}

GALPipeline::Builder& GALPipeline::Builder::AddVertexInput(const VertexInput& vert_input) {
  vert_inputs_.push_back(vert_input);
  return *this;
}

GALPipeline::Builder& GALPipeline::Builder::AddVertexDesc(const VertexDesc& vert_desc) {
  vert_descs_.push_back(vert_desc);
  return *this;
}

GALPipeline::Builder& GALPipeline::Builder::AddUniformDesc(const UniformDesc& uniform_desc) {
  uniform_descs_.push_back(uniform_desc);
  return *this;
}

std::unique_ptr<GALPipeline> GALPipeline::Builder::Create() {
  return std::make_unique<GALPipeline>(*this);
}

} // namespace gal