/*
 * Top-Down ZDD Construction Library for Frontier-Based Search
 * by Hiroaki Iwashita <iwashita@erato.ist.hokudai.ac.jp>
 * Copyright (c) 2013 Japan Science and Technology Agency
 * $Id: ModuloZdd.hpp 517 2014-01-29 05:15:16Z iwashita $
 */

#pragma once

#include <cassert>
#include <iostream>

#include "../dd/DdSpec.hpp"

namespace tdzdd {

class ModuloZdd: public ScalarDdSpec<ModuloZdd,size_t> {
    int const n;
    size_t const modulus;
    size_t const value;

public:
    ModuloZdd(int n, size_t modulus, size_t value)
            : n(n), modulus(modulus), value(value) {
        assert(n >= 1);
        assert(modulus >= 1);
        assert(value < modulus);
    }

    int getRoot(size_t& state) const {
        state = 0;
        return n;
    }

    int getChild(size_t& state, int level, int take) const {
        if (take && ++state == modulus) state = 0;
        --level;
        return (level == 0 && state == value) ? -1 : level;
    }
};

} // namespace tdzdd
