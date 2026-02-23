/*
 *	coev - c++20 coroutine library
 *
 *	Copyright (c) 2023-2025, Zhao Yun Shan
 *
 */
#pragma once
#include <string>
#include <cstddef>
#include <lz4.h>
#include <lz4frame.h>
#include <coev/coev.h>

namespace coev::lz4
{
    int Decompress(std::string &decompressed, const char *buf, size_t buf_size);
    int Compress(std::string &compressed, const char *buf, size_t buf_size);
}
