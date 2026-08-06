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

#include <stdexcept>
#include <string>

#include <tdzdd/dd/DataTable.hpp>
#include <tdzdd/util/MemoryPool.hpp>
#include <tdzdd/util/MyVector.hpp>

using namespace tdzdd;

namespace {

struct ThrowOnCopy {
    static int copiesUntilThrow;  ///< -1 means "never throw".
    static int liveCount;
    int value;

    explicit ThrowOnCopy(int v = 0)
            : value(v) {
        ++liveCount;
    }

    ThrowOnCopy(ThrowOnCopy const& o)
            : value(o.value) {
        if (copiesUntilThrow == 0) throw std::runtime_error("copy failed");
        if (copiesUntilThrow > 0) --copiesUntilThrow;
        ++liveCount;
    }

    ~ThrowOnCopy() {
        --liveCount;
    }
};

int ThrowOnCopy::copiesUntilThrow = -1;
int ThrowOnCopy::liveCount = 0;

} // namespace

TEST(MyVectorTest, SelfAssignmentKeepsElements) {
    MyVector<int> v;
    v.push_back(10);
    v.push_back(20);

    v = v;

    ASSERT_EQ(2U, v.size());
    EXPECT_EQ(10, v[0]);
    EXPECT_EQ(20, v[1]);
}

TEST(MyVectorTest, PushBackOwnElementWhileReallocating) {
    MyVector<int> v;
    v.push_back(0);
    while (v.size() < v.capacity()) {
        v.push_back(int(v.size()));
    }
    size_t const n = v.size();
    ASSERT_EQ(v.capacity(), n);

    v.push_back(v[0]);  // reallocation happens with a reference into the array

    ASSERT_EQ(n + 1, v.size());
    for (size_t i = 0; i < n; ++i) {
        EXPECT_EQ(int(i), v[i]);
    }
    EXPECT_EQ(0, v[n]);
}

TEST(MyVectorTest, PushBackOwnElementOfNonPodType) {
    MyVector<std::string> v;
    v.push_back("a long enough string to be allocated on the heap");
    while (v.size() < v.capacity()) {
        v.push_back("another long enough string to be allocated on the heap");
    }
    size_t const n = v.size();
    ASSERT_EQ(v.capacity(), n);
    std::string const expected = v[0];

    v.push_back(v[0]);

    ASSERT_EQ(n + 1, v.size());
    EXPECT_EQ(expected, v[0]);
    EXPECT_EQ(expected, v[n]);
}

TEST(MyVectorTest, CopyAssignmentIsExceptionSafe) {
    ThrowOnCopy::copiesUntilThrow = -1;
    ThrowOnCopy::liveCount = 0;
    {
        MyVector<ThrowOnCopy> a;
        for (int i = 0; i < 4; ++i) {
            a.push_back(ThrowOnCopy(i));
        }

        MyVector<ThrowOnCopy> b;
        ThrowOnCopy::copiesUntilThrow = 2;
        EXPECT_THROW(b = a, std::runtime_error);
        ThrowOnCopy::copiesUntilThrow = -1;

        // b must only hold the elements that were actually constructed.
        ASSERT_EQ(2U, b.size());
        EXPECT_EQ(0, b[0].value);
        EXPECT_EQ(1, b[1].value);
    }
    EXPECT_EQ(0, ThrowOnCopy::liveCount);
}

TEST(MyVectorTest, CopyConstructorReleasesElementsWhenCopyThrows) {
    ThrowOnCopy::copiesUntilThrow = -1;
    ThrowOnCopy::liveCount = 0;
    {
        MyVector<ThrowOnCopy> a;
        for (int i = 0; i < 4; ++i) {
            a.push_back(ThrowOnCopy(i));
        }

        int const liveBefore = ThrowOnCopy::liveCount;
        ThrowOnCopy::copiesUntilThrow = 2;
        EXPECT_THROW(MyVector<ThrowOnCopy> b(a), std::runtime_error);
        ThrowOnCopy::copiesUntilThrow = -1;
        EXPECT_EQ(liveBefore, ThrowOnCopy::liveCount);
    }
    EXPECT_EQ(0, ThrowOnCopy::liveCount);
}

TEST(DataTableTest, SelfAssignmentKeepsRows) {
    DataTable<int> table(2);
    table[0].push_back(10);
    table[1].push_back(20);
    table[1].push_back(30);

    table = table;

    ASSERT_EQ(2, table.numRows());
    ASSERT_EQ(1U, table[0].size());
    ASSERT_EQ(2U, table[1].size());
    EXPECT_EQ(10, table[0][0]);
    EXPECT_EQ(20, table[1][0]);
    EXPECT_EQ(30, table[1][1]);
}

TEST(MemoryPoolTest, MoveFromClearsNonEmptyDestination) {
    MemoryPool dst;
    int* oldValue = dst.allocate<int>();
    *oldValue = 10;

    MemoryPool src;
    int* srcValue = src.allocate<int>();
    *srcValue = 20;

    dst.moveFrom(src);

    EXPECT_TRUE(src.empty());
    EXPECT_FALSE(dst.empty());
    int* newValue = dst.allocate<int>();
    *newValue = 30;
    EXPECT_EQ(30, *newValue);
}

TEST(MemoryPoolTest, MemoryPoolsResizeMovesPools) {
    MemoryPools pools;
    pools.resize(1);
    int* firstValue = pools[0].allocate<int>();
    *firstValue = 10;

    pools.resize(4);

    EXPECT_FALSE(pools[0].empty());
    int* secondValue = pools[0].allocate<int>();
    *secondValue = 20;
    EXPECT_EQ(20, *secondValue);
}

TEST(MemoryPoolTest, CopyAssignEmptySourceClearsDestination) {
    MemoryPool dst;
    int* oldValue = dst.allocate<int>();
    *oldValue = 10;

    MemoryPool empty;
    dst = empty;

    EXPECT_TRUE(dst.empty());
    EXPECT_TRUE(empty.empty());
}

TEST(MemoryPoolTest, CopyAssignNonEmptySourceThrows) {
    MemoryPool src;
    int* srcValue = src.allocate<int>();
    *srcValue = 10;

    MemoryPool dst;
    int* dstValue = dst.allocate<int>();
    *dstValue = 20;

    EXPECT_THROW(dst = src, std::runtime_error);
    EXPECT_FALSE(src.empty());
    EXPECT_FALSE(dst.empty());
}
