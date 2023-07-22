# coev

c++20 coroutine libev

---

coev 是基于libev的c++20协程库。2019年时c++委员会就提出了协程草案，github上也有很多协程的实现方案，实现方案对于开发者并不友好，作者封装了一个较为简单的库，方便c++开发者使用。

c++20的协程是无栈协程，相较于传统的有栈协程，大大提升了协程的切换效率。相对于经典的boost::context, boost::context, c++20协程对于开发模式改变较大。

c++20的协程开发具有难度，因此coev封装了3种常用的Awaiter，降低了理解c++20协程的难度，提升开发效率，coev也能快速将异步过程转为协程。

## event

event 是最小的协程类，用于快速将异步调用转换成协程。与此匹配的是EventChain，wait_for<eventchain>，相互配合可以快速实现协程。

```cpp
using EVRecv = eventchain<RECV>;//起个新的名字
struct Trigger :  EVRecv
{
} g_trigger;

awaiter co_waiting()
{ 
 co_await wait_for<EVRecv>(g_trigger);
 co_return 0;
}
awaiter co_trigger()
{
 co_await sleep_for(5);
 g_trigger.EVRecv::resume();
 co_return 0;
}
```

## awaiter

Awaiter是coev的协程类，Awaiter使用起来很方便，把Awaiter定义为函数返回既可以创建一个协程，同时Awaiter可以定义返回值类型。

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

awaiter可以用分级调用，这解决了coroutine中最常用的多级调用问题。

```cpp
awaiter test_lower()
{
  co_await co_sleep(1);
}
awaiter test_upper()
{
 co_await test_lower();
}
```

awaiter 用于等待协程完成, awaiter可以选择两种模式，一种是等所有task完成再退出，一种是只要一个task完成就退出。

```cpp
awaiter test_any()
{
 co_await wait_for_any(sleep_for(1), sleep_for(2));
}
awaiter test_all()
{
  co_await wait_for_all(sleep_for(1), sleep_for(2));
}
```

## channel

channel用于数据传输。

```cpp
channel<int> ch;
awaiter co_channel_input()
{
  int x = 1;
 co_await ch.set(x); 
 co_return 0;
}
awaiter co_channel_output()
{
 int x = 0;
 co_await ch.get(x);
 co_return  0;
}
```

## mysql

coev 可以查询mysql数据库。

```cpp
awaiter test_mysql()
{
 Mysqlcli c("127.0.0.1", 3306, "root", "12345678", "test");
 auto r = co_await c.connect();
 if (r == -1)
 {
  co_return 0;
 }
 auto s = "select * from t_test limit 1;"
 auto r = co_await c.query(s.c_str(), s.size(), [](auto,auto) {});
 if (r == -1)
 {
  co_return  0;
 }
 co_return 0;
}
```

## redis

coev 可以查询redis。

```cpp
awaiter test_redis()
{
 Rediscli c("127.0.0.1", 6379, "");

 co_await c.connect();
 co_await c.query("ping hello",
  [](auto &r)
  {
   LOG_DBG("%s\n", r.last_msg);
  });

 co_return 0;
}
```
