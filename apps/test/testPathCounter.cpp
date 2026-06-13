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
#include <string>
#include <tdzdd/DdSpec.hpp>
#include <tdzdd/dd/PathCounter.hpp>

namespace {

class FullBinary: public tdzdd::DdSpec<FullBinary,int,2> {
    int const n;

public:
    explicit FullBinary(int n)
            : n(n) {
    }

    int getRoot(int& state) const {
        state = 0;
        return n;
    }

    int getChild(int& state, int level, int value) const {
        state += value;
        return level == 1 ? -1 : level - 1;
    }
};

class HighAritySkipMerge: public tdzdd::DdSpec<HighAritySkipMerge,int,99> {
    int const n;

public:
    explicit HighAritySkipMerge(int n)
            : n(n) {
    }

    int getRoot(int& state) const {
        state = 0;
        return n;
    }

    int getChild(int& state, int level, int value) const {
        if (level == n) {
            if (value == 0) {
                state = 0;
                return 1;
            }
            state = 1;
            return level - 1;
        }

        if (level == 1) {
            return value == 0 ? -1 : 0;
        }

        if (level == 2) {
            state = 0;
            return 1;
        }

        return level - 1;
    }
};

std::string multiply(std::string s, int n) {
    int carry = 0;
    for (std::string::reverse_iterator t = s.rbegin(); t != s.rend(); ++t) {
        int const v = (*t - '0') * n + carry;
        *t = char('0' + v % 10);
        carry = v / 10;
    }
    while (carry != 0) {
        s.insert(s.begin(), char('0' + carry % 10));
        carry /= 10;
    }
    return s;
}

std::string addOne(std::string s) {
    for (std::string::reverse_iterator t = s.rbegin(); t != s.rend(); ++t) {
        if (*t != '9') {
            ++*t;
            return s;
        }
        *t = '0';
    }
    s.insert(s.begin(), '1');
    return s;
}

std::string expectedHighAritySkipMerge(int n) {
    std::string s = "98";
    for (int i = 0; i < n - 2; ++i) {
        s = multiply(s, 99);
    }
    return addOne(s);
}

} // namespace

TEST(PathCounterTest, CountsWideBinaryPathSet) {
    FullBinary spec(70);
    std::string const expected = "1180591620717411303424";

    ASSERT_EQ(expected, tdzdd::countPaths(spec));
    ASSERT_EQ(expected, tdzdd::countPaths(spec, true));
}

TEST(PathCounterTest, CountsLongSkipMergeWithFixedCounterWidth) {
    int const n = 80;
    HighAritySkipMerge spec(n);
    std::string const expected = expectedHighAritySkipMerge(n);

    ASSERT_EQ(expected, tdzdd::countPaths(spec));
    ASSERT_EQ(expected, tdzdd::countPaths(spec, true));
}
