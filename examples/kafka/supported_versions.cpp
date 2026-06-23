/*
 *	coev - c++20 coroutine library
 *
 *	Copyright (c) 2023-2026, Zhao Yun Shan
 *
 */
#include "supported_versions.h"
using namespace coev::kafka;

const std::vector<KafkaVersion>& SupportedVersions()
{
    static const std::vector<KafkaVersion> versions = {
        V1_0_0_0,
        V1_0_1_0,
        V1_0_2_0,
        V1_1_0_0,
        V1_1_1_0,
        V2_0_0_0,
        V2_0_1_0,
        V2_1_0_0,
        V2_1_1_0,
        V2_2_0_0,
        V2_2_1_0,
        V2_2_2_0,
        V2_3_0_0,
        V2_3_1_0,
        V2_4_0_0,
        V2_4_1_0,
        V2_5_0_0,
        V2_5_1_0,
        V2_6_0_0,
        V2_6_1_0,
        V2_6_2_0,
        V2_6_3_0,
        V2_7_0_0,
        V2_7_1_0,
        V2_7_2_0,
        V2_8_0_0,
        V2_8_1_0,
        V2_8_2_0,
        V3_0_0_0,
        V3_0_1_0,
        V3_0_2_0,
        V3_1_0_0,
        V3_1_1_0,
        V3_1_2_0,
        V3_2_0_0,
        V3_2_1_0,
        V3_2_2_0,
        V3_2_3_0,
        V3_3_0_0,
        V3_3_1_0,
        V3_3_2_0,
        V3_4_0_0,
        V3_4_1_0,
        V3_5_0_0,
        V3_5_1_0,
        V3_5_2_0,
        V3_6_0_0,
        V3_6_1_0,
        V3_6_2_0,
        V3_7_0_0,
        V3_7_1_0,
        V3_7_2_0,
        V3_8_0_0,
        V3_8_1_0,
        V3_9_0_0,
        V3_9_1_0,
        V4_0_0_0,
        V4_1_0_0,
    };
    return versions;
}
