/*
 *	coev - c++20 coroutine library
 *
 *	Copyright (c) 2023-2026, Zhao Yun Shan
 *
 */
#pragma once

namespace coev::kafka
{
    enum ControlRecordType
    {
        ControlRecordAbort = 0,
        ControlRecordCommit = 1,
        ControlRecordUnknown = 2
    };
}