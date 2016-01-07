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

#include <tdzdd/DdStructure.hpp>

#include "RandomDd.hpp"

using namespace tdzdd;

extern bool useMP;

template<int A>
void do_test(int n, int w, double d) {
    DdStructure<A> dd(RandomDd<A>(n, w, d), useMP);

    DdStructure<A> qdd = dd;
    DdStructure<A> bdd = dd;
    DdStructure<A> zdd = dd;
    qdd.qddReduce();
    bdd.bddReduce();
    zdd.zddReduce();
    ASSERT_LE(qdd.size(), dd.size());
    ASSERT_LE(bdd.size(), qdd.size());
    ASSERT_LE(zdd.size(), qdd.size());
    ASSERT_EQ(qdd.bddCardinality(n), bdd.bddCardinality(n));
    ASSERT_EQ(qdd.zddCardinality(), zdd.zddCardinality());

    DdStructure<A> bqd = qdd;
    DdStructure<A> zqd = qdd;
    bqd.bddReduce();
    zqd.zddReduce();
    ASSERT_EQ(bdd, bqd);
    ASSERT_EQ(zdd, zqd);

    zqd.zddSubset(qdd);
    ASSERT_EQ(zdd, zqd);

    DdStructure<A> zbd = bdd.bdd2zdd(n);
    if (bdd != zdd) ASSERT_NE(bdd, zbd);
    ASSERT_EQ(zdd, zbd);

    DdStructure<A> bzd = zdd.zdd2bdd(n);
    if (bdd != zdd) ASSERT_NE(zdd, bzd);
    ASSERT_EQ(bdd, bzd);
}

TEST(RandomDdTest, Binary) {
    for (int i = 0; i < 100; ++i) {
        do_test<2>(100, 1000, 0.3);
    }
}

TEST(RandomDdTest, Ternary) {
    for (int i = 0; i < 10; ++i) {
        do_test<3>(100, 1000, 0.3);
    }
}

TEST(RandomDdTest, Quaternary) {
    for (int i = 0; i < 10; ++i) {
        do_test<4>(100, 1000, 0.3);
    }
}

TEST(RandomDdTest, NinetyNineAry) {
    for (int i = 0; i < 10; ++i) {
        do_test<99>(10, 1000, 0.3);
    }
}
