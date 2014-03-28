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

TEST(RandomDdTest, BDD_and_ZDD) {
    int n = 100;
    int w = 1000;
    double d = 0.3;

    for (int i = 0; i < 10; ++i) {
        DdStructure<2> dd(RandomDd<2>(n, w, d), useMP);

        DdStructure<2> qdd = dd;
        DdStructure<2> bdd = dd;
        DdStructure<2> zdd = dd;
        qdd.qddReduce();
        bdd.bddReduce();
        zdd.zddReduce();
        ASSERT_LE(qdd.size(), dd.size());
        ASSERT_LE(bdd.size(), qdd.size());
        ASSERT_LE(zdd.size(), qdd.size());
        ASSERT_EQ(qdd.evaluate(BddCardinality<>(n)), bdd.evaluate(BddCardinality<>(n)));
        ASSERT_EQ(qdd.evaluate(ZddCardinality<>()), zdd.evaluate(ZddCardinality<>()));

        DdStructure<2> bqd = qdd;
        DdStructure<2> zqd = qdd;
        bqd.bddReduce();
        zqd.zddReduce();
        ASSERT_EQ(bdd, bqd);
        ASSERT_EQ(zdd, zqd);

        DdStructure<2> zbd = bdd.bdd2zdd(n);
        if (bdd != zdd) ASSERT_NE(bdd, zbd);
        ASSERT_EQ(zdd, zbd);

        DdStructure<2> bzd = zdd.zdd2bdd(n);
        if (bdd != zdd) ASSERT_NE(zdd, bzd);
        ASSERT_EQ(bdd, bzd);
    }
}
