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

#include "DegreeZdd.hpp"
#include <cassert>
#include <cstdlib>

int DegreeZdd::getRoot(State* degree) const {
    for (int j = 0; j < quiz.cols; ++j) {
        degree[j] = (quiz.number[0][j] > 0 ? 1 : 0);
    }
    return top_level;
}

int DegreeZdd::getChild(State* degree, int level, int take) const {
  int i = (top_level - level) / (quiz.cols - 1);
  int j = (top_level - level) % (quiz.cols - 1);

    // horizontal line
    if (take) {
        if (degree[j] == 2 || degree[j + 1] == 2) return 0;
        ++degree[j];
        ++degree[j + 1];
    }

    // vertical line
    do {
        if (i < quiz.rows - 1) { // not bottom
            degree[j] = (degree[j] == 1 ? 1 : 0)
                    + (quiz.number[i + 1][j] > 0 ? 1 : 0);
        }
        else { // bottom
            if (degree[j] == 1) return 0;
            degree[j] = 2;
        }
    } while (++j == quiz.cols - 1); // repeat for the rightmost column

    --level;
    return (level > 0) ? level : -1;
}

void DegreeZdd::printState(std::ostream& os, State const* degree) const {
    for (int j = 0; j < quiz.cols; ++j) {
        int dj = degree[j];
        if (dj == j) os << " . ";
        else if (dj == quiz.cols) os << " * ";
        else if (dj > quiz.cols) os << "[" << dj << "]";
        else os << " " << dj << " ";
    }
}
