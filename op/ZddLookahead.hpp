/*
 * Top-Down ZDD Construction Library for Frontier-Based Search
 * by Hiroaki Iwashita <iwashita@erato.ist.hokudai.ac.jp>
 * Copyright (c) 2012 Japan Science and Technology Agency
 * $Id: ZddLookahead.hpp 518 2014-03-18 05:29:23Z iwashita $
 */

#pragma once

#include <cassert>
#include <iostream>
#include <vector>

#include "../dd/DdSpec.hpp"

namespace tdzdd {

template<typename S>
class ZddLookahead: public DdSpec<ZddLookahead<S>,S::ARITY> {
    typedef S Spec;

    Spec spec;
    std::vector<char> work;

public:
    ZddLookahead(S const& s)
            : spec(s), work(spec.datasize()) {
    }

    int datasize() const {
        return spec.datasize();
    }

    int get_root(void* p) {
        return spec.get_root(p);
    }

    int get_child(void* p, int level, int b) {
        level = spec.get_child(p, level, b);

        while (level >= 1) {
            for (int b = 1; b < Spec::ARITY; ++b) {
                spec.get_copy(work.data(), p);
                if (spec.get_child(work.data(), level, b) != 0) {
                    spec.destruct(work.data());
                    return level;
                }
                spec.destruct(work.data());
            }
            level = spec.get_child(p, level, false);
        }

        return level;
    }

    void get_copy(void* to, void const* from) {
        spec.get_copy(to, from);
    }

    void destruct(void* p) {
        spec.destruct(p);
    }

    void destructLevel(int level) {
        spec.destructLevel(level);
    }

    bool needWipedown(int level) {
        return false;
    }

    void set_wipedown_root(void* p, int level) {
    }

    void doWipedown(int level) {
    }

    size_t hash_code(void const* p, int level) const {
        return spec.hash_code(p, level);
    }

    bool equal_to(void const* p, void const* q, int level) const {
        return spec.equal_to(p, q, level);
    }
};

template<typename S>
ZddLookahead<S> zddLookahead(S const& spec) {
    return ZddLookahead<S>(spec);
}

} // namespace tdzdd
