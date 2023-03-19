# coev
c++20 coroutine libev

---

coev is a c++20 coroutine library based on libev. In 2019, the C++ Committee proposed a draft of coroutines. There are also many implementations of coroutines on github. but the implementations are not friendly to developers. The author encapsulates a relatively simple library for the convenience of C++ developers.

The coroutine of c++20 is a stackless coroutine, which greatly improves the switching efficiency of the coroutine compared with the traditional stackful coroutine. Compared with the classic boost::context, c++20 coroutines have changed a lot in the development mode.

The development of c++20 coroutines is difficult, so coev encapsulates three commonly used Awaiters, which reduces the difficulty of understanding c++20 coroutines and improves development efficiency. coev can also quickly convert asynchronous processes into coroutines.

## Awaiter

Awaiter is a coroutine class of coev. Awaiter is very convenient to use. Defining Awaiter as a function return can create a coroutine, and Awaiter can define the return value type.

```
Awaiter<int> co_sleep()  
{  
	co_await sleep_for(1); 
	co_return 0；  
}  
```

Awaiter can be called hierarchically, which solves the most commonly used multi-level calling problem in coroutine.


```
Awaiter<int> test_lower()
{
	co_await co_sleep(1);
}
Awaiter<int> test_upper()
{
	co_await test_lower();
}
```


## Task


Task is used to wait for the completion of the coroutine. Task can choose two modes, one is to wait for all tasks to complete before exiting, and the other is to exit as long as one task is completed.

```
Awaiter<int> test_any()
{
	co_await wait_for_all(co_sleep(1), co_sleep(2));
}
Awaiter<int> test_all()
{
	co_await wait_for_any(co_sleep(1), co_sleep(2));
}
```

## Channel

Channel is used for data transmission.

```
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
```
