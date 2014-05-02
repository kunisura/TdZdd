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

#include <cstdlib>

#include <tdzdd/DdSpec.hpp>

template<int ARITY>
class RandomDd: public tdzdd::DdSpec<RandomDd<ARITY>,size_t,ARITY> {
    int const n;
    size_t const width;
    int const drop;

public:
    RandomDd(int n, size_t width, double dropRatio)
            : n(n), width(width), drop(dropRatio * (double(RAND_MAX) + 1.0)) {
        assert(n >= 1);
        assert(width >= 1);
        assert(-1 <= drop && drop <= RAND_MAX);
    }

    int getRoot(size_t& state) const {
        state = 0;
        return n;
    }

    int getChild(size_t& state, int level, int take) const {
        if (level <= 1) return (std::rand() & 1) ? -1 : 0;
        if (std::rand() < drop) return 0;

        state *= (1ULL << 22) + 15;
        state += std::rand();
        state *= (1ULL << 22) + 15;
        state += std::rand();
        state *= (1ULL << 22) + 15;
        state += std::rand();
        state %= width;
        return level - 1;
    }
};
