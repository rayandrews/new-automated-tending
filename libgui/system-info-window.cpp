#include "gui.hpp"

#include "system-info-window.hpp"

#include <chrono>
#include <ctime>

#include "util.hpp"

NAMESPACE_BEGIN

namespace gui {
SystemInfoWindow::SystemInfoWindow(float                   width,
                                   float                   height,
                                   const ImGuiWindowFlags& flags)
    : Window{"System Info", width, height, flags} {}

SystemInfoWindow::~SystemInfoWindow() {}

void SystemInfoWindow::show([[maybe_unused]] Manager* manager) {
  std::time_t end_time =
      std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
  auto time_str = std::ctime(&end_time);

  ImGui::Text("%s", time_str);

  ImGui::Text("%.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate,
              ImGui::GetIO().Framerate);
}
}  // namespace gui

NAMESPACE_END
