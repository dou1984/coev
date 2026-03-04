/*
 *	coev - c++20 coroutine library
 *
 *	Copyright (c) 2023-2026, Zhao Yun Shan
 *
 */
#pragma once

enum IncrementalAlterConfigsOperation : int8_t
{
    IncrementalAlterConfigsOperationSet = 0,
    IncrementalAlterConfigsOperationDelete = 1,
    IncrementalAlterConfigsOperationAppend = 2,
    IncrementalAlterConfigsOperationSubtract = 3,
};
