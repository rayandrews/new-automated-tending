#include "mechanism.hpp"

#include "movement.hpp"

#include <cmath>
#include <thread>

#include <libutil/util.hpp>

NAMESPACE_BEGIN

namespace mechanism {
namespace impl {
MovementBuilderImpl::MovementBuilderImpl() {}

ATM_STATUS MovementBuilderImpl::setup_x(
    const std::string&           stepper_x_id,
    const device::stepper::step& steps_per_mm,
    const std::string&           limit_switch_x_id) {
  if (!device::StepperRegistry::get()->exist(stepper_x_id)) {
    return ATM_ERR;
  }

  if (!device::DigitalInputDeviceRegistry::get()->exist(limit_switch_x_id)) {
    return ATM_ERR;
  }

  if (steps_per_mm == 0) {
    return ATM_ERR;
  }

  stepper_x_id_ = stepper_x_id;
  limit_switch_x_id_ = limit_switch_x_id;
  steps_per_mm_x_ = steps_per_mm;
  return ATM_OK;
}

ATM_STATUS MovementBuilderImpl::setup_y(
    const std::string&           stepper_y_id,
    const device::stepper::step& steps_per_mm,
    const std::string&           limit_switch_y_id) {
  if (!device::StepperRegistry::get()->exist(stepper_y_id)) {
    return ATM_ERR;
  }

  if (!device::DigitalInputDeviceRegistry::get()->exist(limit_switch_y_id)) {
    return ATM_ERR;
  }

  if (steps_per_mm == 0) {
    return ATM_ERR;
  }

  stepper_y_id_ = stepper_y_id;
  limit_switch_y_id_ = limit_switch_y_id;
  steps_per_mm_y_ = steps_per_mm;
  return ATM_OK;
}

ATM_STATUS MovementBuilderImpl::setup_z(
    const std::string&           stepper_z_id,
    const device::stepper::step& steps_per_mm,
    const std::string&           limit_switch_z_top_id,
    const std::string&           limit_switch_z_bottom_id) {
  if (!device::StepperRegistry::get()->exist(stepper_z_id)) {
    return ATM_ERR;
  }

  if (!device::DigitalInputDeviceRegistry::get()->exist(
          limit_switch_z_top_id)) {
    return ATM_ERR;
  }

  if (!device::DigitalInputDeviceRegistry::get()->exist(
          limit_switch_z_bottom_id)) {
    return ATM_ERR;
  }

  if (steps_per_mm == 0) {
    return ATM_ERR;
  }

  stepper_z_id_ = stepper_z_id;
  limit_switch_z_top_id_ = limit_switch_z_top_id;
  limit_switch_z_bottom_id_ = limit_switch_z_bottom_id;
  steps_per_mm_z_ = steps_per_mm;
  return ATM_OK;
}

std::shared_ptr<Movement>& MovementBuilderImpl::build() {
  if (movement_mechanism_instance_ == nullptr) {
    movement_mechanism_instance_ = Movement::create(this);
  }

  massert(movement_mechanism_instance_ != nullptr, "sanity");

  return movement_mechanism_instance_;
}
}  // namespace impl

Movement::Movement(const impl::MovementBuilderImpl* builder)
    : builder_{builder},
      // steps_per_mm_{builder->steps_per_mm()},
      thread_pool_{4} {
  active_ = true;
  ready_ = true;
  next_move_interval_ = 0;
  last_move_end_ = 0;
  event_timer_x_ = 0;
  event_timer_y_ = 0;
  event_timer_z_ = 0;

  setup_stepper();
  if (active()) {
    setup_limit_switch();
  }
}

template <>
long Movement::convert_length_to_steps<movement::unit::cm>(
    double                       length,
    const device::stepper::step& steps_per_mm) {
  return length * steps_per_mm / 100.0;
}

template <>
long Movement::convert_length_to_steps<movement::unit::mm>(
    double                       length,
    const device::stepper::step& steps_per_mm) {
  return length * steps_per_mm;
}

void Movement::setup_stepper() {
  auto stepper_registry = device::StepperRegistry::get();
  auto stepper_x = stepper_registry->get(builder()->stepper_x_id());

  if (!stepper_x) {
    active_ = false;
    return;
  }

  stepper_x_ = stepper_x;

  auto stepper_y = stepper_registry->get(builder()->stepper_y_id());

  if (!stepper_y) {
    active_ = false;
    return;
  }

  stepper_y_ = stepper_y;

  auto stepper_z = stepper_registry->get(builder()->stepper_z_id());

  if (!stepper_z) {
    active_ = false;
    return;
  }

  stepper_z_ = stepper_z;
}

void Movement::setup_limit_switch() {
  auto digital_input_registry = device::DigitalInputDeviceRegistry::get();

  auto limit_switch_x =
      digital_input_registry->get(builder()->limit_switch_x_id());

  if (!limit_switch_x) {
    active_ = false;
    return;
  }

  limit_switch_x_ = limit_switch_x;

  auto limit_switch_y =
      digital_input_registry->get(builder()->limit_switch_y_id());

  if (!limit_switch_y) {
    active_ = false;
    return;
  }

  limit_switch_y_ = limit_switch_y;

  auto limit_switch_z_top =
      digital_input_registry->get(builder()->limit_switch_z_top_id());

  if (!limit_switch_z_top) {
    active_ = false;
    return;
  }

  limit_switch_z_top_ = limit_switch_z_top;

  auto limit_switch_z_bottom =
      digital_input_registry->get(builder()->limit_switch_z_bottom_id());

  if (!limit_switch_z_bottom) {
    active_ = false;
    return;
  }

  limit_switch_z_bottom_ = limit_switch_z_bottom;
}

const device::stepper::step Movement::stop_x(void) {
  return stepper_x()->stop();
}

const device::stepper::step Movement::stop_y(void) {
  return stepper_y()->stop();
}

const device::stepper::step Movement::stop_z(void) {
  return stepper_z()->stop();
}

void Movement::stop(void) {
  [[maybe_unused]] auto step_x = stop_x();
  [[maybe_unused]] auto step_y = stop_y();
  [[maybe_unused]] auto step_z = stop_z();
}

void Movement::start_move(const long& x, const long& y, const long& z) {
  const time_unit time_x = stepper_x()->time_for_move(x);
  const time_unit time_y = stepper_y()->time_for_move(y);
  const time_unit time_z = stepper_z()->time_for_move(z);

  // find which motor would take the longest to finish,
  const time_unit move_time = std::max(time_x, std::max(time_y, time_z));

  LOG_DEBUG("Will move about {} micros", move_time);

  // start moving x
  if (x == 0) {
    event_timer_x_ = 0;
  } else {
    stepper_x()->start_move(x, move_time);
    event_timer_x_ = 1;
  }

  // start moving y
  if (y == 0) {
    event_timer_y_ = 0;
  } else {
    stepper_y()->start_move(y, move_time);
    event_timer_y_ = 1;
  }

  // start moving z
  if (z == 0) {
    event_timer_z_ = 0;
  } else {
    stepper_z()->start_move(z, move_time);
    event_timer_z_ = 1;
  }

  ready_ = false;
  last_move_end_ = 0;
  next_move_interval_ = 1;
}

time_unit Movement::next() {
  while ((micros() - last_move_end()) < next_move_interval()) {
    // not yet running
  }
  // sleep_until<time_units::micros>(next_move_interval(), last_move_end());

  bool next_x = false;
  bool next_y = false;
  bool next_z = false;

  std::future<time_unit> timer_x;
  std::future<time_unit> timer_y;
  std::future<time_unit> timer_z;

  if (event_timer_x() <= next_move_interval()) {
    next_x = true;
    // event_timer_x_ = stepper_x()->next(
    //     limit_switch_x()->read().value_or(device::digital::value::low) ==
    //     device::digital::value::high);

    timer_x = thread_pool().enqueue([this] {
      return stepper_x()->next(
          limit_switch_x()->read().value_or(device::digital::value::low) ==
          device::digital::value::high);
    });
  } else {
    event_timer_x_ -= next_move_interval();
  }

  if (event_timer_y() <= next_move_interval()) {
    next_y = true;
    // event_timer_y_ = stepper_y()->next(
    //     limit_switch_y()->read().value_or(device::digital::value::low) ==
    //     device::digital::value::low);
    timer_y = thread_pool().enqueue([this] {
      return stepper_y()->next(
          limit_switch_y()->read().value_or(device::digital::value::low) ==
          device::digital::value::high);
    });
  } else {
    event_timer_y_ -= next_move_interval();
  }

  if (event_timer_z() <= next_move_interval()) {
    next_z = true;
    // const auto limit_switch_z_top_touched =
    //     limit_switch_z_top()->read().value_or(device::digital::value::low) ==
    //     device::digital::value::high;
    // const auto limit_switch_z_bottom_touched =
    //     limit_switch_z_bottom()->read().value_or(device::digital::value::low)
    //     == device::digital::value::high;
    // event_timer_z_ = stepper_z()->next(limit_switch_z_top_touched ||
    //                                    limit_switch_z_bottom_touched);
    timer_z = thread_pool().enqueue([this] {
      const auto limit_switch_z_top_touched =
          limit_switch_z_top()->read().value_or(device::digital::value::low) ==
          device::digital::value::high;
      const auto limit_switch_z_bottom_touched =
          limit_switch_z_bottom()->read().value_or(
              device::digital::value::low) == device::digital::value::high;
      return stepper_z()->next(limit_switch_z_top_touched ||
                               limit_switch_z_bottom_touched);
    });
  } else {
    event_timer_z_ -= next_move_interval();
  }

  if (next_x) {
    event_timer_x_ = timer_x.get();
  }

  if (next_y) {
    event_timer_y_ = timer_y.get();
  }

  if (next_z) {
    event_timer_z_ = timer_z.get();
  }

  last_move_end_ = micros();
  next_move_interval_ = 0;

  // Find the time when the next pulse needs to fire
  // this is the smallest non-zero timer value from all active motorsA
  if (event_timer_x() > 0 &&
      (event_timer_x() < next_move_interval() || next_move_interval() == 0)) {
    next_move_interval_ = event_timer_x();
  }

  if (event_timer_y() > 0 &&
      (event_timer_y() < next_move_interval() || next_move_interval() == 0)) {
    next_move_interval_ = event_timer_y();
  }

  if (event_timer_z() > 0 &&
      (event_timer_z() < next_move_interval() || next_move_interval() == 0)) {
    next_move_interval_ = event_timer_z();
  }

  ready_ = (next_move_interval() == 0);

  return next_move_interval();
}

void Movement::homing() {
  LOG_INFO("Homing is started...");
  // enabling motor
  enable_motors();

  // homing z
  bool z_completed =
      limit_switch_z_top()->read().value_or(device::digital::value::low) ==
      device::digital::value::high;
  while (!z_completed) {
    z_completed =
        limit_switch_z_top()->read().value_or(device::digital::value::low) ==
        device::digital::value::high;
    LOG_DEBUG("Still homing Z-Axis...");
    stepper_z()->move(-400, z_completed);
  }

  // homing x and y
  auto result = thread_pool().enqueue([this] {
    bool is_x_completed =
        limit_switch_x()->read().value_or(device::digital::value::low) ==
        device::digital::value::high;
    bool is_y_completed =
        limit_switch_y()->read().value_or(device::digital::value::low) ==
        device::digital::value::high;

    while (!is_x_completed || !is_y_completed) {
      long steps_x = 0;
      long steps_y = 0;

      if (!is_x_completed) {
        steps_x = -20;
      }
      if (!is_y_completed) {
        steps_y = -20;
      }

      start_move(steps_x, steps_y, 0.0);
      while (!ready()) {
        next();
      }

      is_x_completed =
          limit_switch_x()->read().value_or(device::digital::value::low) ==
          device::digital::value::high;
      is_y_completed =
          limit_switch_y()->read().value_or(device::digital::value::low) ==
          device::digital::value::high;
    }

    return is_x_completed && is_y_completed;
  });

  [[maybe_unused]] bool finished = result.get();

  // disabling motor
  disable_motors();

  LOG_INFO("Homing is finished...");
}

void Movement::enable_motors() const {
  LOG_INFO("Enabling motors...");
  stepper_x()->enable();
  stepper_y()->enable();
  stepper_z()->enable();
}

void Movement::disable_motors() const {
  LOG_INFO("Disabling motors...");
  stepper_x()->disable();
  stepper_y()->disable();
  stepper_z()->disable();
}
}  // namespace mechanism

NAMESPACE_END
