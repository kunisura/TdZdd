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

NoUTurnZdd::NoUTurnZdd(Board const& quiz, bool kansai) :
        quiz_(quiz), kansai(kansai), rows(quiz.getRows()), cols(quiz.getCols()),
        maxLevel(rows * (cols - 1)) {
    setArraySize(cols);
}

int NoUTurnZdd::getRoot(State* a) const {
    for (int j = 0; j < cols; ++j) {
        a[j].hline = false;
        a[j].vline = false;
        a[j].filled = false;
    }
    return maxLevel;
}

int NoUTurnZdd::getChild(State* a, int level, int take) const {
    std::div_t pos = level2pos(level);
    int i = pos.quot;
    int j = pos.rem;
    bool& roof = a[cols - 1].hline;

    // horizontal line
    if (take) {
        if (a[j].vline + a[j].hline + a[j + 1].vline >= 2) return 0;
        if (a[j].vline && !a[j + 1].filled) return 0;
        if (!a[j].filled && a[j + 1].vline) return 0;
    }

    // vertical line
    int d = (quiz_.number[i][j] != 0) + a[j].vline + take;
    if (j >= 1) d += a[j - 1].hline;
    if (!kansai && d == 0) return 0;

    if (!a[j].hline || take) roof = false;
    else if (a[j].vline) roof = true;
    else if (d != 0) roof = false;
    if (roof && a[j + 1].vline) return 0;

    a[j].hline = take;
    a[j].vline = (d == 1);
    a[j].filled = (d != 0);
    if (j >= 1 && a[j - 1].hline && a[j - 1].vline && a[j].vline) return 0;

    if (++j == cols - 1) { // repeat for the rightmost column
        int d = (quiz_.number[i][j] != 0) + a[j].vline;
        if (j >= 1) d += a[j - 1].hline;
        if (!kansai && d == 0) return 0;

        roof = false;

        a[j].filled = (d >= 1);
        a[j].vline = (d == 1);
        if (j >= 1 && a[j - 1].hline && a[j - 1].vline && a[j].vline) return 0;
    }

    --level;
    return (level > 0) ? level : -1;
}

void NoUTurnZdd::printState(std::ostream& os, State const* a) const {
    for (int j = 0; j < cols; ++j) {
        os << (a[j].vline ? "|" : a[j].filled ? "*" : "O")
                << (a[j].hline ? "-" : ".");
    }
}
