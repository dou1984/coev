#include "awaiter.h"

namespace coev
{
	awaiter::promise_type::~promise_type()
	{
		if (_this != nullptr)
		{
			_this->m_ready = true;
			_this->resume();
		}
	}
	awaiter awaiter::promise_type::get_return_object()
	{
		return {std::coroutine_handle<promise_type>::from_promise(*this)};
	}
	std::suspend_never awaiter::promise_type::return_value(int v)
	{
		value = v;
		return {};
	}
	std::suspend_never awaiter::promise_type::yield_value(int v)
	{
		value = v;
		return {};
	}
	awaiter::awaiter(std::coroutine_handle<promise_type> h) : m_coroutine(h)
	{
		m_coroutine.promise()._this = this;
	}
	awaiter::~awaiter()
	{
		if (m_coroutine != nullptr)
			m_coroutine.promise()._this = nullptr;
	}
	void awaiter::resume()
	{
		if (m_awaiting && !m_awaiting.done())
			m_awaiting.resume();
		taskext::__resume();
	}
	void awaiter::destroy()
	{
		if (m_coroutine)
		{
			m_coroutine.promise()._this = nullptr;
			m_coroutine.destroy();
			m_coroutine = nullptr;
			m_awaiting = nullptr;
		}
	}
}