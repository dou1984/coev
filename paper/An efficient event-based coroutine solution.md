# An efficient event-driven coroutine solution

Date: 2023-10-01
Auther: Zhao Yun Shan
Project: Programming Language C++

Contents
...

## Motivation

The C++20 coroutine library offers a powerful solution for high-performance coroutines, yet its widespread adoption has been hindered by the absence of a universal encapsulation mechanism. To address this issue, the author has introduced an innovative approach—a set of event-driven encapsulations that cleanly separate coroutine development from business logic. This breakthrough empowers developers to effortlessly transition existing asynchronous programs into coroutine-based ones without the burdensome task of delving into the intricacies of coroutine encapsulation.

## event-driven coroutine solution

Up until now, the author has undertaken numerous projects involving asynchronous frameworks. The arduous process of converting each asynchronous process class into a coroutine class posed a significant challenge. In response, the author has ingeniously designed an event-driven coroutine library capable of swiftly converting asynchronous programs into coroutines. This groundbreaking approach liberates developers from the necessity of grappling with C++20 coroutine intricacies or crafting new coroutine classes to facilitate the transition of asynchronous programs into coroutine-based ones.

The author has encapsulated four pivotal classes—namely, **awaiter**, **event**, **async** and **task**—utilizing the C++20 coroutine framework. The core principle underpinning these subpackage classes is their event-driven nature. Developers are no longer burdened with the task of intertwining C++20 coroutine code with file I/O, network operations, or pipeline handling. Instead, they can create highly abstract coroutine code that remains entirely segregated from other modules. During the development process, the sole responsibility lies in preserving the context while waiting for data completion and suspending the coroutine. When the data becomes available, developers can seamlessly trigger the context restoration, enabling the coroutine to resume its execution seamlessly.


## Principle

+ **event** is the minimal requirement coroutine class, responsible only for event driving. It does not generate new **coroutine_handle**. When waiting for data preparation, `co_await event` can immediately suspend the **awaiter** coroutine. When data is ready, it can resume the awaiter coroutine asynchronously. The event serves as the core of event-driven mechanisms.
+ **awaiter** is an encapsulated coroutine class that generate new **coroutine_handle**. When waiting for data preparation, you can use `co_await` on **events** and **awaiters** within the **awaiter** funtion body, suspending the current **awaiter**. When the **awaiter** function body completes (`co_return` or `co_yield`), it will resume to the upper-level **awaiter** that called it, allowing the function body of the upper-level **awaiter** to continue execution.
+ **async** is a collection of events used to store multiple **event**, allowing for the waiting of **event** in different categories. It is used for multiple **awaiters** to sequentially await **event**.
+ **task** is used when simultaneously waiting for multiple **awaiter** coroutines. **task** can be used to await the completion of multiple independent coroutine simultaneously. Different combinations of tasks can accomplish complex workflows.

## event

**event** is the most basic coroutine class, responsible only for event-driven operations. The member variable **m_awaiting** within the class represents the **awaiter** created for the **event**. When data is ready, it resumes execution in the coroutine function body via m_awaiting.

```cpp
struct event final : chain // chain is a simplified list
{
  std::atomic_int m_status{INIT};
  std::coroutine_handle<> m_awaiter = nullptr;// waiting for the upper coroutine_handle of the event
  event(chain *_eventchain);
  virtual ~event();
  void await_resume();
  bool await_ready();
  void await_suspend(std::coroutine_handle<> awaiter);
  void resume();
};
```

# async

The usage of an “event list” (**evl**) for storing events. When waiting for data, the **wait_for** method is called, which inserts an event into the evl. At this point, the awaiter coroutine enters a waiting state. When it’s necessary to resume the coroutine, the resume function of the event can be invoked, allowing the awaiter function body to continue executing. Additionally, evlts represents a thread-safe “event list,” protecting the data within the event list in multi-threaded programs.

