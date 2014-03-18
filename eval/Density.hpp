/*
 * Top-Down ZDD Construction Library for Frontier-Based Search
 * by Hiroaki Iwashita <iwashita@erato.ist.hokudai.ac.jp>
 * Copyright (c) 2014 Japan Science and Technology Agency
 * $Id: Density.hpp 518 2014-03-18 05:29:23Z iwashita $
 */

#pragma once

#include "../dd/DdEval.hpp"

namespace tdzdd {

struct Density: public DdEval<Density,double> {
    void evalTerminal(double& d, bool one) const {
        d = one ? 1.0 : 0.0;
    }

    void evalNode(double& d, int, double const& d0, int, double const& d1, int) const {
        d = (d0 + d1) / 2.0;
    }
};

} // namespace tdzdd
