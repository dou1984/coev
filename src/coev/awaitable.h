/*
 *	coev - c++20 coroutine library
 *
 *	Copyright (c) 2023, Zhao Yun Shan
 *	All rights reserved.
 *
 */
#pragma once
#include <coroutine>
#include <atomic>
#include <memory>
#include "chain.h"
#include "promise.h"
#include "log.h"
#include "tasknotify.h"

namespace coev
{
	template <class T, class... R>
	class awaitable final : public tasknotify
	{
	public:
		struct promise_value
		{
			T value;
			std::suspend_never return_value(T &&v)
			{
				value = std::forward<T>(v);
				return {};
			}		
			std::suspend_never return_value(const T &v)
			{
				value = v;
				return {};
			}
			std::suspend_always yield_value(T &&v)= delete;
			std::suspend_always yield_value(const T &v) =delete;
			
		};
		struct promise_void
		{
			std::suspend_never return_void()
			{
				return {};
			}
			std::suspend_always yield_void() =delete;
		
		};
		struct promise_tuple
		{
			std::tuple<T, R...> value;
			template <class... ARGS>
			std::suspend_never return_value(ARGS &&...args)
			{
				__set<0>(std::forward<ARGS>(args)...);
				return {};
			}		
			template <class... ARGS>
			std::suspend_never return_value(const ARGS &...args)
			{
				value = std::make_tuple(args...);
				return {};
			}
			template <class... ARGS>
			std::suspend_never yield_value(const ARGS &...args) = delete;		
			template <class... ARGS>
			std::suspend_never yield_value(ARGS &&...args) = delete;

			template <size_t I, class... ARGS>
			void __set(ARGS &&...args);
			template <size_t I, class ARGS>
			void __set(ARGS &&args)
			{
				std::get<I>(value) = std::forward<ARGS>(args);
			}
			template <size_t I, class ARGS, class... RES>
			void __set(ARGS &&args, RES &&...res)
			{
				std::get<I>(value) = std::forward<ARGS>(args);
				__set<I + 1>(std::forward<RES>(res)...);
			}
		};
		struct promise_type : promise, std::conditional_t<(sizeof...(R) > 0), promise_tuple, std::conditional_t<std::is_void_v<T>, promise_void, promise_value>>
		{
			awaitable *m_awaitable = nullptr;
			promise_type() = default;
			~promise_type()
			{
				if (m_awaitable)
				{
					m_awaitable->m_state = STATUS_RESUMED;
					auto _awaitable = m_awaitable;
					m_awaitable = nullptr;
					_awaitable->resume();
				}
			}
			awaitable<T, R...> get_return_object()
			{
				return {std::coroutine_handle<promise_type>::from_promise(*this)};
			}
		};

	public:
		awaitable() = default;
		awaitable(std::coroutine_handle<promise_type> h) : m_callee(h)
		{
			m_callee.promise().m_awaitable = this;
	
		}
		awaitable(awaitable &&o) = delete;
		awaitable(const awaitable &) = delete;
		const awaitable &operator=(awaitable &&) = delete;
		const awaitable &operator=(const awaitable &&) = delete;
		~awaitable()
		{
			LOG_CORE("destructor %p\n", this);
			if (m_callee.address())
				m_callee.promise().m_awaitable = nullptr;
		}
		bool done() { return m_callee ? m_callee.done() : true; }
		auto await_resume()
		{
			if constexpr (sizeof...(R) > 0)
			{
				return m_callee ? std::move(m_callee.promise().value) : decltype(m_callee.promise().value){};
			}
			else if constexpr (!std::is_void_v<T>)
			{
				return m_callee ? std::move(m_callee.promise().value) : decltype(m_callee.promise().value){};
			}
			else
			{
			}
		}
		void destroy()
		{
			if (m_callee.address() && m_callee.promise().m_awaitable)
			{
				m_callee.promise().m_awaitable = nullptr;
				m_callee.destroy();
			}
			m_callee = nullptr;
			m_caller = nullptr;
		}
		void await_suspend(std::coroutine_handle<> caller)
		{
			m_caller = caller;
			m_state = STATUS_SUSPEND;
		}
		bool await_ready()
		{
			LOG_CORE("await_ready %p %s\n", this, m_state==STATUS_RESUMED ? "READY":"WAIT");
			return m_state == STATUS_RESUMED;
		}
		void resume()
		{
			LOG_CORE("resume %p\n", this);
			m_state = STATUS_RESUMED;
			if (m_caller.address() && !m_caller.done())
				m_caller.resume();
			tasknotify::notify();
		}

	private:
		std::coroutine_handle<promise_type> m_callee = nullptr;
		std::coroutine_handle<> m_caller = nullptr;
		std::atomic_int m_state{STATUS_INIT};
	};
}