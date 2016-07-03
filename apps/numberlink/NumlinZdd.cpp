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

#include "NumlinZdd.hpp"
#include <cassert>
#include <cstdlib>

/* mate values (0 <= j < cols)
 *  mate[j] = j          degree=0
 *  mate[j] = cols       degree=2
 *  mate[j] > cols       connected to number (mate[j]-cols)
 *  other                connected to mate[j]
 */

NumlinZdd::NumlinZdd(Board const& quiz, int maxBlank, bool noRoundabout) :
        quiz(quiz), maxBlank(maxBlank), noRoundabout(noRoundabout) {
    setArraySize(quiz.cols);

    for (finalHintRow = quiz.rows - 1; finalHintRow >= 0; --finalHintRow) {
        for (finalHintCol = quiz.cols - 1; finalHintCol >= 0; --finalHintCol) {
            if (quiz.number[finalHintRow][finalHintCol] > 0) return;
        }
    }
}

int NumlinZdd::getRoot(S_State& k, A_State* mate) const {
    k = 0;
    for (int j = 0; j < quiz.cols; ++j) {
        int t = quiz.number[0][j];
        mate[j] = (t > 0) ? quiz.cols + t : j;
    }
    return quiz.top_level;
}

int NumlinZdd::getChild(S_State& k, A_State* mate, int level, int take) const {
    int i = quiz.level2row(level);
    int j = quiz.level2col(level);

    // horizontal line
    if (take) {
        int ret = linkHoriz(mate, i, j);
        if (ret <= 0) return ret;
        if (noRoundabout && j < quiz.cols - 2 && mate[j + 1] == j + 2) {
            return 0;
        }
    }
    else if (noRoundabout && mate[j] > quiz.cols && mate[j] == mate[j + 1]) {
        return 0;
    }

    // vertical line
    do {
        if (mate[j] == j) { // degree=0
            if (k == maxBlank) return 0;
            if (maxBlank > 0) ++k;
        }

        if (i < quiz.rows - 1) { // not bottom
            if (mate[j] != j && mate[j] != quiz.cols) { // degree=1
                int ret = linkVert(mate, i, j);
                if (ret <= 0) return ret;
                if (noRoundabout && j >= 1 && mate[j] == j - 1) {
                    return 0;
                }
            }
            else {
                int t = quiz.number[i + 1][j];
                if (t > 0) {
                    if (noRoundabout && mate[j] == quiz.cols + t) {
                        return 0;
                    }
                    mate[j] = quiz.cols + t;
                }
                else {
                    mate[j] = j;
                }
            }
        }
        else { // bottom
            if (mate[j] != j && mate[j] != quiz.cols) return 0; // degree=1
            mate[j] = quiz.cols;
        }
    } while (++j == quiz.cols - 1); // repeat for the rightmost column

    return level - 1;
}

int NumlinZdd::linkHoriz(A_State* mate, int i, int j) const {
    int k = j + 1;
    int mj = mate[j];
    int mk = mate[k];

    if (mj == quiz.cols || mk == quiz.cols) return 0;  // degree=2
    if (mj == k) return 0; // cycle

    mate[j] = mate[k] = quiz.cols;

    if (mj < quiz.cols || mk < quiz.cols) { // not connecting two numbers
        if (mj < quiz.cols) mate[mj] = mk;
        if (mk < quiz.cols) mate[mk] = mj;
        return 1;
    }

    // connecting two numbers
    assert(mj > quiz.cols && mk > quiz.cols);
    if (mj != mk) return 0; // connecting incompatible numbers

    return checkCompletion(mate, i + 1, j - 1); // touched previously
}

int NumlinZdd::linkVert(A_State* mate, int i, int j) const {
    int mj = mate[j];
    int t = quiz.number[i + 1][j];
    assert(mj != quiz.cols);

    if (t == 0) { // no number at (i+1, j)
        return 1;
    }

    // number at (i+1, j)
    if (mj < quiz.cols) { // no number at (i, j)
        mate[j] = quiz.cols;
        mate[mj] = quiz.cols + t;
        return 1;
    }

    // connecting two numbers
    assert(mj > quiz.cols);
    if (mj != quiz.cols + t) return 0; // connecting incompatible numbers

    mate[j] = quiz.cols;
    return checkCompletion(mate, i + 1, j);
}

int NumlinZdd::checkCompletion(A_State const* mate, int i, int j) const {
    if (i < finalHintRow || (i == finalHintRow && j < finalHintCol)) return 1;

    bool acceptable = true;
    bool completed = true;

    for (int k = 0; k < quiz.cols; ++k) {
        if (mate[k] < quiz.cols && mate[k] != k) { // garbage found
            acceptable = false;
        }
        else if (mate[k] > quiz.cols) { // incomplete path found
            completed = false;
            break;
        }
    }

    return completed ? (acceptable ? -1 : 0) : 1;
}

void NumlinZdd::printState(std::ostream& os, S_State const& k,
        A_State const* mate) const {
    for (int j = 0; j < quiz.cols; ++j) {
        int mj = mate[j];
        if (mj == j) os << " . ";
        else if (mj == quiz.cols) os << " * ";
        else if (mj > quiz.cols) os << "[" << mj << "]";
        else os << " " << mj << " ";
        os << "(" << k << ")";
    }
}
