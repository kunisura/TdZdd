/*
 * TdZdd: a Top-down/Breadth-first Decision Diagram Manipulation Framework
 * by Hiroaki Iwashita <iwashita@erato.ist.hokudai.ac.jp>
 * Copyright (c) 2014 ERATO MINATO Project
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

#include <tdzdd/DdEval.hpp>

#include "../cnfbdd/Cudd.hpp"

/**
 * Exporter to Cudd.
 */
struct ZtddToCudd: public tdzdd::DdEval<ZtddToCudd,Cudd> {
    bool showMessages() const {
        return true;
    }

    void evalTerminal(Cudd& f, int value) const {
        f = Cudd(value ? 0 : 1);
    }

    void evalNode(Cudd& f, int level,
            tdzdd::DdValues<Cudd,3> const& values) const {
        Cudd const& fz = values.get(0);
        Cudd const& fn = values.get(1);
        Cudd const& fp = values.get(2);
        f = Cudd(level, fz & fp, fz & fn);
    }
};
