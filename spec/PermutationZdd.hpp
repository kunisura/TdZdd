/*
 * Top-Down ZDD Construction Library for Frontier-Based Search
 * by Hiroaki Iwashita <iwashita@erato.ist.hokudai.ac.jp>
 * Copyright (c) 2012 Japan Science and Technology Agency
 * $Id: PermutationZdd.hpp 517 2014-01-29 05:15:16Z iwashita $
 */

#pragma once

#include <cassert>
#include <iostream>

#include "../dd/DdSpec.hpp"
#include "../util/MySet.hpp"

namespace tdzdd {

class PermutationZdd: public ScalarDdSpec<PermutationZdd,MyBitSet<1> > {
    int const m;
    int const topLevel;

public:
    PermutationZdd(int m)
            : m(m), topLevel(m * m) {
        assert(1 <= m && m <= 64);
    }

    int getRoot(State& s) const {
        s.clear();
        return topLevel;
    }

    int getChild(State& s, int level, int take) const {
        if (take) {
            s.add((level - 1) % m);
            level = ((level - 1) / m) * m;
            if (level == 0) return -1;
        }
        else {
            --level;
            if (level % m == 0) return 0;
        }

        for (int k = (level - 1) % m; k >= 0; --k, --level) {
            if (!s.includes(k)) return level;
        }

        assert(false);
        return 0;
    }

    size_t hashCode(State const& s) const {
        return s.hash();
    }
};

} // namespace tdzdd
