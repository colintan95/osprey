#ifndef GAL_GAL_PIPELINE_H_
#define GAL_GAL_PIPELINE_H_

#include <memory>
#include <vector>
#include "gal/gal_platform.h"
#include "gal/gal_shader.h"

namespace gal {

class GALPipeline {

// Forward declaration
class Builder;

public:
  GALPipeline(Builder& builder);
  ~GALPipeline();

public:
  struct Viewport {
    float x = 0.f;
    float y = 0.f;
    float width = 0.f;
    float height = 0.f;
  };

  struct VertexInput {
    int buffer_idx = 0;
    int stride = 0;
  };

  struct VertexDesc {
    int buffer_idx = 0;
    int shader_idx = 0;
    int num_components = 0;
    int offset = 0;
  };

  struct UniformDesc {
    int shader_idx = 0;
    ShaderType shader_stage = ShaderType::Invalid;
  };

  class Builder {
  friend class GALPipeline;

  public:
    Builder(GALPlatform* gal_platform) : gal_platform_(gal_platform) {}

    Builder& SetShader(ShaderType type, const GALShader& shader);
    Builder& SetViewport(const Viewport& viewport);
    Builder& AddVertexInput(const VertexInput& vert_input);
    Builder& AddVertexDesc(const VertexDesc& vert_desc);
    Builder& AddUniformDesc(const UniformDesc& uniform_desc);
    
    std::unique_ptr<GALPipeline> Create();

  private:
    GALPlatform* gal_platform_;

    GALShader vert_shader_;
    GALShader frag_shader_;

    Viewport viewport_;

    std::vector<VertexInput> vert_inputs_;
    std::vector<VertexDesc> vert_descs_;
    std::vector<UniformDesc> uniform_descs_;
  };
};

} // namespace gal

#endif // GAL_GAL_PIPELINE_H_