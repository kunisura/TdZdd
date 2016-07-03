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

#include <tdzdd/DdSpec.hpp>
#include "Board.hpp"

class NumlinZdd: public tdzdd::HybridDdSpec<NumlinZdd,int,uint8_t,2> {
    Board const& quiz;
    int const maxBlank;
    bool const noRoundabout;
    int finalHintRow;
    int finalHintCol;

public:
    /**
     * Constructor.
     * @param quiz matrix of number pairs.
     * @param maxBlank maximum number of unused boxes.
     * @param noRoundabout flag to prune roundabout ways.
     */
    NumlinZdd(Board const& quiz, int maxBlank, bool noRoundabout = false);

    /**
     * Gets a root configuration.
     * @param mate mate array.
     * @return root level.
     */
    int getRoot(S_State& k, A_State* mate) const;

    /**
     * Gets a child configuration.
     * @param mate mate array.
     * @param level decision level.
     * @param take 1 to take the edge; 0 otherwise.
     * @return next decision level.
     */
    int getChild(S_State& k, A_State* mate, int level, int take) const;

    /**
     * Prints a state.
     * @param os output stream.
     * @param a state array.
     */
    void printState(std::ostream& os, S_State const& k, A_State const* mate) const;

private:
    /**
     * Take a horizontal line (i, j)-(i, j+1).
     * @param mate mate array.
     * @param i row position.
     * @param j column position.
     * @return -1/0 when jumping to the 1/0-terminal.
     */
    int linkHoriz(A_State* mate, int i, int j) const;

    /**
     * Take a vertical line (i, j)-(i+1, j).
     * @param mate mate array.
     * @param i row position.
     * @param j column position.
     * @return -1/0 when jumping to the 1/0-terminal.
     */
    int linkVert(A_State* mate, int i, int j) const;

    /**
     * Check if the puzzle is completed.
     * @param mate mate array.
     * @param i row position of the last decision.
     * @param j column position of the last decision.
     * @return -1/0 when jumping to the 1/0-terminal.
     */
    int checkCompletion(A_State const* mate, int i, int j) const;
};
