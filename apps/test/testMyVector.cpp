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

#include <tdzdd/dd/DataTable.hpp>
#include <tdzdd/util/MemoryPool.hpp>
#include <tdzdd/util/MyVector.hpp>

using namespace tdzdd;

TEST(MyVectorTest, SelfAssignmentKeepsElements) {
    MyVector<int> v;
    v.push_back(10);
    v.push_back(20);

    v = v;

    ASSERT_EQ(2U, v.size());
    EXPECT_EQ(10, v[0]);
    EXPECT_EQ(20, v[1]);
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
