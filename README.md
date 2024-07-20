# coev

c++20 coroutine libev

---

coev is a c++20 event-based coroutine solution. In 2019, the C++ Committee proposed a draft of coroutines. There are also many solutions of coroutines on github. but the libraries are not friendly to developers. The author encapsulates a relatively simple library for the convenience of C++ developers.

The coroutine of c++20 is a stackless coroutine, which greatly improves the switching efficiency of the coroutine compared with the traditional stackful coroutine. Compared with the classic boost::context, c++20 coroutines have changed a lot in the development mode.

The development of c++20 coroutines is difficult, so coev encapsulates three commonly used Awaiters, which reduces the difficulty of understanding c++20 coroutines and improves development efficiency. coev can also quickly convert asynchronous processes into coroutines.

# install

```sh
#ubuntu
apt install -y libhiredis-dev libmysqlclient-dev libhttp-parser-dev libev-dev
make build
cd build
cmake ..
make 

#compile examples
make build
cd build
cmake BUILD_EXAMPLES=ON ..
make
```

The author encapsulates three classes "awaiter", "event", "async" and "task" through the c++20 coroutine library. The main idea of ​​these three subpackage classes is to be event-based. Developers no longer need to association coroutine with file's I/O, networks and pipelines etc. Coroutine codes are highly abstract, and completely separated from other module codes. In the development, the only things needed is to store current context and wait for data complished. When the data is ready, we can trigger the recovery context and continue the coroutine.

## event

event is the smallest coroutine class, used to quickly convert asynchronous calls into coroutines. "eventchain" and "wait_for<eventchain>" cooperate with each other to quickly implement coroutines.

```cpp
async<evl> g_triger;

awaiter co_waiting()
{ 
 co_await wait_for<0>(&g_trigger);
 co_return 0;
}
awaiter co_trigger()
{
 co_await sleep_for(5);
 resume<0>(&g_triger);
 co_return 0;
}
```

## awaiter

awaiter is a coroutine class of coev. awaiter is very convenient to use. Defining awaiter as a function return can create a coroutine.

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

awaiter can be called hierarchically, which solves the most commonly used multi-level calling problem in coroutine.

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

awaiter is used to wait for the completion of the coroutine. awaiter can choose two modes, one is to wait for all tasks to complete before exiting, and the other is to exit as long as one task is completed.

```cpp
awaiter co_sleep(int t)
{
  co_await sleep_for(t);
  co_return 0；
}
awaiter test_any()
{
 co_await wait_for_any(co_sleep(1), co_sleep(2));
}
awaiter test_all()
{
 co_await wait_for_all(co_sleep(1), co_sleep(2));
}
```

## channel

channel is used for data transmission.

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

coev can query the mysql database.

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

coev can query the redis library.

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



