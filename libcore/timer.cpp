#include "core.hpp"

#include "timer.hpp"

#include <chrono>
#include <thread>

NAMESPACE_BEGIN

time_unit millis() {
  uint64_t ms =
      std::chrono::duration_cast<std::chrono::milliseconds>(
          std::chrono::high_resolution_clock::now().time_since_epoch())
          .count();
  return static_cast<time_unit>(ms);
}

template <>
void sleep_for<time_units::millis>(time_unit time) {
  std::this_thread::sleep_for(std::chrono::milliseconds(time));
}

template <>
void sleep_until<time_units::millis>(time_unit time, time_unit start_time) {
  time_unit start = start_time;

  if (start == 0) {
    start = millis();
  }

  std::this_thread::sleep_until(std::chrono::high_resolution_clock::time_point{
      std::chrono::milliseconds(start + time)});
}

time_unit micros() {
  uint64_t us =
      std::chrono::duration_cast<std::chrono::microseconds>(
          std::chrono::high_resolution_clock::now().time_since_epoch())
          .count();
  return static_cast<time_unit>(us);
}

template <>
void sleep_for<time_units::micros>(time_unit time) {
  std::this_thread::sleep_for(std::chrono::microseconds(time));
}

template <>
void sleep_until<time_units::micros>(time_unit time, time_unit start_time) {
  time_unit start = start_time;

  if (start == 0) {
    start = micros();
  }

  std::this_thread::sleep_until(std::chrono::high_resolution_clock::time_point{
      std::chrono::microseconds(start + time)});
}

time_unit nanos() {
  uint64_t ns =
      std::chrono::duration_cast<std::chrono::nanoseconds>(
          std::chrono::high_resolution_clock::now().time_since_epoch())
          .count();
  return static_cast<time_unit>(ns);
}

template <>
void sleep_for<time_units::nanos>(time_unit time) {
  std::this_thread::sleep_for(std::chrono::nanoseconds(time));
}

template <>
void sleep_until<time_units::nanos>(time_unit time, time_unit start_time) {
  time_unit start = start_time;

  if (start == 0) {
    start = nanos();
  }

  std::this_thread::sleep_until(std::chrono::high_resolution_clock::time_point{
      std::chrono::nanoseconds(start + time)});
}

NAMESPACE_END