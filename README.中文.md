# coev

c++20 coroutine libev

---

  coev 是一个基于 C++20 协程（coroutines）的高性能协程编程库。它对 C++20 协程进行了完善的封装，底层依托成熟稳定的 libev 网络库，具备出色的网络吞吐能力。
  
  coev 的设计初衷是简化 C++20 协程复杂晦涩的使用方式。通过高度抽象与模块解耦，开发者可以像编写同步代码一样轻松实现异步逻辑（不同的是使用co_await等待 I/O 操作完成） ，即可获得**清晰直观的控制流**，同时完全支持函数中的**临时变量**及其传递。

  最重要的是，coev 将协程（coroutines）与网络（network）逻辑彻底解耦，使开发者能够将任意异步操作无缝转化为协程形式，并**手动或自动触发协程恢复**，从而获得极大的灵活性与控制自由度。

  在任务调度方面，coev 提供了强大的并发原语:
1. 支持 wait_for_all（等待所有任务完成）
2. 支持 wait_for_any（任一任务完成即返回）
3. 可任意组合嵌套使用，灵活构建复杂的并发逻辑
   
  所有任务在完成后会自动释放资源，无需手动管理生命周期。

  此外，coev 内置了类型安全、线程安全的 Channel 机制，可用于在协程或线程间高效传递任意类型的数据，极大提升了多协程协作的表达能力。
  为提升开发效率，coev 已对常用组件进行了协程化封装，包括：
1. TCP / UDP
2. MySQL 客户端（基于 mysqlclient）
3. Redis 客户端（基于 hiredis）
4. HTTP / HTTP/2（基于http_parser/libnghttp2）
   

# 示例

调用co_await g_trigger.suppend()等待一个指令回复，调用g_trigger.resume()恢复到等待的协程。临时变量可通过参数传递，并且整个过程是线程安全的，调用就是如此的简单，轻松实现复杂的控制流。
```cpp
using namespace coev;
async g_trigger;
awaitable<int> co_waiting()
{ 
 co_await g_trigger.suppend(); // 等待事件触发
 co_return 0;
}
awaitable<int> co_trigger()
{
 co_await sleep_for(5);
 g_trigger.resume();  // 触发事件，跳转到协程co_waiting
 co_return 0;
}
awaitable<int> test_any()
{
 co_await wait_for_any(sleep_for(1), sleep_for(2)); // 等待任意协程返回结果。
}
awaitable<int> test_all()
{
  co_await wait_for_all(sleep_for(1), sleep_for(2)); // 等待所有协程返回结果。
}
```

## Channel

co_channel用于数据传输，这也是协程中比较重要的用法，co_channel也是线程安全的。

```cpp
using namespace coev;
co_channel<int> ch;
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

# 如何安装

编译器需要支持c++20标准及以上。

```sh
#ubuntu
make build
cd build
cmake ..
make 
```
