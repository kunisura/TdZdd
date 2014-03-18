/*
 * Top-Down ZDD Construction Library for Frontier-Based Search
 * by Hiroaki Iwashita <iwashita@erato.ist.hokudai.ac.jp>
 * Copyright (c) 2012 Japan Science and Technology Agency
 * $Id: ToZBDD.hpp 495 2013-11-08 11:12:55Z iwashita $
 */

#pragma once

#ifndef B_64
#if __SIZEOF_POINTER__ == 8
#define B_64
#endif
#endif
#include <ZBDD.h>

#include <vector>

#include "../dd/DdEval.hpp"

namespace tdzdd {

/**
 * Exporter to ZBDD.
 * TdZdd nodes at level @a i are converted to
 * ZBDD nodes at level @a i + @p offset.
 * When the ZBDD variables are not enough, they are
 * created automatically by BDD_NewVar().
 */
struct ToZBDD: public DdEval<ToZBDD,ZBDD> {
    int const offset;

public:
    ToZBDD(int offset = 0)
            : offset(offset) {
    }

    void initialize(int topLevel) const {
        while (BDD_VarUsed() < topLevel + offset) {
            BDD_NewVar();
        }
    }

    void evalTerminal(ZBDD& f, bool one) const {
        f = ZBDD(one ? 1 : 0);
    }

    void evalNode(ZBDD& f, int level, ZBDD& f0, int i0, ZBDD& f1,
            int i1) const {
        if (level + offset <= 0) {
            f = f0;
        }
        else {
#ifdef _OPENMP
#pragma omp critical
#endif
            f = f0 + f1.Change(BDD_VarOfLev(level + offset));
        }
    }
};

} // namespace tdzdd
