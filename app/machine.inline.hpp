#ifndef APP_MACHINE_INLINE_HPP_
#define APP_MACHINE_INLINE_HPP_

#include "machine.hpp"

NAMESPACE_BEGIN

namespace machine {
template <typename FSM>
void TendingDef::spraying::idle::on_enter(event::spraying::start&& event,
                                          FSM&                     fsm) const {
  return;
}

template <typename FSM>
void TendingDef::spraying::preparation::on_enter(
    event::spraying::prepare&& event,
    FSM&                       fsm) const {
  return;
}

template <typename FSM>
void TendingDef::spraying::preparation::on_exit(event::spraying::run&& event,
                                                FSM& fsm) const {
  return;
}
}  // namespace machine

NAMESPACE_END

#endif  // APP_MACHINE_INLINE_HPP_
