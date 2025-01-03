# coev

c++20 coroutine libev

---

coev 是高性能的c++20协程库, coev封装了3个c++20协程类awaiter、event、async, 这3个类大大降低了c++20协程的开发难度，提升开发效率，coev目的是快速将异步程序转为协程。

# install

```sh
#ubuntu
make build
cd build
cmake ..
make 
```

## event

event 是最小的协程类，用于快速将异步调用转换成协程。与此匹配的是eventchain，wait_for<eventchain>，相互配合可以快速实现协程。

```cpp
async g_triger;
awaitable co_waiting()
{ 
 co_await wait_for(g_trigger); // 等待事件触发
 co_return 0;
}
awaitable co_trigger()
{
 co_await sleep_for(5);
 resume(&g_trigger);  // 触发事件，跳转协程到co_waiting
 co_return 0;
}
```

## awaitable

awaiter是coev的协程类，awaiter使用起来很方便，把awaiter定义为函数返回既可以创建一个协程。

awaiter可以用分级调用，这解决了coroutine中最常用的多级调用问题。

```cpp
awaitable<int> test_lower()
{
  co_await co_sleep(1);
}
awaitable<int> test_upper()
{
 co_await test_lower();
}
```

awaitable 协程嵌套

```cpp
awaitable<int> co_sleep(int t)
{
  co_await sleep_for(t);
  co_return 0；
}
awaitable<int> co_iterator(int t)
{
 if (t > 0)
 {
  co_await co_iterator(t - 1);
  co_await sleep_for(1);
 }
 co_return 0;
}
```

awaitable 也可用于多个协程的等待, 一种是wait_for_all 等待所有awaiter完成，一种是wait_for_any等待任意awaiter完成 。

```cpp
awaitable<int> test_any()
{
 co_await wait_for_any(sleep_for(1), sleep_for(2));
}
awaitable<int> test_all()
{
  co_await wait_for_all(sleep_for(1), sleep_for(2));
}
```

## channel

channel用于数据传输。

```cpp
channel<int> ch;
awaitable<int> co_channel_input()
{
  int x = 1;
 co_await ch.set(x); 
 co_return 0;
}
awaitable<int> co_channel_output()
{
 int x = 0;
 co_await ch.get(x);
 co_return  0;
}
```

更多信息可以查看
https://github.com/dou1984/coev/wiki/High-performance-event%E2%80%90based-stackless-coroutine