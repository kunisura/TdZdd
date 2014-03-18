/*
 * Top-Down ZDD Construction Library for Frontier-Based Search
 * by Hiroaki Iwashita <iwashita@erato.ist.hokudai.ac.jp>
 * Copyright (c) 2012 Japan Science and Technology Agency
 * $Id: ZddInterleave.hpp 518 2014-03-18 05:29:23Z iwashita $
 */

#pragma once

#include <cassert>
#include <iostream>
#include <vector>

#include "../dd/DdSpec.hpp"

namespace tdzdd {

class ZddInterleave: public PodArrayDdSpec<ZddInterleave,NodeId> {
    std::vector<DdStructure const*> dds;
    int const n;
    DdStructure const dontCare;

    int getLevel(NodeId f, int k) const {
        int i = f.row();
        return (i == 0) ? -f.col() : (i - 1) * n + k + 1;
    }

    int getLevel(NodeId* a) const {
        int level = 0;
        for (int k = 0; k < n; ++k) {
            int i = getLevel(a[k], k);
            if (i == 0) return 0;
            level = std::max(level, i);
        }
        return level == 0 ? -1 : level;
    }

public:
    ZddInterleave(DdStructure const& dd, int n)
            : dds(n), n(n) {
        setArraySize(n);
        for (int k = 0; k < n; ++k) {
            dds[k] = &dd;
        }
    }

    template<typename C>
    ZddInterleave(C const& c)
            : dds(), n(c.size()) {
        setArraySize(n);
        dds.reserve(n);
        for (typename C::const_iterator t = c.begin(); t != c.end(); ++t) {
            dds.push_back(&*t);
        }
    }

    ZddInterleave(DdStructure const& dd, int n, int pos)
            : dds(n), n(n), dontCare(dd.topLevel()) {
        setArraySize(n);
        for (int k = 0; k < n; ++k) {
            dds[k] = (k == pos) ? &dd : &dontCare;
        }
    }

    int getRoot(NodeId* a) const {
        for (int k = 0; k < n; ++k) {
            a[k] = dds[k]->root();
        }
        return getLevel(a);
    }

    int getChild(NodeId* a, int level, int b) const {
        int i = (level - 1) / n + 1;
        int k = (level - 1) % n;
        if (a[k].row() == i) {
            a[k] = dds[k]->child(a[k], b);
        }
        else if (b) {
            a[k] = 0;
        }
        return getLevel(a);
    }
};

} // namespace tdzdd
