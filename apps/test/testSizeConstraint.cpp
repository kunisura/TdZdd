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

#include <climits>

#include <tdzdd/DdStructure.hpp>
#include <tdzdd/DdSpecOp.hpp>
#include "../graphillion/SizeConstraint.hpp"

using namespace tdzdd;

extern bool useMP;

/**
 * DD evaluator that finds the size of the smallest item set represented by this ZDD.
 */
struct MinNumItems: public DdEval<MinNumItems,int> {
    void evalTerminal(int& n, bool one) const {
        n = one ? 0 : INT_MAX - 1;
    }

    void evalNode(int& n, int, DdValues<int,2> const& values) const {
        n = std::min(values.get(0), values.get(1) + 1);
    }
};

/**
 * DD evaluator that finds the size of the largest item set represented by this ZDD.
 */
struct MaxNumItems: public DdEval<MaxNumItems,int> {
    void evalTerminal(int& n, bool one) const {
        n = one ? 0 : INT_MIN;
    }

    void evalNode(int& n, int, DdValues<int,2> const& values) const {
        n = std::max(values.get(0), values.get(1) + 1);
    }
};

TEST(SizeConstraintTest, BDD) {
    DdStructure<2> p(SizeConstraint(10, IntRange(0, 1)), useMP);
    DdStructure<2> q(SizeConstraint(10, IntRange(2, 10, 2)), useMP);
    DdStructure<2> r(SizeConstraint(10, IntRange(3, 10, 2)), useMP);
    ASSERT_EQ(19, p.size());
    ASSERT_EQ(54, q.size());
    ASSERT_EQ(52, r.size());

    DdStructure<2> bp = p;
    DdStructure<2> bq = q;
    DdStructure<2> br = r;
    bp.bddReduce();
    bq.bddReduce();
    br.bddReduce();
    ASSERT_EQ(18, bp.size());
    ASSERT_EQ(26, bq.size());
    ASSERT_EQ(31, br.size());
    ASSERT_EQ(11, bp.evaluate(BddCardinality<int>(10)));
    ASSERT_EQ(511, bq.evaluate(BddCardinality<int>(10)));
    ASSERT_EQ(1024 - 11 - 511,br.evaluate(BddCardinality<int>(10)));

    DdStructure<2> pqr(bddOr(bddOr(bp, bq), br), useMP);
    pqr.bddReduce();
    ASSERT_EQ(DdStructure<2>(0), pqr);
}

TEST(SizeConstraintTest, ZDD) {
    DdStructure<2> p(SizeConstraint(10, IntRange(0, 1)), useMP);
    DdStructure<2> q(SizeConstraint(10, IntRange(2, 10, 2)), useMP);
    DdStructure<2> r(SizeConstraint(10, IntRange(3, 10, 2)), useMP);
    ASSERT_EQ(19, p.size());
    ASSERT_EQ(54, q.size());
    ASSERT_EQ(52, r.size());

    DdStructure<2> zp = p;
    DdStructure<2> zq = q;
    DdStructure<2> zr = r;
    zp.zddReduce();
    zq.zddReduce();
    zr.zddReduce();
    ASSERT_EQ(10, zp.size());
    ASSERT_EQ(25, zq.size());
    ASSERT_EQ(30, zr.size());
    ASSERT_EQ(11, zp.evaluate(ZddCardinality<int>()));
    ASSERT_EQ(511, zq.evaluate(ZddCardinality<int>()));
    ASSERT_EQ(1024 - 11 - 511, zr.evaluate(ZddCardinality<int>()));

    DdStructure<2> pqr(zddUnion(zddUnion(zp, zq), zr), useMP);
    pqr.zddReduce();
    ASSERT_EQ(DdStructure<2>(10), pqr);

    ASSERT_EQ(0, zp.evaluate(MinNumItems()));
    ASSERT_EQ(1, zp.evaluate(MaxNumItems()));
    ASSERT_EQ(2, zq.evaluate(MinNumItems()));
    ASSERT_EQ(10, zq.evaluate(MaxNumItems()));
    ASSERT_EQ(3, zr.evaluate(MinNumItems()));
    ASSERT_EQ(9, zr.evaluate(MaxNumItems()));
}
