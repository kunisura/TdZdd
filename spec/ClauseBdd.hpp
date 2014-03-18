/*
 * Top-Down ZDD Construction Library for Frontier-Based Search
 * by Hiroaki Iwashita <iwashita@erato.ist.hokudai.ac.jp>
 * Copyright (c) 2014 Japan Science and Technology Agency
 * $Id: ClauseBdd.hpp 518 2014-03-18 05:29:23Z iwashita $
 */

#pragma once

#include <cassert>
#include <iostream>

#include "../dd/DdSpec.hpp"
#include "../util/MyVector.hpp"

namespace tdzdd {

class ClauseBdd: public StatelessDdSpec<ClauseBdd> {
    MyVector<int> clause;
    int cursor;

    struct CompareVars {
        bool& tautology;

        CompareVars(bool& tautology)
                : tautology(tautology) {
        }

        bool operator()(int a, int b) {
            if (a == -b) tautology = true;
            return std::abs(a) < std::abs(b);
        }
    };

    void sortLiterals() {
        if (clause.empty()) return;
        bool tautology = false;
        std::sort(clause.begin(), clause.end(), CompareVars(tautology));
        if (tautology) {
            clause.clear();
        }
        else {
            int k = std::unique(clause.begin(), clause.end()) - clause.begin();
            clause.resize(k);
        }
    }

public:
    template<typename T>
    ClauseBdd(T const& clause)
            : clause(clause), cursor(0) {
        sortLiterals();
    }

    int getRoot() {
        cursor = clause.size() - 1;
        if (cursor < 0) return -1;
        return std::abs(clause[cursor]);
    }

    int getChild(int level, int take) {
        assert(level >= 1);
        assert(cursor >= 0);

        int i = std::abs(clause[cursor]);

        if (i < level) {
            cursor = clause.size() - 1;
            i = std::abs(clause[cursor]);
        }

        while (i > level) {
            if (--cursor < 0) return 0;
            i = std::abs(clause[cursor]);
        }

        if (i == level) {
            if (clause[cursor] > 0 ? take : !take) return -1;
            if (cursor == 0) return 0;
            i = std::abs(clause[cursor - 1]);
        }

        assert(1 <= i && i < level);
        return i;
    }
};

} // namespace tdzdd
