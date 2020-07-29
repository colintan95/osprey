#include "app.h"

#include <iostream>
#include <fstream>
#include <memory>
#include <vector>
#include "gal/gal_shader.h"
#include "window/window.h"
#include "window/window_manager.h"

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