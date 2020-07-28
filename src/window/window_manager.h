#ifndef WINDOW_WINDOW_MANAGER_H_
#define WINDOW_WINDOW_MANAGER_H_

#include <exception>
#include <memory>
#include <string>
#include "window/window.h"

namespace window {

class WindowManager {
public:
  WindowManager();
  ~WindowManager();

  Window* CreateWindow(int width, int height, const std::string& title);

private:
  std::unique_ptr<Window> window_;
};

} // namespace window

#endif // WINDOW_WINDOW_MANAGER_H_