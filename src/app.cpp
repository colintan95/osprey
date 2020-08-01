#include "app.h"

#include <glm/glm.hpp>

#include <iostream>
#include <fstream>
#include <memory>
#include <vector>
#include "gal/gal_command_buffer.h"
#include "gal/gal_commands.h"
#include "gal/gal_shader.h"
#include "gal/gal_pipeline.h"
#include "window/window.h"
#include "window/window_manager.h"

namespace {

struct Vertex {
  glm::vec2 pos;
  glm::vec3 color;
};

} // namespace

App::App() {
  window_manager_ = std::make_unique<window::WindowManager>();
  window_ = window_manager_->CreateWindow(1920, 1080, "My Window");

  gal_platform_ = std::make_unique<gal::GALPlatform>(window_);

  // TODO(colintan): Make this into a separate class
  std::ifstream vert_shader_file("shaders/triangle_vert.spv", std::ios::ate | std::ios::binary);
  if (!vert_shader_file.is_open()) {
    throw;
  }

  size_t vert_shader_file_size = static_cast<size_t>(vert_shader_file.tellg());
  vert_shader_file.seekg(0);

  std::vector<std::byte> vert_shader_binary(vert_shader_file_size);
  vert_shader_file.read(reinterpret_cast<char*>(vert_shader_binary.data()), vert_shader_file_size);

  std::ifstream frag_shader_file("shaders/triangle_frag.spv", std::ios::ate | std::ios::binary);
  if (!frag_shader_file.is_open()) {
    throw;
  }

  size_t frag_shader_file_size = static_cast<size_t>(frag_shader_file.tellg());
  frag_shader_file.seekg(0);

  std::vector<std::byte> frag_shader_binary(frag_shader_file_size);
  frag_shader_file.read(reinterpret_cast<char*>(frag_shader_binary.data()), frag_shader_file_size);

  gal::GALShader vert_shader;
  if (!vert_shader.CreateFromBinary(gal_platform_.get(), gal::ShaderType::Vertex, 
                                    vert_shader_binary)) {
    throw;
  }

  gal::GALShader frag_shader;
  if (!frag_shader.CreateFromBinary(gal_platform_.get(), gal::ShaderType::Fragment, 
                                    frag_shader_binary)) {
    throw;
  }

  gal::GALPipeline::Viewport viewport;
  viewport.x = 0;
  viewport.y = 0;
  viewport.width = window_->GetWidth();
  viewport.height = window_->GetHeight();

  gal::GALPipeline::VertexInput vert_input;
  vert_input.buffer_idx = 0;
  vert_input.stride = sizeof(Vertex);

  gal::GALPipeline::VertexDesc pos_desc;
  pos_desc.buffer_idx = 0;
  pos_desc.shader_idx = 0;
  pos_desc.num_components = 2;
  pos_desc.offset = 0;

  gal::GALPipeline::VertexDesc color_desc;
  color_desc.buffer_idx = 0;
  color_desc.shader_idx = 1;
  color_desc.num_components = 3;
  color_desc.offset = 2 * sizeof(float);

  gal::GALPipeline::UniformDesc uniform_desc;
  uniform_desc.shader_idx = 0;
  uniform_desc.shader_stage = gal::ShaderType::Vertex;
  
  try {
    gal_pipeline_ = gal::GALPipeline::BeginBuild(gal_platform_.get())
        .SetShader(gal::ShaderType::Vertex, vert_shader)
        .SetShader(gal::ShaderType::Fragment, frag_shader)
        .SetViewport(viewport)
        .AddVertexInput(vert_input)
        .AddVertexDesc(pos_desc)
        .AddVertexDesc(color_desc)
        .AddUniformDesc(uniform_desc)
        .Create();
  } catch (gal::Exception& e) {
    std::cerr << e.what() << std::endl;
    throw;
  }

  std::vector<Vertex> vertices = {
    {{0.f, -0.5f}, {1.f, 0.f, 0.f}},
    {{0.5f, 0.5f}, {0.f, 1.f, 0.f}},
    {{-0.5f, 0.5f,}, {0.f, 0.f, 1.f}}
  };

  try {
    vert_buffer_ = gal::GALBuffer::BeginBuild(gal_platform_.get())
        .SetType(gal::BufferType::Vertex)
        .SetBufferData(reinterpret_cast<uint8_t*>(vertices.data()), 
                       sizeof(Vertex) * vertices.size())
        .Create();
  } catch (gal::Exception& e) {
    std::cerr << e.what() << std::endl;
    throw;
  }

  try {
    command_buffer_ = std::make_unique<gal::GALCommandBuffer>(gal_platform_.get());
  } catch (gal::Exception& e) {
    std::cerr << e.what() << std::endl;
    throw;
  }

  if (!command_buffer_->BeginRecording()) {
    std::cerr << "Command buffer could not begin recording." << std::endl;
    throw;
  }

  gal::command::SetPipeline set_pipeline;
  set_pipeline.pipeline = gal_pipeline_.get();
  command_buffer_->SubmitCommand(set_pipeline);

  gal::command::SetVertexBuffer set_vert_buf;
  set_vert_buf.buffer = vert_buffer_.get();
  set_vert_buf.buffer_idx = 0;
  command_buffer_->SubmitCommand(set_vert_buf);

  gal::command::DrawTriangles draw_triangles;
  draw_triangles.num_triangles = 1;
  command_buffer_->SubmitCommand(draw_triangles);

  if (!command_buffer_->EndRecording()) {
    std::cerr << "Command buffer could not end recording." << std::endl;
    throw;
  }
}

App::~App() {
  
}

void App::MainLoop() {
  while (!window_->ShouldClose()) {
    Frame();
  }
}

void App::Frame() {
  window_->Tick();
  window_->SwapBuffers();
}