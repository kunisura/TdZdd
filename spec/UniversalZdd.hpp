/*
 * Top-Down ZDD Construction Library for Frontier-Based Search
 * by Hiroaki Iwashita <iwashita@erato.ist.hokudai.ac.jp>
 * Copyright (c) 2012 Japan Science and Technology Agency
 * $Id: UniversalZdd.hpp 517 2014-01-29 05:15:16Z iwashita $
 */

#pragma once

#include <cassert>
#include <iostream>

#include "../dd/DdSpec.hpp"

namespace tdzdd {

class UniversalZdd: public StatelessDdSpec<UniversalZdd> {
    int const n;

public:
    UniversalZdd(int n)
            : n(n) {
    }

    int getRoot() const {
        return n;
    }

    int getChild(int level, int b) const {
        --level;
        return (level >= 1) ? level : -1;
    }
};

} // namespace tdzdd
