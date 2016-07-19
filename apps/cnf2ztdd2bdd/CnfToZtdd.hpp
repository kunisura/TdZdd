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

#include <cassert>
#include <iostream>
#include <vector>

#include <tdzdd/DdSpec.hpp>

#include "../cnfbdd/Cudd.hpp"

struct CnfToZtddState {
    typedef std::vector<int> Clause;

    Clause const* beg; // the first clause
    Clause const* end; // following the last clause
    size_t pos; // literal position

    size_t hash() const;
    bool operator==(CnfToZtddState const& o) const;
};

class CnfToZtdd: public tdzdd::DdSpec<CnfToZtdd,CnfToZtddState,3> {
    typedef CnfToZtddState::Clause Clause;

    int nv; // the number of variables
    std::vector<Clause> cnf; // sorted list of clauses

public:
    /**
     * Gets the variable number at given level.
     * @param level variable level.
     * @return valiable number.
     */
    int varAtLevel(int level) const {
        assert(1 <= level && level <= nv);
        return nv - level + 1;
    }

    /**
     * Gets the variable level of a given variable.
     * @param var variable number.
     * @return valiable level.
     */
    int levelOfVar(int var) const {
        assert(1 <= var && var <= nv);
        return nv - var + 1;
    }

    /**
     * Gets the number of variables.
     * @return the number of clauses.
     */
    int numVars() const {
        return nv;
    }

    /**
     * Gets the number of clauses.
     * @return the number of clauses.
     */
    size_t numClauses() const {
        return cnf.size();
    }

    /**
     * Reads DIMACS CNF.
     * @param is input stream to read.
     */
    void load(std::istream& is) {
        readDimacs(is);
        sortClauses();
    }

private:
    struct VarCmp {
        bool& tautology;

        VarCmp(bool& tautology) :
                tautology(tautology) {
        }

        bool operator()(int a, int b) {
            if (a == -b) tautology = true;
            return std::abs(a) < std::abs(b);
        }
    };

    struct ClauseCmp {
        bool operator()(Clause const& a, Clause const& b) const {
            int n = std::min(a.size(), b.size());
            for (int i = 0; i < n; ++i) {
                if (a[i] == -b[i]) return a[i] < b[i];
                if (std::abs(a[i]) != std::abs(b[i]))
                    return std::abs(a[i]) > std::abs(b[i]);
            }
            return a.size() < b.size();
        }
    };

    struct ClauseEq {
        bool operator()(Clause const& a, Clause const& b) const {
            return a == b;
        }
    };

    class ClauseSearcher {
        size_t pos;

    public:
        ClauseSearcher(int pos) :
                pos(pos) {
        }

        bool operator()(Clause const& a, int b) const {
            if (a.size() <= size_t(pos)) return true;
            if (a[pos] == -b) return a[pos] < b;
            return std::abs(a[pos]) > std::abs(b);
        }
    };

    void readDimacs(std::istream& is);
    void sortClauses();

public:
    int getRoot(State& s);
    int getChild(State& s, int level, int value);

    size_t hashCode(State const& s) const {
        return s.hash();
    }

    void printLevel(std::ostream& os, int level) const {
        os << varAtLevel(level);
    }

    void printState(std::ostream& os, State const& s) const;
};
