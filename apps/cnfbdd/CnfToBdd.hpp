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
#include <tdzdd/util/MySet.hpp>

#include "Cudd.hpp"

struct CnfToBddState {
    typedef int ClauseNumber;
    typedef tdzdd::MySmallSetOnPool<ClauseNumber> ClauseSet;

    ClauseSet* set;
    ClauseSet* id;
};

class CnfToBdd: public tdzdd::DdSpec<CnfToBdd,CnfToBddState,2> {
    typedef CnfToBddState::ClauseNumber ClauseNumber;
    typedef CnfToBddState::ClauseSet ClauseSet;
    typedef std::vector<ClauseNumber> ClauseList;
    typedef std::vector<int> Clause;

    int nv; // the number of variables
    int nc; // the number of clauses
    std::vector<Clause> cnf; // sorted list of clauses

    std::vector<ClauseList> posiClauses; // list of positive clauses for each level
    std::vector<ClauseList> negaClauses; // list of negative clauses for each level
    std::vector<ClauseList> enterClauses; // list of entering clauses for each level
    std::vector<ClauseList> leaveClauses; // list of leaving clauses for each level
    std::vector<ClauseList> frontierClauses; // list of active clauses for each level

    std::vector<Cudd> clauseVar; // BDD variable for each clause
    std::vector<Cudd> posiCube; // cube of positive clauses for each level
    std::vector<Cudd> negaCube; // cube of negative clauses for each level
    std::vector<Cudd> posiMask; // mask of positive clauses for each level
    std::vector<Cudd> negaMask; // mask of negative clauses for each level
    std::vector<Cudd> enterCube; // cube of entering clauses for each level
    std::vector<Cudd> leaveCube; // cube of leaving clauses for each level
    std::vector<Cudd> enterConstraint; // entering constraint for each level
    std::vector<Cudd> leaveConstraint; // leaving constraint for each level
    std::vector<Cudd> frontierCube; // cube of active clauses for each level
    int completingLevel; // the highest level where all clauses are touched

    std::vector<ClauseList> clauseMap; // mapping to canonical clause for each level
    std::vector<std::vector<Cudd> > clauseMapCube; // cube of source clauses for each level
    std::vector<std::vector<Cudd> > clauseMapCond; // union of source clauses for each level
    bool useClauseMap_; // switch for clauseMap

    std::vector<Cudd> frontierSet; // reachable state set for each level
    //std::vector<DdStructure> frontierZdd; // reachable state set for each level

    tdzdd::MemoryPools pools;
    std::vector<uint16_t> work;

public:
    /**
     * Enables/disables mapping to canonical clause IDs.
     * @param flag new value.
     * @return old value.
     */
    bool useClauseMap(bool flag) {
        bool tmp = useClauseMap_;
        useClauseMap_ = flag;
        return tmp;
    }

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
     * Gets the variable level of a given literal.
     * @param lit literal code.
     * @return valiable level.
     */
    int levelOfLiteral(int lit) const {
        assert(lit != 0 && std::abs(lit) <= nv);
        return nv - std::abs(lit) + 1;
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
    int numClauses() const {
        return nc;
    }

    /**
     * Reads DIMACS CNF and analyze reachability.
     * @param is input stream to read.
     * @param sort enables clause sorting.
     */
    void load(std::istream& is, bool sort = false) {
        readDimacs(is);
        if (sort) sortClauses();
        prepare();
    }

    /**
     * Makes rich reachability information.
     * @param limit the maximum BDD size in top-down traversal.
     */
    void traverse(size_t limit);

private:
    struct VarCmp {
        bool& tautology;

        VarCmp(bool& tautology)
                : tautology(tautology) {
        }

        bool operator()(int a, int b) {
            if (a == -b) tautology = true;
            return std::abs(a) < std::abs(b);
        }
    };

    struct ClauseCmpForOrdering {
        bool operator()(Clause const& a, Clause const& b) const {
            int n = std::min(a.size(), b.size());
            for (int i = 0; i < n; ++i) {
                int const v = std::abs(a[i]);
                int const w = std::abs(b[i]);
                if (v != w) return v < w;
            }
            return a.size() < b.size();
        }

        bool operator()(Clause const* a, Clause const* b) const {
            return operator()(*a, *b);
        }
    };

    struct ClauseCmpForRelaxing {
        bool operator()(Clause const& a, Clause const& b) const {
            int n = std::min(a.size(), b.size());
            for (int i = 0; i < n; ++i) {
                int const v = std::abs(a[i]);
                int const w = std::abs(b[i]);
                if (v != w) return v > w;
                if (a[i] != b[i]) return a[i] > b[i];
            }
            return a.size() > b.size();
        }

        bool operator()(Clause const* a, Clause const* b) const {
            return operator()(*a, *b);
        }
    };

    struct ClauseEq {
        bool operator()(Clause const& a, Clause const& b) const {
            return a == b;
        }

        bool operator()(Clause const* a, Clause const* b) const {
            return *a == *b;
        }
    };

    /**
     * Reads DIMACS CNF.
     * @param is input stream to read.
     */
    void readDimacs(std::istream& is);

    /**
     * Sorts the clauses.
     */
    void sortClauses();

    /**
     * Translates CNF information into BDDs.
     */
    void prepare();

    void makeClauseMap(Clause const* base, Clause const* const * from,
            Clause const* const * to, int k);

    /**
     * Makes rich reachability information from top to bottom.
     * @param limit the maximum BDD size.
     */
    void traverseTD(size_t limit);

    /**
     * Makes rich reachability information from bottom to top.
     */
    void traverseBU();

public:
    /**
     * Dumps the CNF in Graphviz (dot) format.
     * @param os the output stream.
     */
    void dumpCnf(std::ostream& os = std::cout, std::string title = "") const;

private:
    bool badState(std::vector<uint16_t>& clauses, int level) const;

public:
    int getRoot(State& s);
    int getChild(State& s, int level, int take);
    void destructLevel(int i);

    size_t hashCode(State const& s) const {
        return s.id->hash();
    }

    bool equalTo(State const& s1, State const& s2) const {
        return *s1.id == *s2.id;
    }

    std::ostream& printState(std::ostream& os, State const& s) const {
        return os << *s.id;
    }
};
