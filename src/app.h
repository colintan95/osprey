#ifndef APP_H_
#define APP_H_

#include <memory>
#include "gal/gal_pipeline.h"
#include "gal/gal_platform.h"
#include "window/window.h"
#include "window/window_manager.h"

class App {
public:
  App();
  ~App();

  void MainLoop();

  void Frame();

private:
  std::unique_ptr<window::WindowManager> window_manager_;
  window::Window* window_;
  
  std::unique_ptr<gal::GALPlatform> gal_platform_;
  std::unique_ptr<gal::GALPipeline> gal_pipeline_;
};

#endif // APP_H_