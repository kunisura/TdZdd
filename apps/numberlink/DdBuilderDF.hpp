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

#pragma once

#include <cassert>

#include <tdzdd/DdStructure.hpp>
#include <tdzdd/util/MessageHandler.hpp>
#include <tdzdd/util/MyVector.hpp>

/**
 * Depth-first DD builder without memo-cache.
 */
template<typename S>
class DdBuilderDF {
    typedef S Spec;
    static int const AR = Spec::ARITY;

    tdzdd::NodeTableEntity<AR>& output;
    Spec spec;
    int const datasize;

    tdzdd::MyVector<size_t> work;

    void* state(int level) {
        return &work[datasize * level];
    }

public:
    DdBuilderDF(Spec const& s, tdzdd::NodeTableHandler<AR>& output) :
            output(output.privateEntity()), spec(s),
            datasize((spec.datasize() + sizeof(size_t) - 1) / sizeof(size_t)),
            work(datasize) {
    }

    /**
     * Builds a DD.
     * @param root the root node.
     */
    void build(tdzdd::NodeId& root) {
        int n = spec.get_root(state(0));
        work.resize(datasize * (n + 1));

        if (n <= 0) {
            output.init(1);
            root = tdzdd::NodeId(0, n != 0);
        }
        else {
            output.init(n + 1);
            root = build_(0, n);
        }

        spec.destruct(state(0));
    }

private:
    tdzdd::NodeId build_(int p, int i) {
        tdzdd::Node<AR> r;

        for (int b = 0; b < AR; ++b) {
            spec.get_copy(state(p + 1), state(p));
            int ii = spec.get_child(state(p + 1), i, b);

            if (ii <= 0) {
                r.branch[b] = (ii != 0);
            }
            else {
                r.branch[b] = build_(p + 1, ii);
            }

            spec.destruct(state(p + 1));
        }

        if (r == tdzdd::Node<AR>(0, 0)) return 0;

        output[i].push_back(r);
        return tdzdd::NodeId(i, output[i].size() - 1);
    }
};

/**
 * Depth-first DD construction.
 * @param spec ZDD spec.
 */
template<typename SPEC, int ARITY>
void buildDF(tdzdd::DdStructure<ARITY>& dd,
        tdzdd::DdSpecBase<SPEC,ARITY> const& spec) {
    tdzdd::NodeTableHandler<ARITY>& diagram = dd.getDiagram();
    SPEC const& s = spec.entity();
    tdzdd::MessageHandler mh;
    mh.begin(typenameof(s)) << " ...";
    DdBuilderDF<SPEC> builder(s, diagram);
    builder.build(dd.root());
    mh.end(dd.size());
}

/**
 * Depth-first ZDD subset builder.
 */
template<typename S>
class ZddSubsetterDF {
    typedef S Spec;
    static int const AR = Spec::ARITY;

    tdzdd::NodeTableEntity<AR> const& input;
    tdzdd::NodeTableEntity<AR>& output;
    Spec spec;
    int const datasize;

    size_t memoSize;
    tdzdd::MyVector<uint64_t> memo1;
    tdzdd::MyVector<uint64_t> memo2;
    tdzdd::MyVector<uint64_t> memo3;
    tdzdd::MyVector<uint64_t> memo4;

    tdzdd::MyVector<size_t> work;

    void mark(tdzdd::MyVector<uint64_t>& memo, size_t code) {
        size_t i = code % memoSize;
        uint64_t bit = uint64_t(1) << ((code / memoSize) % 64);
        memo[i] |= bit;
    }

    void mark(size_t code) {
        if (memoSize != 0) {
            mark(memo1, code);
            mark(memo2, code * 11);
            mark(memo3, code * 101);
            mark(memo4, code * 1009);
        }
    }

    bool marked(tdzdd::MyVector<uint64_t>& memo, size_t code) {
        size_t i = code % memoSize;
        uint64_t bit = uint64_t(1) << ((code / memoSize) % 64);
        return memo[i] & bit;
    }

    bool marked(size_t code) {
        return (memoSize != 0)
                && marked(memo1, code)
                && marked(memo2, code * 11)
                && marked(memo3, code * 101)
                && marked(memo4, code * 1009);
    }

