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

class ColoredZdd: public tdzdd::PodArrayDdSpec<ColoredZdd,tdzdd::NodeId,2> {
    std::vector<tdzdd::DdStructure<2> const*> dds;
    int const colors;

    int getLevel(tdzdd::NodeId f, int k) const {
        int i = f.row();
        return (i == 0) ? -f.col() : (i - 1) * colors + k + 1;
    }

    int getLevel(tdzdd::NodeId const* a) const {
        int level = 0;
        for (int k = 0; k < colors; ++k) {
            int i = getLevel(a[k], k);
            if (i == 0) return 0;
            level = std::max(level, i);
        }
        return level == 0 ? -1 : level;
    }

public:
    ColoredZdd(tdzdd::DdStructure<2> const& dd, int colors)
            : dds(colors), colors(colors) {
        setArraySize(colors);
        for (int k = 0; k < colors; ++k) {
            dds[k] = &dd;
        }
    }

    template<typename C>
    ColoredZdd(C const& c)
            : dds(), colors(c.size()) {
        setArraySize(colors);
        dds.reserve(colors);
        for (typename C::const_iterator t = c.begin(); t != c.end(); ++t) {
            dds.push_back(&*t);
        }
    }

    int getRoot(tdzdd::NodeId* a) const {
        for (int k = 0; k < colors; ++k) {
            a[k] = dds[k]->root();
        }
        return getLevel(a);
    }

    int getChild(tdzdd::NodeId* a, int level, int b) const {
        int i = (level - 1) / colors + 1;
        int k = (level - 1) % colors;

        if (b) {
            if (a[k].row() == i) {
                a[k] = dds[k]->child(a[k], true);
            }
            else {
                a[k] = 0;
            }

            for (int kk = k - 1; kk >= 0; --kk) {
                if (a[kk].row() == i) {
                    a[kk] = dds[kk]->child(a[kk], false);
                }
            }
        }
        else {
            bool lastOne = true;
            for (int kk = k - 1; kk >= 0; --kk) {
                if (a[kk].row() == i) {
                    lastOne = false;
                    break;
                }
            }
            if (lastOne) return 0;

            if (a[k].row() == i) {
                a[k] = dds[k]->child(a[k], false);
            }
        }

        return getLevel(a);
    }
};
