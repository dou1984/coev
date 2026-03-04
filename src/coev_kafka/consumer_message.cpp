/*
 *	coev - c++20 coroutine library
 *
 *	Copyright (c) 2023-2026, Zhao Yun Shan
 *
 */
#include "consumer_message.h"

const std::string &ConsumerMessage::key()
{
    return m_key;
}
const std::string &ConsumerMessage::value()
{
    return m_value;
}
