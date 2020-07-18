#include <iostream>
#include <stdexcept>

#include <libcore/core.hpp>
#include <libdevice/device.hpp>
#include <libgui/gui.hpp>
#include <libmachine/machine.hpp>

int main() {
  USE_NAMESPACE;

  ATM_STATUS       status = ATM_OK;
  machine::tending tsm;
  gui::Manager     ui_manager;

  // initialization
  try {
    tsm.start();
    massert(tsm.is_running(), "sanity");
  } catch (std::runtime_error& e) {
    std::cerr << e.what() << std::endl;
    status = ATM_ERR;
  }

  // early stopping
  if (status == ATM_ERR) {
    tsm.stop();
    massert(tsm.is_terminated(), "sanity");
    return status;
  }

  ui_manager.init("Emmerich Tending Machine", 400, 400);

  // early stopping
  if (!ui_manager.active()) {
    return ATM_ERR;
  }

  ui_manager.key_callback([](gui::Manager::MainWindow* current_window, int key,
                             [[maybe_unused]] int scancode, int action,
                             int mods) {
    if (mods == GLFW_MOD_ALT && key == GLFW_KEY_F4 && action == GLFW_PRESS) {
      glfwSetWindowShouldClose(current_window, GL_TRUE);
    } else if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS) {
      glfwSetWindowShouldClose(current_window, GL_TRUE);
    }
  });

  ui_manager.error_callback([](int error, const char* description) {
    LOG_ERROR("Glfw Error {}: {}", error, description);
  });

  ui_manager.add_window<gui::FaultWindow>(&tsm);
  ui_manager.add_window<gui::MovementWindow>();
  ui_manager.add_window<gui::ManualMovementWindow>(&tsm);
  ui_manager.add_window<gui::StatusWindow>();
  ui_manager.add_window<gui::CleaningWindow>();
  ui_manager.add_window<gui::CleaningControlWindow>();

  while (ui_manager.handle_events()) {
    ui_manager.render();
  }

  // stopping
  ui_manager.exit();
  tsm.stop();
  massert(tsm.is_terminated(), "sanity");

  return ATM_OK;
}
