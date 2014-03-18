/*
 * Top-Down ZDD Construction Library for Frontier-Based Search
 * by Hiroaki Iwashita <iwashita@erato.ist.hokudai.ac.jp>
 * Copyright (c) 2014 Japan Science and Technology Agency
 * $Id: ClauseZdd.hpp 518 2014-03-18 05:29:23Z iwashita $
 */

#pragma once

#include <cassert>
#include <iostream>

#include "../dd/DdSpec.hpp"
#include "../util/MyVector.hpp"

namespace tdzdd {

class ClauseZdd: public ScalarDdSpec<ClauseZdd,bool> {
    MyVector<int> clause;
    int minVar;

public:
    template<typename T>
    ClauseZdd(int n, T const& c)
            : clause(n + 1), minVar(n) {
        for (typeof(c.begin()) t = c.begin(); t != c.end(); ++t) {
            int v = std::abs(*t);
            if (v <= 0 || n < v) continue;
            int s = (*t < 0) ? -1 : 1;
            if (clause[v] * s < 0) { // tautology
                minVar = 0;
            }
            else if (minVar > v) {
                minVar = v;
            }
            clause[v] = s;
        }
    }

    int getRoot(bool& s) {
        s = (minVar == 0);
        return clause.size() - 1;
    }

    int getChild(bool& s, int level, int take) {
        assert(1 <= level && size_t(level) < clause.size());

        if (!s && clause[level] != 0 && (take != 0) == (clause[level] > 0)) {
            s = true;
        }

        if (s) return --level > 0 ? level : -1;
        if (level == minVar) return 0;

        --level;
        if (level > 0 && level == minVar && clause[level] < 0) {
            s = true;
            return --level > 0 ? level : -1;
        }
        return level;
    }
};

} // namespace tdzdd
