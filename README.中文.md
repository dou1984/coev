# coev

c++20 coroutine libev

---

  coev 是一个基于 C++20 协程（coroutines）的高性能协程编程库。它对 C++20 协程进行了完善的封装，底层依托成熟稳定的 libev 网络库，具备出色的网络吞吐能力。
  
  coev 的设计初衷是简化 C++20 协程复杂晦涩的使用方式。通过高度抽象与模块解耦，开发者可以像编写同步代码一样轻松实现异步逻辑（不同的是使用co_await等待 I/O 操作完成） ，即可获得**清晰直观的控制流**，同时完全支持函数中的**临时变量**及其传递。

  最重要的是，coev 将协程（coroutines）与网络（network）逻辑彻底解耦，使开发者能够将任意异步操作无缝转化为协程形式，并**手动或自动触发协程恢复**，从而获得极大的灵活性与控制自由度。

  coev的目标是支持常用的第三方client库，现在已经支持mysql-client、hiredis、http_parser、nghttp2等库，后期还会继续支持更多的库。
  1. TCP / UDP
  2. MySQL 基于 mysqlclient
  3. Redis 基于 hiredis
  4. HTTP  基于http_parser
  5. HTTP/2 基于nghttp
  6. zookeeper
  7. kafka

  客户端支持同线程并发，互不干扰。利用协程的高效切换，替代了昂贵的线程间数据搬运，大幅提升执行效率。
  相对于Actor模型：解决了同步调用阻塞线程的问题，无需依赖多线程池。Coev 利用协程实现高并发等待，极大提高了处理效率。
  相对于Preactor模型：消除了复杂的“回调地狱”。Coev采用类同步的线性代码风格，代码整洁度与维护性远超传统异步架构。


  在任务调度方面，coev 提供了强大的并发原语:
1. 支持 wait_for_all 等待所有任务完成，相当于c++26里的WhenAll。
2. 支持 wait_for_any 任一任务完成即返回，也可根据task_id使用不同处理方式。
3. 可任意组合嵌套使用，灵活构建复杂的并发逻辑。
   
  所有任务在完成后会自动释放资源，无需手动管理生命周期。实现早于c++26 标准，已实现Task的功能。

# 示例

调用co_await g_trigger.suppend()等待一个指令回复，调用g_trigger.resume()恢复到等待的协程。临时变量可通过参数传递，并且整个过程是线程安全的，调用就是如此的简单，轻松实现复杂的控制流。
```cpp
using namespace coev;
guard::async g_trigger;
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
guard::co_channel<int> ch;
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

# 联系作者
QQ：710189694
