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
#include <iostream>
#include <vector>

#include <tdzdd/DdSpec.hpp>

class ZddInterleave: public tdzdd::PodArrayDdSpec<ZddInterleave,tdzdd::NodeId,2> {
    std::vector<tdzdd::DdStructure<2> const*> dds;
    int const n;
    tdzdd::DdStructure<2> const dontCare;

    int getLevel(tdzdd::NodeId f, int k) const {
        int i = f.row();
        return (i == 0) ? -f.col() : (i - 1) * n + k + 1;
    }

    int getLevel(tdzdd::NodeId* a) const {
        int level = 0;
        for (int k = 0; k < n; ++k) {
            int i = getLevel(a[k], k);
            if (i == 0) return 0;
            level = std::max(level, i);
        }
        return level == 0 ? -1 : level;
    }

public:
    ZddInterleave(tdzdd::DdStructure<2> const& dd, int n)
            : dds(n), n(n) {
        setArraySize(n);
        for (int k = 0; k < n; ++k) {
            dds[k] = &dd;
        }
    }

    template<typename C>
    ZddInterleave(C const& c)
            : dds(), n(c.size()) {
        setArraySize(n);
        dds.reserve(n);
        for (typename C::const_iterator t = c.begin(); t != c.end(); ++t) {
            dds.push_back(&*t);
        }
    }

    ZddInterleave(tdzdd::DdStructure<2> const& dd, int n, int pos)
            : dds(n), n(n), dontCare(dd.topLevel()) {
        setArraySize(n);
        for (int k = 0; k < n; ++k) {
            dds[k] = (k == pos) ? &dd : &dontCare;
        }
    }

    int getRoot(tdzdd::NodeId* a) const {
        for (int k = 0; k < n; ++k) {
            a[k] = dds[k]->root();
        }
        return getLevel(a);
    }

    int getChild(tdzdd::NodeId* a, int level, int b) const {
        int i = (level - 1) / n + 1;
        int k = (level - 1) % n;
        if (a[k].row() == i) {
            a[k] = dds[k]->child(a[k], b);
        }
        else if (b) {
            a[k] = 0;
        }
        return getLevel(a);
    }
};
