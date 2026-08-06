/*
 * TdZdd: a Top-down/Breadth-first Decision Diagram Manipulation Framework
 * by Hiroaki Iwashita <iwashita@erato.ist.hokudai.ac.jp>
 * Copyright (c) 2014 ERATO MINATO Project
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 */

#include <gtest/gtest.h>

#include <tdzdd/util/MemoryPool.hpp>
#include <tdzdd/util/MySet.hpp>

using namespace tdzdd;

TEST(MySmallSetTest, GetReturnsStoredElements) {
    MySmallSet<int,4> s;
    s.add(20);
    s.add(10);

    ASSERT_EQ(2U, s.size());
    EXPECT_EQ(10, s.get(0));  // elements are kept sorted
    EXPECT_EQ(20, s.get(1));
}

TEST(MySmallSetTest, OnPoolGetReturnsStoredElements) {
    MemoryPool pool;
    MySmallSetOnPool<int>* s = MySmallSetOnPool<int>::newInstance(pool, 4);
    s->add(30);
    s->add(10);
    s->add(20);

    ASSERT_EQ(3U, s->size());
    EXPECT_EQ(10, s->get(0));
    EXPECT_EQ(20, s->get(1));
    EXPECT_EQ(30, s->get(2));
}

#ifndef NDEBUG
// These tests exist only in the debug builds, where assert() is enabled.
TEST(MySmallSetDeathTest, GetBeyondSizeAsserts) {
    ::testing::FLAGS_gtest_death_test_style = "threadsafe";
    MySmallSet<int,4> s;
    s.add(1);

    EXPECT_EQ(1, s.get(0));
    EXPECT_DEATH(s.get(1), "");  // within capacity N but beyond size()
}

TEST(MySmallSetDeathTest, OnPoolGetBeyondSizeAsserts) {
    ::testing::FLAGS_gtest_death_test_style = "threadsafe";
    MemoryPool pool;
    MySmallSetOnPool<int>* s = MySmallSetOnPool<int>::newInstance(pool, 4);
    s->add(1);

    EXPECT_EQ(1, s->get(0));
    EXPECT_DEATH(s->get(1), "");  // N == 0: the old assert checked nothing
}
#endif
