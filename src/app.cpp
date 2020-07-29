#include "app.h"

#include <memory>
#include "window/window.h"
#include "window/window_manager.h"

App::App() {
  window_manager_ = std::make_unique<window::WindowManager>();
  window_ = window_manager_->CreateWindow(1920, 1080, "My Window");
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