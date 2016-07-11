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

#include "Board.hpp"

#include <iomanip>
#include <stdexcept>

void Board::init() {
    number.clear();
    number.resize(rows);
    for (int i = 0; i < rows; ++i) {
        number[i].resize(cols);
    }

    hlink.clear();
    hlink.resize(rows);
    for (int i = 0; i < rows; ++i) {
        hlink[i].resize(cols - 1);
    }

    vlink.clear();
    vlink.resize(rows - 1);
    for (int i = 0; i < rows - 1; ++i) {
        vlink[i].resize(cols);
    }
}

namespace {

template<typename M>
M transposed_matrix(M& a) {
    int rows = a[0].size();
    int cols = a.size();
    M b(rows);
    for (int i = 0; i < rows; ++i) {
        b[i].resize(cols);
        for (int j = 0; j < cols; ++j) {
            b[i][j] = a[j][i];
        }
    }
    return b;
}

}

void Board::transpose() {
    number = transposed_matrix(number);
    std::vector<std::vector<bool> > tmp = transposed_matrix(hlink);
    hlink = transposed_matrix(vlink);
    vlink = tmp;
    std::swap(rows, cols);
}

void Board::readNumbers(std::istream& is) {
    int c;

    for (;;) {
        if (!is) throw std::runtime_error("format error");
        while (isspace(c = is.get()))
            ;
        if (isdigit(c)) break;
        while (is && (c = is.get()) != '\n')
            ;
    }
    is.unget();
    is >> cols;

    while (isspace(c = is.get()))
        ;
    if (c == ',') {
        rows = cols;
        while (isspace(c = is.get()))
            ;
        if (!isdigit(c)) throw std::runtime_error("format error");
        is.unget();
        is >> cols;
    }
    else {
        if (!isdigit(c)) throw std::runtime_error("format error");
        is.unget();
        is >> rows;
    }

    if (cols < 1 || rows < 1) throw std::runtime_error("illegal size");

    init();

    int i = -1;
    int j = 0;

    while (is) {
        while (isspace(c = is.get())) {
            if (c == '\n') ++i, j = 0;
        }
        if (!is) break;

        if (i < 0 || i >= rows) continue;
        if (j >= cols) continue;

        if (isdigit(c)) {
            is.unget();
            int t;
            is >> t;
            number[i][j] = t;
        }
        ++j;
    }
}

void Board::writeNumbers(std::ostream& os) const {
    os << cols << " " << rows << "\n";

    for (int i = 0; i < rows; ++i) {
        for (int j = 0; j < cols; ++j) {
            if (j != 0) os << " ";
            os << number[i][j];
        }
        os << "\n";
    }
}

void Board::printNumlin(std::ostream& os) const {
    static char const* connector[] = {"  ", "  ", "  ", "──", "  ", "─┘", " └",
                                      "─┴", "  ", "─┐", " ┌", "─┬", " │", "─┤",
                                      " ├", "─┼"};
    os << "┏";
    for (int j = 0; j < cols; ++j) {
        os << "━━";
    }
    os << "━┓\n";

    for (int i = 0; i < rows; ++i) {
        os << "┃";

        for (int j = 0; j < cols; ++j) {
            int k = number[i][j];
            int c = 0;
            if (j - 1 >= 0 && hlink[i][j - 1]) c |= 1;
            if (j + 1 < cols && hlink[i][j]) c |= 2;
            if (i - 1 >= 0 && vlink[i - 1][j]) c |= 4;
            if (i + 1 < rows && vlink[i][j]) c |= 8;
            if (c <= 2 || c == 4 || c == 8) {
                if (k == 0) os << "  ";
                else os << std::setw(2) << k;
            }
            else {
                os << connector[c];
            }
        }

        os << " ┃\n";
    }

    os << "┗";
    for (int j = 0; j < cols; ++j) {
        os << "━━";
    }
    os << "━┛\n";
}

void Board::makeVerticalLinks() {
    std::vector<std::vector<int> > degree(rows);
    for (int i = 0; i < rows; ++i) {
        degree[i].resize(cols);
        for (int j = 0; j < cols; ++j) {
            if (number[i][j]) ++degree[i][j];
            if (j >= 1 && hlink[i][j - 1]) ++degree[i][j];
            if (j + 1 < cols && hlink[i][j]) ++degree[i][j];
        }
    }

    for (int i = 0; i < rows - 1; ++i) {
        for (int j = 0; j < cols; ++j) {
            if (degree[i][j] == 1) {
                vlink[i][j] = true;
                ++degree[i][j];
                ++degree[i + 1][j];
            }
            else {
                vlink[i][j] = false;
            }
        }
    }
}

void Board::fillNumbers() {
    for (int i = 0; i < rows; ++i) {
        for (int j = 0; j < cols; ++j) {
            fillNumbersFrom(i, j);
        }
    }
}

int Board::fillNumbersFrom(int i, int j) {
    int& num = number[i][j];

    if (num < 0) return 0;
    if (num > 0) return num;

    num = -1;

    if (i - 1 >= 0 && vlink[i - 1][j]) {
        num = fillNumbersFrom(i - 1, j);
        if (num > 0) return num;
    }

    if (j - 1 >= 0 && hlink[i][j - 1]) {
        num = fillNumbersFrom(i, j - 1);
        if (num > 0) return num;
    }

    if (j < cols - 1 && hlink[i][j]) {
        num = fillNumbersFrom(i, j + 1);
        if (num > 0) return num;
    }

    if (i < rows - 1 && vlink[i][j]) {
        num = fillNumbersFrom(i + 1, j);
        if (num > 0) return num;
    }

    num = 0;
    return num;
}
