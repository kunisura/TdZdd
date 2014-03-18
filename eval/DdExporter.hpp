/*
 * Top-Down ZDD Construction Library for Frontier-Based Search
 * by Hiroaki Iwashita <iwashita@erato.ist.hokudai.ac.jp>
 * Copyright (c) 2013 Japan Science and Technology Agency
 * $Id: DdExporter.hpp 450 2013-07-29 01:49:19Z iwashita $
 */

#pragma once

#include "../dd/DdEval.hpp"

namespace tdzdd {

/**
 * Decision Diagram Exporter.
 * TdZdd nodes at level @a i are exported as
 * foreign DD nodes at level @a i.
 */
template<typename DD>
struct DdExporter: public DdEval<DdExporter<DD>,DD> {
    int topLevel;

public:
    DdExporter()
            : topLevel(0) {
    }

    void initialize(int topLevel) {
        this->topLevel = topLevel;
    }

    void evalTerminal(DD& f, bool val) const {
        f = DD(val);
    }

    void evalNode(DD& f, int level, DD& f0, int i0, DD& f1, int i1) const {
        f = DD(level, f0, f1);
    }
};

} // namespace tdzdd