    void* state(int level) {
        return &work[datasize * level];
    }

public:
    ZddSubsetterDF(tdzdd::NodeTableHandler<AR> const& input, Spec const& s,
            tdzdd::NodeTableHandler<AR>& output, size_t memoSize) :
            input(*input), output(output.privateEntity()), spec(s),
            datasize((spec.datasize() + sizeof(size_t) - 1) / sizeof(size_t)),
            memoSize(memoSize),
            memo1(memoSize),
            memo2(memoSize),
            memo3(memoSize),
            memo4(memoSize),
            work(datasize * this->input.numRows()) {
    }

    /**
     * Builds a DD.
     * @param root the root node.
     */
    void build(tdzdd::NodeId& root) {
        int n = spec.get_root(state(0));

        int k = (root == 1) ? -1 : root.row();

        while (n != 0 && k != 0 && n != k) {
            if (n < k) {
                assert(k >= 1);
                k = downTable(root, 0, n);
            }
            else {
                assert(n >= 1);
                n = downSpec(state(0), n, 0, k);
            }
        }

        if (n <= 0 || k <= 0) {
            assert(n == 0 || k == 0 || (n == -1 && k == -1));
            root = tdzdd::NodeId(0, n != 0 && k != 0);
            n = 0;
            output.init(1);
        }
        else {
            assert(n == k);
            assert(n == root.row());
            output.init(n + 1);
            root = build_(root, 0);
        }

        spec.destruct(state(0));
    }

private:
    tdzdd::NodeId build_(tdzdd::NodeId f, int p) {
        int i = f.row();
        size_t code = 0;
        if (i % 2 == 0) {
            code = f.hash() + spec.hash_code(state(p), i) * 271828171;
            if (marked(code)) return 0;
        }

        tdzdd::Node<AR> r;

        for (int b = 0; b < AR; ++b) {
            tdzdd::NodeId ff = f;
            spec.get_copy(state(p + 1), state(p));

            int kk = downTable(ff, b, i - 1);
            int ii = downSpec(state(p + 1), i, b, kk);

            while (ii != 0 && kk != 0 && ii != kk) {
                if (ii < kk) {
                    assert(kk >= 1);
                    kk = downTable(ff, 0, ii);
                }
                else {
                    assert(ii >= 1);
                    ii = downSpec(state(p + 1), ii, 0, kk);
                }
            }

            if (ii <= 0 || kk <= 0) {
                bool val = ii != 0 && kk != 0;
                r.branch[b] = val;
            }
            else {
                assert(ii == ff.row() && ii == kk && ii < i);
                r.branch[b] = build_(ff, p + 1);
            }

            spec.destruct(state(p + 1));
        }

        if (r == tdzdd::Node<AR>(0, 0)) {
            if (i % 2 == 0) mark(code);
            return 0;
        }
        output[i].push_back(r);
        return tdzdd::NodeId(i, output[i].size() - 1);
    }

    int downTable(tdzdd::NodeId& f, int b, int zerosupLevel) const {
        if (zerosupLevel < 0) zerosupLevel = 0;

        f = input.child(f, b);
        while (f.row() > zerosupLevel) {
            f = input.child(f, 0);
        }
        return (f == 1) ? -1 : f.row();
    }

    int downSpec(void* p, int level, int b, int zerosupLevel) {
        if (zerosupLevel < 0) zerosupLevel = 0;
        assert(level > zerosupLevel);

        int i = spec.get_child(p, level, b);
        while (i > zerosupLevel) {
            i = spec.get_child(p, i, 0);
        }
        return i;
    }
};

/**
 * Depth-first ZDD subsetting.
 * @param spec ZDD spec.
 */
template<typename SPEC, int ARITY>
void zddSubsetDF(tdzdd::DdStructure<ARITY>& dd,
        tdzdd::DdSpecBase<SPEC,ARITY> const& spec, size_t memoSize) {
    tdzdd::NodeTableHandler<ARITY>& diagram = dd.getDiagram();
    SPEC const& s = spec.entity();
    tdzdd::MessageHandler mh;
    mh.begin(typenameof(s)) << " ...";
    tdzdd::NodeTableHandler<ARITY> tmpTable;
    ZddSubsetterDF<SPEC> zs(diagram, s, tmpTable, memoSize);
    zs.build(dd.root());
    diagram = tmpTable;
    mh.end(dd.size());
}
