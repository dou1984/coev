# An efficient event-based coroutine solution

## Purpose

The c++20 coroutine library is a high-performance coroutine library solution, but it has not become popular, mainly because there is no universal encapsulation.
 The author provides a set of event-driven encapsulation that separates coroutine development from business code. Developers do not need to care about the encapsulation implementation of coroutines and can quickly convert existing asynchronous programs into coroutine programs.

## event-based coroutine solution

Until now, the author has many projects with asynchronous framework. It is a heavy workload to transform each class of these asynchronous processes into a coroutine class. Therefore, the author designed an event-driven coroutine library that can quickly convert asynchronous programs into As a coroutine, other developers no longger need to understand the C++20 coroutine or encapsulate a new coroutine class to quickly convert asynchronous programs into coroutines.

The author encapsulates three classes awaiter, event, and eventchain through the c++20 coroutine. The main idea of ​​these three subpackage classes is to be event-driven. Developers no longer need to combine c++20 coroutine with files I/O, networks, and pipelines etc. codes are mixed to achieve highly abstract coroutine code and completely separated from other modules. In the development process, the only thing need to store the context when we wait for data complished and suspend coroutine . When the data is ready , we can trigger the recovery context and continue the coroutine.

# event

event is the smallest coroutine class, used to quickly convert asynchronous calls into coroutines. "eventchain" and "wait_for<eventchain>" cooperate with each other to quickly implement coroutines.

```cpp
struct event final : chain // chain is a simplified list
{
  std::coroutine_handle<> m_awaiting = nullptr; // waiting for the upper coroutine_handle of the event
  uint64_t m_tid = 0;
  std::atomic_bool m_ready{false}; 
  event(chain *_eventchain);
  virtual ~event();
  void await_resume();
  bool await_ready() { return false; }
  void await_suspend(std::coroutine_handle<> awaiting)；
  void resume();
  // Part of the implementation is omitted here
};
```

The eventchain is used to store the "list" of events. When the coroutine needs to be resumed, calling the resume function can jump to the waiting awaiter.

```cpp
template <class TYPE>
struct eventchain : chain, TYPE
{
  bool resume()
  {
    if (chain::empty())
      return false;
    auto c = static_cast<event *>(chain::pop_front());
    c->resume();
    return true;
  }
};
  ```

  Awaiter is a coroutine encapsulation class that can be used to start a coroutine. The function can wait for the event event to complete.

```cpp
struct awaiter final : taskevent // Taskevent is used when waiting for multiple coroutines to completed.
{
  struct promise_type 
  {   
   int value;   
   awaiter *m_awaiter = nullptr; // current awaiter
   promise_type() = default;
   ~promise_type();
   awaiter get_return_object();
   std::suspend_never return_value(int v);
   std::suspend_never yield_value(int v); 
   // Part of the implementation is omitted here
  };
  awaiter(std::coroutine_handle<promise_type> h) : m_coroutine(h)
  {
    m_coroutine.promise().m_awaiter = this;
  }
  void resume(); // resume to the upper awaiter
  bool done() { return m_coroutine ? m_coroutine.done() : true; }
  bool await_ready() { return m_ready; } 
  void await_suspend(std::coroutine_handle<> awaiting) { m_awaiting = awaiting; }
  auto await_resume() { return m_coroutine ? m_coroutine.promise().value : 0; }
  void destroy(); // terminate a waiting coroutine force.
  std::coroutine_handle<promise_type> m_coroutine = nullptr;
  std::coroutine_handle<> m_awaiting = nullptr; 
  bool m_ready = false;
  // Part of the implementation is omitted here
};
```

Use "wait_for" an event in awaiter. Wait_for will insert the event into the eventchain and wait for the data complished and event to be triggered. When the data is ready and the event is triggered, you can resume the event in the eventchain and the event will be restored to the waiting awaiter.


```cpp
using EVRecv = eventchain<RECV>; //give eventchain a new name，用以区分不同的等待事件
struct Trigger : EVRecv
{
} g_trigger;

template <class EVCHAIN, class OBJ>
event wait_for(OBJ &obj)
{
 EVCHAIN *ev = &obj;
 return event{ev};
}
awaiter co_waiting()
{ 
 co_return co_await wait_for<EVRecv>(g_trigger); // 等待数据准备及事件触发
}
awaiter co_trigger()
{
 co_await sleep_for(5); //等待5秒
 g_trigger.EVRecv::resume();  // 数据准备完成、触发事件时，跳转到协程co_waiting
 co_return 0;
}
```

You can start multiple awaiter coroutines and wait for events in the event chain at the same time. When the data preparation is completed and the event is triggered, the awaiter coroutines can be resumed one by one.

```cpp
awaiter co_waiting_first()
{ 
 co_return co_await wait_for<EVRecv>(g_trigger); 
}
awaiter co_waiting_second()
{ 
 co_return co_await wait_for<EVRecv>(g_trigger); 
}
void co_trigger()
{
 while(g_trigger.EVRecv::resume());//Resume waiting awaiters one by one
}
```

The awaiter coroutine can be nested in the awaiter's coroutine. Developers can encapsulate different libraries and then hand them over to other developers to encapsulate them on this basis. Developers do not need to encapsulate the C++ coroutine again.

  ```cpp
 template <class... T>
 awaiter wait_for_all(T &&..._task)
 {
  task w; //task用于存储awaiter
  (w.insert_task(&_task), ...);
  while (!w.empty()) 
   co_await wait_for<EVEvent>(w);
  co_return 0;
}
awaiter co_wait_complated()
{
  co_await wait_for_all(sleep_for(1), sleep_for(2));
}
```


wait_for_any is used to wait for any one of multiple awaiter coroutines to complete. When a waiting awaiter coroutine completes, wait_for_any will return.

```cpp
template <class... T>
awaiter wait_for_any(T &&..._task)
{
  task w;
  (w.insert_task(&_task), ...);
  co_await wait_for<EVEvent>(w);
  w.destroy();
  co_return 0;
}
awaiter co_wait_incomplated()
{
 co_await wait_for_any(sleep_for(1), sleep_for(2));
}
```

Eventchain is not thread-safe. If you use eventchain to wait for events in multiple threads at the same time, you need to add thread locks.
At the same time, there is another problem. If you wait for an event in thread A, and after thread B resumes the event, the code after the awaiter resumes will be executed in thread B. This is a problem that needs attention.

# Summarize

All awaiter coroutines are event-driven. When you need to wait for an event, the event is saved in the event chain. When the event is completed, resume the event and jump to the waiting awaiter. When the awaiter completes, you can return to the upper layer to wait. awaiter in. Developers only need to understand awaiter, event, eventchain and wait_for functions. They do not need to understand promise_type, coroutine_handle and other processes, nor do they need to re-encapsulate coroutines according to different businesses. This can greatly reduce the difficulty of development and is also consistent with The development thinking and habits of most people.