# An efficient event-driven coroutine solution

Date: 2023-10-01
Auther: Zhao Yun Shan
Project: Programming Language C++

## Contents
|[motivation](#motivation) |
[solution](#solution) |
[principle](#principle) |
[event](#event) |
[async](#async) |
[awaiter](#awaiter) |
[task](#task) |
[summarize](#summarize) |

# Motivation

&nbsp;&nbsp;&nbsp;&nbsp;The C++20 coroutine library offers a powerful solution for high-performance coroutines, yet its widespread adoption has been hindered by the absence of a universal encapsulation mechanism. To address this issue, the author has introduced an innovative approach—a set of event-driven encapsulations that cleanly separate coroutine development from business logic. This breakthrough empowers developers to effortlessly transition existing asynchronous programs into coroutine-based ones without the burdensome task of delving into the intricacies of coroutine encapsulation.

# 回顾

In modern programming and concurrency processing domains, coroutines are considered an extremely important feature because they effectively address complexities and performance bottlenecks inherent in traditional thread models. Coroutines enable a program to pause and resume execution without losing their coroutine state, significantly simplifying asynchronous programming paradigms and enhancing code readability and maintainability. By leveraging coroutines, developers can write non-blocking, highly efficient concurrent code in a more intuitive manner, particularly in scenarios involving I/O-intensive applications, game development, network programming, and other situations that demand efficient utilization of system resources.

# Solution
&nbsp;&nbsp;&nbsp;&nbsp;Up until now, the author has undertaken numerous projects involving asynchronous frameworks. The arduous process of converting each asynchronous process class into a coroutine class posed a significant challenge. In response, the author has ingeniously designed an event-driven coroutine library capable of swiftly converting asynchronous programs into coroutines. This groundbreaking approach liberates developers from the necessity of grappling with C++20 coroutine intricacies or crafting new coroutine classes to facilitate the transition of asynchronous programs into coroutine-based ones.

&nbsp;&nbsp;&nbsp;&nbsp;The author has encapsulated four pivotal classes—namely, **awaiter**, **event**, **async** and **task**—utilizing the C++20 coroutine framework. The core principle underpinning these subpackage classes is their event-driven nature. Developers are no longer burdened with the task of intertwining C++20 coroutine code with file I/O, network operations, or pipeline handling. Instead, they can create highly abstract coroutine code that remains entirely segregated from other modules. During the development process, the sole responsibility lies in preserving the context while waiting for data completion and suspending the coroutine. When the data becomes available, developers can seamlessly trigger the context restoration, enabling the coroutine to resume its execution seamlessly.

# Principle

+ **event** is the minimal requirement coroutine class, responsible only for event driving. It does not generate new **coroutine_handle**. When waiting for data preparation, `co_await event` can immediately suspend the **awaiter** coroutine. When data is ready, it can resume the awaiter coroutine asynchronously. The event serves as the core of event-driven mechanisms.
+ **awaiter** is an encapsulated coroutine class that generate new **coroutine_handle**. When waiting for data preparation, you can use `co_await` on **events** and **awaiters** within the **awaiter** funtion body, suspending the current **awaiter**. When the **awaiter** function body completes (`co_return` or `co_yield`), it will resume to the upper-level **awaiter** that called it, allowing the function body of the upper-level **awaiter** to continue execution.
+ **async** is a collection of events used to store multiple **event**, allowing for the waiting of **event** in different categories. It is used for multiple **awaiters** to sequentially await **event**.
+ **task** is used when simultaneously waiting for multiple **awaiter** coroutines. **task** can be used to await the completion of multiple independent coroutine simultaneously. Different combinations of tasks can accomplish complex workflows.

# event

&nbsp;&nbsp;&nbsp;&nbsp;**event** is the most basic coroutine class, responsible only for event-driven operations. The member variable **m_awaiter** within the class represents the **awaiter** created for the **event**. When data becomes available, it resumes execution in the coroutine function body via **m_awaiter**.

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

&nbsp;&nbsp;&nbsp;&nbsp;**evl** is an “event list” usd for storing events. When waiting for data, the **wait_for** method is called, which inserts an event into the evl. At this point, the awaiter coroutine enters a waiting state. When it’s necessary to resume the coroutine, the resume function of the event can be invoked, allowing the awaiter function body to continue executing. Additionally, evlts represents a thread-safe "event list", protecting the data within the event list in multi-threaded programs.

&nbsp;&nbsp;&nbsp;&nbsp;**async** is a collection of events. **async** contains multiple "event lists", which can be distinguished by using tags. For example, I/O read and write events can be separated into two independent "event lists" for storage.

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

# awaiter

&nbsp;&nbsp;&nbsp;&nbsp;**awaiter** is a coroutine wrapper class. When `co_await` is used within an **awaiter**, it creates a **coroutine_handle** and records the **coroutine_handle** of the caller’s **awaiter**. When the **awaiter** function body completes execution, the current **coroutine_handle** is destroyed, and the caller’s `resume` function is invoked, allowing the execution to resume within the caller’s **awaiter** function body.

&nbsp;&nbsp;&nbsp;&nbsp;Developer can wait for an **event** within the **awaiter** function body. The `wait_for` method is used to wait for the event. When the **event** is completed, the `resume` function is called, allowing the **awaiter** coroutine function body to continue executing.

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
  bool await_ready() { return m_state == READY; } 
  void await_suspend(std::coroutine_handle<> caller) { m_caller = caller; }
  auto await_resume() { return m_callee ? m_callee.promise().value : 0; }
  void destroy(); // terminate current coroutine force.
  std::coroutine_handle<promise_type> m_callee = nullptr;
  std::coroutine_handle<> m_caller = nullptr;		
  std::atomic_int m_state {INIT};
  // Part of the implementation is omitted here
};
```

&nbsp;&nbsp;&nbsp;&nbsp;In an **awaiter**, when using `wait_for` with an **event**, the `wait_for` function inserts the event into the **async** and waits for data and an event trigger. When the data is ready or the event is triggered, **async** can asynchronously invoke the `resume` function of the **event**, which will resume execution in the caller's **awaiter**. The async can contain multiple **event lists**.

```cpp
async<evl> g_triger;
awaiter co_waiting()
{ 
 co_await wait_for<0>(&g_trigger); // waiting for date and an event trigger
 co_return 0;
}
awaiter co_trigger()
{
 co_await sleep_for(1);   // sleep for 1 second.
 resume<0>(g_trigger);    // data is ready, resume and go to co_waiting 
 co_return 0;
}
```

&nbsp;&nbsp;&nbsp;&nbsp;Developer can launch multiple **awaiter** coroutines while simultaneously waiting for events from an **event list**. When data is ready and an event is triggered, the **awaiter** can resume execution sequentially.

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

&nbsp;&nbsp;&nbsp;&nbsp;Developer can nest **awaiter** coroutines within each other. This nesting allows developers to encapsulate different libraries and share them with others without additional wrapping of the **awaiter** coroutines.

```cpp
awaiter co_sleep(int t)
{
  co_await sleep_for(t);
  co_return 0；
}
awaiter co_iterator(int t)
{
 if (t > 0)
 {
  co_await co_iterator(t - 1);
  co_await sleep_for(1);
 }
 co_return 0;
}
```

&nbsp;&nbsp;&nbsp;&nbsp;Regarding **evlts**, it represents a thread-safe event list. You can also use the `wait_for` function to wait for events, with the variables in the wait_for call protected by std::mutex.

```cpp
async<evlts> g_trigger;
std::list<int> g_data;
awaiter co_channel_recv() {
  co_await wait_for<0>(&g_trigger,	    
    []() {//If there is no data in g_data, it needs to suspend. If there is data in g_data, it can immediately return. 
      return g_data.empty();
    },
		[]() {
      std::cout<< g_data.front();
			g_data.pop_front();// get data from g_data.
		});
  co_return 0;
}
awaiter co_channel_send(){  
  resume<0>(&g_trigger,[]() { //Put data into g_data and resume back to co_channel_recv.
    g_data.emplace_back(1);
  });
}
```

# task 

&nbsp;&nbsp;&nbsp;&nbsp;**task** is used when simultaneously waiting for multiple **awaiter** coroutines. It is a list that stores **taskevent** instances and hold multiple **awaiter** coroutines.
&nbsp;&nbsp;&nbsp;&nbsp;`wait_for_all`function waits for multiple **awaiter** coroutines to complete. When all the awaited coroutines finish, `wait_for_all` returns.

```cpp
template <class... T>
awaiter wait_for_all(T &&..._task)
{
  task w;
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

&nbsp;&nbsp;&nbsp;&nbsp;`wait_for_any` function waits for any of the **awaiter** coroutines to complete. When one of the **awaiter** coroutines finishes, `wait_for_any` returns.

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

&nbsp;&nbsp;&nbsp;&nbsp;All awaiter coroutines are event-driven. When waiting for an event, save the event in async. When the event completes, resume the event and jump to the waiting awaiter. When the awaiter completes, it can return to the waiting caller’s awaiter. Developers only need to understand awaiter, event, async, and the wait_for function. There is no need to understand concepts like promise_type or coroutine_handle , nor is there a need to encapsulate coroutines based on different business logic.

&nbsp;&nbsp;&nbsp;&nbsp;The solution is highly abstract and can be used to transform asynchronous programs into coroutines. Whether it’s file I/O, networking, pipelines, or business code, all asynchronous processes can be converted into coroutines. This significantly reduces development complexity and aligns with the thinking habits of most developers.

&nbsp;&nbsp;&nbsp;&nbsp;The author has provided a complete implementation, including examples with HTTP, MySQL client, and Hiredis. You can find the source code at <https://github.com/dou1984/coev/tree/main/src/coev>.