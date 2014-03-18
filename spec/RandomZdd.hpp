/*
 * Top-Down ZDD Construction Library for Frontier-Based Search
 * by Hiroaki Iwashita <iwashita@erato.ist.hokudai.ac.jp>
 * Copyright (c) 2013 Japan Science and Technology Agency
 * $Id: RandomZdd.hpp 517 2014-01-29 05:15:16Z iwashita $
 */

#pragma once

#include <cassert>
#include <iostream>
#include <cstdlib>

#include "../dd/DdSpec.hpp"

namespace tdzdd {

class RandomZdd: public ScalarDdSpec<RandomZdd,size_t> {
    int const n;
    size_t const width;
    int const drop;

public:
    RandomZdd(int n, size_t width, double dropRatio)
            : n(n), width(width), drop(dropRatio * (double(RAND_MAX) + 1.0)) {
        assert(n >= 1);
        assert(width >= 1);
        assert(-1 <= drop && drop <= RAND_MAX);
    }

    int getRoot(size_t& state) const {
        state = 0;
        return n;
    }

    int getChild(size_t& state, int level, int take) const {
        if (level <= 1) return (std::rand() & 1) ? -1 : 0;
        if (std::rand() < drop) return 0;

        state *= (1ULL << 22) + 15;
        state += std::rand();
        state *= (1ULL << 22) + 15;
        state += std::rand();
        state *= (1ULL << 22) + 15;
        state += std::rand();
        state %= width;
        return level - 1;
    }
};

} // namespace tdzdd
