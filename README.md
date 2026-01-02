# coev

c++20 coroutine libev

---

coev is a high-performance coroutine programming library built on C++20 coroutines. It provides a comprehensive and user-friendly abstraction over C++20 coroutines, leveraging the mature and stable libev networking library as its foundation to deliver excellent network throughput.

The primary design goal of coev is to simplify the notoriously complex and obscure usage of C++20 coroutines. Through high-level abstraction and modular decoupling, developers can write asynchronous logic as intuitively as synchronous code—simply using co_await to wait for I/O operations to complete—while enjoying clear and straightforward control flow and full support for temporary variables within functions, including their seamless passing across suspension points.

Most importantly, coev completely decouples coroutines from networking logic, enabling developers to transform any asynchronous operation into a coroutine seamlessly. Coroutine resumption can be triggered either manually or automatically via events, offering exceptional flexibility and fine-grained control.

In terms of task scheduling, coev provides powerful concurrency primitives:
1. wait_for_all – waits until all tasks complete
2. wait_for_any – returns as soon as any one task completes
3. These primitives can be arbitrarily combined and nested to construct sophisticated concurrent workflows

All tasks automatically release their resources upon completion, eliminating the need for manual lifetime management.

Additionally, coev includes a type-safe and thread-safe Channel mechanism for efficiently passing data of any type between coroutines or threads, significantly enhancing expressiveness in multi-coroutine collaboration.

To further boost developer productivity, coev offers coroutine-ready wrappers for commonly used components, including:
1. TCP / UDP
2. MySQL client (based on mysqlclient)
3. Redis client (based on hiredis)
4. HTTP / HTTP/2 (based on http_parser and libnghttp2)


## Examples

Call co_await g_trigger.suspend() to wait for a command response, and invoke g_trigger.resume() to resume the suspended coroutine. Temporary variables can be passed through parameters, and the entire process is thread-safe—making it remarkably simple to implement complex control flows with ease.

```cpp
using namespace coev;
async g_trigger;
awaitable<int> co_waiting()
{ 
 co_await g_trigger.suppend(); // wait for event trigger
 co_return 0;
}
awaitable<int> co_trigger()
{
 co_await sleep_for(5);
 g_trigger.resume();  // Trigger an event to jump to the coroutine co_waiting.
 co_return 0;
}
awaitable<int> test_any()
{
 co_await wait_for_any(sleep_for(1), sleep_for(2)); // Wait for the result from any coroutine.
}
awaitable<int> test_all()
{
  co_await wait_for_all(sleep_for(1), sleep_for(2)); // Wait for results from all coroutines.
}
```

## Channel

Channels are used for data transmission, which is also one of the most important use cases in coroutines, and channels are thread-safe.

```cpp
channel<int> ch;  
awaitable<int> co_channel_input()  
{  
 int x = 1;  
 ch.set(x); 
 co_return 0;  
}  
awaitable<int> co_channel_output()  
{  
 int x = 0;  
 co_await ch.get(x);  
 co_return  0;  
}  
```

# How to install

The compiler must support the C++20 standard or higher.

```sh
#ubuntu
make build
cd build
cmake ..
make 
```

# Contact author
QQ:710189694