**async** is a collection of events. **async** contains multiple “event lists”, which can be distinguished by using tags. For example, I/O read and write events can be separated into two independent event lists for storage.

```cpp
struct evl : chain {};
struct evlts : chain, std::mutex {};
template <typename T>
concept async_t = std::is_same<T, evl>::value || std::is_same<T, evlts>::value;
template <async_t G = evl, async_t... T>
struct async : std::tuple<G, T...>
{
};
```

## awaiter
**awaiter** is a coroutine wrapper class. When **co_await** is used within an awaiter, it creates a **coroutine_handle** and records the **coroutine_handle** of the caller’s **awaiter**. When the **awaiter** function body completes execution, the current **coroutine_handle** is destroyed, and the caller’s resume function is invoked, allowing the execution to resume within the caller’s awaiter function body.

Developer can wait for an **event** within the **awaiter** function body. The **wait_for** method is used to wait for the event. When the **event** is completed, the resume function is called, allowing the **awaiter** coroutine function body to continue executing.
```cpp
struct awaiter final : taskevent // Taskevent is used when waiting for multiple coroutines to completed.
{
  struct promise_type 
  {   
   int value;   
   awaiter *m_awaiter = nullptr; // current awaiter
   promise_type() = default;
   awaiter get_return_object();
   std::suspend_never return_value(int v);
   std::suspend_never yield_value(int v); 
   // Part of the implementation is omitted here
  };
  awaiter(std::coroutine_handle<promise_type> h); 
  void resume(); // resume to the caller's awaiter
  bool done() { return m_coroutine ? m_coroutine.done() : true; }
  bool await_ready() { return m_ready; } 
  void await_suspend(std::coroutine_handle<> caller) { m_caller = caller; }
  auto await_resume() { return m_callee ? m_callee.promise().value : 0; }
  void destroy(); // terminate current coroutine force.
  std::coroutine_handle<promise_type> m_callee = nullptr;
  std::coroutine_handle<> m_caller = nullptr;		
  std::atomic_int m_state {INIT};
  // Part of the implementation is omitted here
};
```

Use "wait_for" an event in awaiter. Wait_for will insert the event into the eventchain and wait for the data complished and event to be triggered. When the data is ready and the event is triggered, you can resume the event in the eventchain and the event will be restored to the waiting awaiter.


```cpp
async<evl> g_triger;
awaiter co_waiting()
{ 
 co_await wait_for<0>(&g_trigger); // waiting for data 
 co_return 0;
}
awaiter co_trigger()
{
 co_await sleep_for(1); //sleep 1 second
 resume<0>(g_trigger);  // the data complished, jump to co_waiting and continue executing
 co_return 0;
}
```

You can start multiple **awaiter** coroutines and wait for events at the same time. When the data is completed and the event is triggered, the awaiter coroutines can be resumed one by one.

```cpp
awaiter co_waiting_first()
{ 
 co_return co_await wait_for<0>(&g_trigger); 
}
awaiter co_waiting_second()
{ 
 co_return co_await wait_for<0>(&g_trigger); 
}
void co_trigger()
{
 while(resume<0>(&g_trigger));//Resume waiting awaiters one by one
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
   co_await wait_for<EVEvent>(&w);
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
  co_await wait_for<EVEvent>(&w);
  w.destroy();
  co_return 0;
}
awaiter co_wait_incomplated()
{
 co_await wait_for_any(sleep_for(1), sleep_for(2));
}
```

# Summarize

All awaiter coroutines are event-driven. When you need to wait for an event, the event is saved in the async. When the event is completed, resume the event and jump to the waiting awaiter. When the awaiter completes, you can return to the caller awaiter. Developers only need to understand awaiter, event, async and wait_for functions. They do not need to understand promise_type, coroutine_handle and other processes, nor do they need to re-encapsulate coroutines according to different businesses. This can greatly reduce the difficulty of development and is also consistent with The development thinking and habits of most people.