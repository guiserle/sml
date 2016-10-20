//
// Copyright (c) 2016 Krzysztof Jusiak (krzysztof at jusiak dot net)
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
#ifndef BOOST_SML_BACK_STATE_TRANSITIONS_HPP
#define BOOST_SML_BACK_STATE_TRANSITIONS_HPP

namespace detail {

template <class>
class sm_impl;

struct _ {};

template <class TException>
struct exception : internal_event {
  using type = TException;
  explicit exception(const TException &exception = {}) : exception_(exception) {}
  TException exception_;
};

template <class TEvent = _>
struct unexpected_event : internal_event {
  using type = TEvent;
  explicit unexpected_event(const TEvent &event = {}) : event_(event) {}
  TEvent event_;
};

template <class...>
struct transitions;

template <class...>
struct transitions_sub;

template <class T, class... Ts>
struct transitions<T, Ts...> {
  template <class SM, class TEvent>
  static bool execute(SM &self, const TEvent &event, typename SM::state_t &current_state) {
    if (aux::get<T>(self.me_.transitions_).execute(self, event, current_state)) {
      return true;
    }
    return transitions<Ts...>::execute(self, event, current_state);
  }
};

template <class T>
struct transitions<T> {
  template <class SM, class TEvent>
  static bool execute(SM &self, const TEvent &event, typename SM::state_t &current_state) {
    return aux::get<T>(self.me_.transitions_).execute(self, event, current_state);
  }

  template <class SM, class>
  static bool execute(SM &self, const on_exit &event, typename SM::state_t &current_state) {
    aux::get<T>(self.me_.transitions_).execute(self, event, current_state);
    return false;  // from bottom to top
  }
};

template <>
struct transitions<> {
  template <class SM, class TEvent>
  static bool execute(SM &self, const TEvent &event, typename SM::state_t &current_state) {
    self.me_.process_internal_event(self, unexpected_event<TEvent>{event}, current_state);
    return false;
  }
};

template <class TSM, class T, class... Ts>
struct transitions_sub<sm<TSM>, T, Ts...> {
  template <class SM, class TEvent>
  static bool execute(SM &self, const TEvent &event, typename SM::state_t &current_state) {
    const auto handled = aux::try_get<sm_impl<TSM>>(&self.sub_sms_).process_event(event, self.deps_, self.sub_sms_);
    return handled ? handled : transitions<T, Ts...>::execute(self, event, current_state);
  }

  template <class SM, class>
  static bool execute(SM &self, const on_entry &event, typename SM::state_t &current_state) {
    transitions<T, Ts...>::execute(self, event, current_state);
    aux::try_get<sm_impl<TSM>>(&self.sub_sms_).process_event(event, self.deps_, self.sub_sms_);
    return true;  // from top to bottom
  }
};

template <class TSM>
struct transitions_sub<sm<TSM>> {
  template <class SM, class TEvent>
  static bool execute(SM &self, const TEvent &event, typename SM::state_t &) {
    aux::try_get<sm_impl<TSM>>(&self.sub_sms_).process_event(event, self.deps_, self.sub_sms_);
    return true;
  }

  template <class SM>
  static bool execute(SM &self, const on_entry &event, typename SM::state_t &) {
    return aux::try_get<sm_impl<TSM>>(&self.sub_sms_).process_event(event, self.deps_, self.sub_sms_);
  }
};

}  // detail
#endif