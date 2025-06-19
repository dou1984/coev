#include <gtest/gtest.h>

#include "../serializer/serializer.h"
#include "../serializer/parser.h"

TEST(test_serializer, append_int)
{
    coev::details::serializer s;
    int32_t val = 1;
    auto r = 0;
    r += s.append(val);
    std::string world = "world";
    r += s.append(world);
    int64_t i64 = 20;
    r += s.append(i64);

    auto a = s.move();
    EXPECT_EQ(a.size(), r);

    coev::details::parser p(a.data(), a.size());
    int32_t val2;
    p.parse(val2);
    EXPECT_EQ(val, val2);
    std::string world2;
    p.parse(world2);
    EXPECT_EQ(world, world2);
    int64_t i64_2;
    p.parse(i64_2);
    EXPECT_EQ(i64, i64_2);
    EXPECT_EQ(p.size(), 0);
}

TEST(test_serializer, append_varint)
{
    coev::details::serializer s;
    auto r = 0;
    uint32_t a0 = 1;
    r += s.append_varint(a0);
    int32_t a1 = -20;
    r += s.append_varint(a1);

    int64_t a2 = 0x7fffffff;
    r += s.append_varint(a2);

    uint64_t a3 = 0xfffffffffffffff4;
    r += s.append_varint(a3);

    auto a = s.move();
    EXPECT_EQ(a.size(), r);
    coev::details::parser p(a.data(), a.size());

    uint32_t b0;
    p.parse_varint(b0);
    EXPECT_EQ(a0, b0);
    int32_t b1;
    p.parse_varint(b1);
    EXPECT_EQ(a1, b1);
    uint64_t b2;
    p.parse_varint(b2);
    EXPECT_EQ(a2, b2);
    uint64_t b3;
    p.parse_varint(b3);
    EXPECT_EQ(a3, b3);

    EXPECT_EQ(p.size(), 0);
}