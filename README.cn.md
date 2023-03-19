# coev
c++20 coroutine libev

---

coev 是基于libev的c++20协程库。2019年时c++委员会就提出了协程草案，github上也有很多协程的实现方案，实现方案对于开发者并不友好，作者封装了一个较为简单的库，方便c++开发者使用。

c++20的协程是无栈协程，相较于传统的有栈协程，大大提升了协程的切换效率。相对于经典的boost::context, boost::context,  c++20协程对于开发模式改变较大。

c++20的协程开发具有难度，因此coev封装了3种常用的Awaiter，降低了理解c++20协程的难度，提升开发效率，coev也能快速将异步过程转为协程。

---

Awaiter是coev的协程类，Awaiter使用起来很方便，把Awaiter定义为函数返回既可以创建一个协程，同时Awaiter可以定义返回值类型。

``
Awaiter<int> co_sleep()
{
	co_await sleep_for(1);
	co_return 0；
}
``

Awaiter可以用分级调用，这解决了coroutine中最常用的多级调用问题。

``
Awaiter<int> test_lower()
{
	co_await co_sleep(1);
}
Awaiter<int> test_upper()
{
	co_await  test_lower();
}
``

---

Task 用于等待协程完成, Task可以选择两种模式，一种是等所有task完成再退出，一种是只要一个task完成就退出。

``
Awaiter<int> test_any()
{
	co_await wait_for_all(co_sleep(1), co_sleep(2));
}
Awaiter<int> test_all()
{
	co_await wait_for_any(co_sleep(1), co_sleep(2));
}
``

---

channel用于数据传输。

``

Channel<int> ch;
Awaiter<int> co_channel_input()
{
	int x = 1;
	co_await ch.set(x); 
	co_return 0;
}
Awaiter<int> co_channel_output()
{
	int x = 0;
	co_await ch.get(x);
	co_return  0;
}
``






