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

struct NoTurnZddState {
    bool hline;
    bool vline;
    bool filled;
};

class NoUTurnZdd: public tdzdd::PodArrayDdSpec<NoUTurnZdd,NoTurnZddState,2> {
    Board const& quiz_;
    bool const kansai;
    int const rows;
    int const cols;
    int const maxLevel;

public:
    /**
     * Constructor.
     * @param quiz matrix of number pairs.
     * @param kansai true if unused box is allowed.
     */
    NoUTurnZdd(Board const& quiz, bool kansai);

    /**
     * Gets the row and column positions of a given level.
     * @param level decision level.
     * @return row and column positions (quot, rem).
     */
    std::div_t level2pos(int level) const {
        return std::div(maxLevel - level, cols - 1);
    }

    /**
     * Returns the quiz.
     * @return quiz.
     */
    Board const& quiz() const {
        return quiz_;
    }

    /**
     * Gets a root configuration.
     * @param a state array.
     * @return root level.
     */
    int getRoot(State* a) const;

    /**
     * Gets a child configuration.
     * @param a state array.
     * @param level decision level.
     * @param take 1 to take the edge; 0 otherwise.
     * @return next decision level.
     */
    int getChild(State* a, int level, int take) const;

    /**
     * Prints a state.
     * @param os output stream.
     * @param a state array.
     */
    void printState(std::ostream& os, State const* a) const;
};
