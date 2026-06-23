/*
 *	coev - c++20 coroutine library
 *
 *	Copyright (c) 2023-2026, Zhao Yun Shan
 *
 */
#pragma once
#include <coev_kafka/version.h>
#include <vector>

const std::vector<coev::kafka::KafkaVersion>& SupportedVersions();
