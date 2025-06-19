#pragma once
#include <coroutine>
namespace coev
{

  struct suspend_bool
  {
    bool m_ready = false;
    constexpr bool await_ready() const noexcept { return m_ready; }
    constexpr void await_suspend(std::coroutine_handle<>) const noexcept {}
    constexpr void await_resume() const noexcept {}
  };
}