#ifndef APP_H_
#define APP_H_

#include <memory>
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
};

#endif // APP_H_