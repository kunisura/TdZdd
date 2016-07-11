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

/* mate values (0 <= j < n)
 *  mate[j] = j  degree=0
 *  mate[j] = n  degree=2
 *  mate[j] > n  connected to number (mate[j]-cols)
 *  other        connected to mate[j]
 */

int NumlinZdd::getRoot(S_State& blank_count, A_State* mate) const {
    blank_count = 0;
    for (int j = 0; j < n; ++j) {
        int t = quiz.number[0][j];
        mate[j] = (t > 0) ? n + t : j;
    }
    return top_level;
}

int NumlinZdd::getChild(S_State& blank_count, A_State* mate, int level,
        int take) const {
    int i = (top_level - level) / (n - 1);
    int j = (top_level - level) % (n - 1);

    if (take) { // $\HEdge{i}{j}$ を採用
        int k = j + 1;
        int mj = mate[j];
        int mk = mate[k];

        if (mj == n || mk == n) return 0;  // 分岐
        if (mj == k) return 0; // サイクル

        mate[j] = mate[k] = n;

        if (mj > n && mk > n) { // パスが完成
            if (mj != mk) return 0; // ラベルが異なる
        }
        else { // 接続関係を更新
            if (mj < n) mate[mj] = mk;
            if (mk < n) mate[mk] = mj;
        }
    }

    // 列 $\mathit{j}$ から列 $\mathit{jj}$ まで決定
    int jj = (j < n - 2) ? j : n - 1;

    if (i < m - 1) { // 最終行でないとき
        for (int k = j; k <= jj; ++k) { // $\VEdge{i}{k}$
            int mk = mate[k];
            if (mk == k) { // degree=0
                if (blank_count == max_blank) return 0;
                if (max_blank > 0) ++blank_count;
            }
            int t = quiz.number[i + 1][k];
            if (mk != k && mk != n) { // 採用
                if (t > 0) { // $\Vertex{i+1}{k}$ はラベル付き
                    mate[k] = n;
                    if (mk > n) { // パスが完成
                        if (mk != n + t) return 0; // ラベルが異なる
                    }
                    else {
                        mate[mk] = n + t;
                    }
                }
            }
            else { // 不採用
                mate[k] = (t > 0) ? n + t : k;
            }
        }
    }
    else { // 最終行のとき
        for (int k = j; k <= jj; ++k) { // $\Vertex{i}{k}$
            int mk = mate[k];
            if (mk == k) { // degree=0
                if (blank_count == max_blank) return 0;
                if (max_blank > 0) ++blank_count;
            }
            else if (mk != n) {
                return 0;
            }
            mate[k] = n;
        }
    }

    return (--level >= 1) ? level : -1;
}

void NumlinZdd::printState(std::ostream& os, S_State const& blank_count,
        A_State const* mate) const {
    for (int j = 0; j < quiz.cols; ++j) {
        int mj = mate[j];
        if (mj == j) os << " . ";
        else if (mj == quiz.cols) os << " * ";
        else if (mj > quiz.cols) os << "[" << mj << "]";
        else os << " " << mj << " ";
        os << "(" << blank_count << ")";
    }
}
