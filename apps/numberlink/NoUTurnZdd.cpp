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

#include "NoUTurnZdd.hpp"
#include <cassert>
#include <cstdlib>

ConstraintZdd::ConstraintZdd(Board const& quiz) :
        quiz(quiz) {
    setArraySize(quiz.cols);
}

int ConstraintZdd::getRoot(State* a) const {
    for (int j = 0; j < quiz.cols; ++j) {
        a[j].hline = false;
        a[j].vline = false;
        a[j].filled = false;
    }
    return quiz.top_level;
}

int ConstraintZdd::getChild(State* a, int level, int take) const {
    bool& roof = a[quiz.cols - 1].hline;
    int i = quiz.level2row(level);
    int j = quiz.level2col(level);

    // horizontal line v(i,j)-v(i,j+1)
    if (take) {
        if (a[j].vline + a[j].hline + a[j + 1].vline >= 2) return 0; // no U-turn
        if (a[j].vline && !a[j + 1].filled) return 0; // ┓ on behalf of ┗
        if (!a[j].filled && a[j + 1].vline) return 0; // ┏ on behalf of ┛
    }

    // _..._ on behalf of ⎡˙˙˙⎤
    if (!a[j].hline || take) roof = false;
    else if (a[j].vline) roof = true;
    else if (quiz.number[i][j] != 0) roof = false;
    if (roof && a[j + 1].vline) return 0;

    // vertex degree of (v(i,j))
    int d = (quiz.number[i][j] != 0) + a[j].vline + take;
    if (j >= 1) d += a[j - 1].hline;
    a[j].hline = take;
    a[j].vline = (d == 1);
    a[j].filled = (d >= 1);
    // no U-turn connection
    if (j >= 1 && a[j - 1].hline && a[j - 1].vline && a[j].vline) return 0;

    if (++j == quiz.cols - 1) { // for the rightmost column
        int d = (quiz.number[i][j] != 0) + a[j].vline + a[j - 1].hline;
        roof = false;
        a[j].vline = (d == 1);
        a[j].filled = (d >= 1);
        // no U-turn connection
        if (a[j - 1].hline && a[j - 1].vline && a[j].vline) return 0;
    }

    --level;
    return (level > 0) ? level : -1;
}

void ConstraintZdd::printState(std::ostream& os, State const* a) const {
    for (int j = 0; j < quiz.cols; ++j) {
        os << (a[j].vline ? "|" : a[j].filled ? "*" : "O")
           << (a[j].hline ? "-" : ".");
    }
}
