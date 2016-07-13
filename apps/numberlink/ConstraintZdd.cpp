/*
 * Copyright (c) 2014 Hiroaki Iwashita
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

#include "ConstraintZdd.hpp"
#include <cassert>
#include <cstdlib>

int ConstraintZdd::getRoot(S_State& s, A_State* a) const {
    s = false;
    for (int j = 0; j < quiz.cols; ++j) {
        a[j].to_east = false;
        a[j].to_south = false;
        a[j].used = false;
    }
    return top_level;
}

int ConstraintZdd::getChild(S_State& s, A_State* a, int level, int take) const {
    int i = (top_level - level) / (quiz.cols - 1);
    int j = (top_level - level) % (quiz.cols - 1);

    if (take) {// use e_h(i,j)
        if (a[j].to_south + a[j].to_east + a[j + 1].to_south >= 2) return 0; // ⊐, ⊔, ⊏
        if (a[j].to_south && !a[j + 1].used) return 0; // ⸤
        if (!a[j].used && a[j + 1].to_south) return 0; // ⸥
    }

    // check ⸢˙˙˙⸣
    if (!a[j].to_east || take) s = false;
    else if (a[j].to_south) s = true;
    else if (quiz.number[i][j] != 0) s = false;
    if (s && a[j + 1].to_south) return 0;

    // degree of v(i,j)
    int d = (quiz.number[i][j] != 0) + a[j].to_south + take;
    if (j >= 1) d += a[j - 1].to_east;
    a[j].to_east = take;
    a[j].to_south = (d == 1);
    a[j].used = (d >= 1);
    if (j >= 1 && a[j - 1].to_east && a[j - 1].to_south && a[j].to_south)
        return 0; // ⊓

    if (++j == quiz.cols - 1) { // for the rightmost column
        int d = (quiz.number[i][j] != 0) + a[j].to_south + a[j - 1].to_east;
        s = false;
        a[j].to_south = (d == 1);
        a[j].used = (d >= 1);
        if (a[j - 1].to_east && a[j - 1].to_south && a[j].to_south) return 0; // ⊓
    }

    return (--level > 0) ? level : -1;
}

void ConstraintZdd::printState(std::ostream& os, S_State const& s,
        A_State const* a) const {
    for (int j = 0; j < quiz.cols; ++j) {
        os << (a[j].to_south ? "|" : a[j].used ? "*" : "O")
           << (a[j].to_east ? "-" : ".");
    }
    if (s) os << " ┌";
}
