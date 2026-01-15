#include "sliding_buffer.hpp"
#include <gtest/gtest.h>
#include <gmock/gmock.h>

using utils::sliding_buffer;

/* ------------------------------------------------------------
 * Construction
 * ------------------------------------------------------------ */

TEST(SlidingBuffer, ConstructWithSize)
{
    sliding_buffer<int> cb(5);
    EXPECT_EQ(cb.size(), 5u);
}

/* ------------------------------------------------------------
 * operator[] bounds checking
 * ------------------------------------------------------------ */

TEST(SlidingBuffer, IndexOutOfBounds)
{
    sliding_buffer<int> cb(3);

    auto r = cb[3];
    EXPECT_FALSE(r.has_value());
    EXPECT_NE(r.error().find("exceeds size"), std::string::npos);
}

/* ------------------------------------------------------------
 * Single element push
 * ------------------------------------------------------------ */

TEST(SlidingBuffer, PushSingleElement)
{
    sliding_buffer<int> cb(4);

    cb.push_back(42);

    auto v = cb[3];
    ASSERT_TRUE(v.has_value());
    EXPECT_EQ(*v, 42);
}

/* ------------------------------------------------------------
 * Range push without wrap
 * ------------------------------------------------------------ */

TEST(SlidingBuffer, PushRangeNoWrap)
{
    constexpr int size = 5;
    sliding_buffer<int> cb(size);

    std::vector<int> src = {1, 2, 3};
    cb.push_back(src.begin(), src.end());

    EXPECT_EQ(*cb[size - 3], 1);
    EXPECT_EQ(*cb[size - 2], 2);
    EXPECT_EQ(*cb[size - 1], 3);
}

/* ------------------------------------------------------------
 * Range push exact fit
 * ------------------------------------------------------------ */

TEST(SlidingBuffer, PushRangeExactFit)
{
    sliding_buffer<int> cb(4);

    std::vector<int> src = {10, 20, 30, 40};
    cb.push_back(src.begin(), src.end());

    EXPECT_EQ(*cb[0], 10);
    EXPECT_EQ(*cb[1], 20);
    EXPECT_EQ(*cb[2], 30);
    EXPECT_EQ(*cb[3], 40);
}

/* ------------------------------------------------------------
 * Range push with wrap-around
 * ------------------------------------------------------------ */

TEST(SlidingBuffer, PushRangeWithWrap)
{
    sliding_buffer<int> cb(5);

    std::vector<int> first = {1, 2, 3, 4};
    cb.push_back(first.begin(), first.end());

    // cur_ == 4 now
    std::vector<int> second = {5, 6, 7};
    cb.push_back(second.begin(), second.end());

    // Logical layout relative to cur_
    EXPECT_EQ(*cb[0], 3);
    EXPECT_EQ(*cb[1], 4);
    EXPECT_EQ(*cb[2], 5);
    EXPECT_EQ(*cb[3], 6);
    EXPECT_EQ(*cb[4], 7);
}

/* ------------------------------------------------------------
 * Overwrite behavior (buffer saturation)
 * ------------------------------------------------------------ */

TEST(SlidingBuffer, OverwriteOldestData)
{
    sliding_buffer<int> cb(3);

    std::vector<int> src = {1, 2, 3, 4, 5};
    cb.push_back(src.begin(), src.end());

    EXPECT_EQ(*cb[0], 3);
    EXPECT_EQ(*cb[1], 4);
    EXPECT_EQ(*cb[2], 5);
}

/* ------------------------------------------------------------
 * Multiple small pushes
 * ------------------------------------------------------------ */

TEST(SlidingBuffer, MultipleSmallPushes)
{
    sliding_buffer<int> cb(4);

    cb.push_back(1);
    cb.push_back(2);
    cb.push_back(3);
    cb.push_back(4);
    cb.push_back(5);

    EXPECT_EQ(*cb[0], 2);
    EXPECT_EQ(*cb[1], 3);
    EXPECT_EQ(*cb[2], 4);
    EXPECT_EQ(*cb[3], 5);
}

/* ------------------------------------------------------------
 * operator[] always relative to current head
 * ------------------------------------------------------------ */

TEST(SlidingBuffer, IndexIsRelativeToCurrentHead)
{
    sliding_buffer<int> cb(3);

    cb.push_back(10);
    cb.push_back(20);
    cb.push_back(30);
    cb.push_back(40);

    EXPECT_EQ(*cb[0], 20);
    EXPECT_EQ(*cb[1], 30);
    EXPECT_EQ(*cb[2], 40);
}
