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

#pragma once

#include <iostream>
#include <vector>

class Board {
public:
    int rows;
    int cols;
    std::vector<std::vector<int> > number;
    std::vector<std::vector<bool> > hlink;
    std::vector<std::vector<bool> > vlink;

    /**
     * Initialize the board based on \p rows and \p cols.
     */
    void init();

    /**
     * Reflect the elements along its main diagonal.
     */
    void transpose();

    /**
     * Read a board configuration.
     * @param is input stream.
     */
    void readNumbers(std::istream& is);

    /**
     * Write a board configuration.
     * @param is input stream.
     */
    void writeNumbers(std::ostream& os) const;

    /**
     * Print the board graphically as a Numberlink puzzle.
     * @param os output stream.
     */
    void printNumlin(std::ostream& os) const;

    /**
     * Complete Numberlink by linking vertical lines.
     */
    void makeVerticalLinks();

    /**
     * Link numbers by horizontal & vertical lines.
     */
    void fillNumbers();

private:
    int fillNumbersFrom(int i, int j);
};
