/*
 * Top-Down ZDD Construction Library for Frontier-Based Search
 * by Hiroaki Iwashita <iwashita@erato.ist.hokudai.ac.jp>
 * Copyright (c) 2012 Japan Science and Technology Agency
 * $Id: MinNumItems.hpp 450 2013-07-29 01:49:19Z iwashita $
 */

#pragma once

#include <climits>

#include "../dd/DdEval.hpp"

namespace tdzdd {

struct MinNumItems: public DdEval<MinNumItems,int> {
    void evalTerminal(int& n, bool one) const {
        n = one ? 0 : INT_MAX;
    }

    void evalNode(int& n, int, int n0, int, int n1, int) const {
        n = std::min(n0, n1 + 1);
    }
};

} // namespace tdzdd
