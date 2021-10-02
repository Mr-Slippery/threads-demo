#pragma once

#include <functional>

enum class thread_state: uint8_t {
    PAUSED,
    RUNNING,
    STOPPING,
    TERMINATED
};

namespace std {
  template <>
  struct hash<thread_state>
  {
    std::size_t operator()(const thread_state& k) const
    {
      using std::hash;
      return hash<uint8_t>()((uint8_t)k);
    }
  };
}
