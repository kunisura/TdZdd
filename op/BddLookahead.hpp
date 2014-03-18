/*
 * Top-Down ZDD Construction Library for Frontier-Based Search
 * by Hiroaki Iwashita <iwashita@erato.ist.hokudai.ac.jp>
 * Copyright (c) 2012 Japan Science and Technology Agency
 * $Id: BddLookahead.hpp 518 2014-03-18 05:29:23Z iwashita $
 */

#pragma once

#include <cassert>
#include <iostream>
#include <vector>

#include "../dd/DdSpec.hpp"

namespace tdzdd {

template<typename S>
class BddLookahead: public DdSpec<BddLookahead<S>,S::ARITY> {
    typedef S Spec;

    Spec spec;
    std::vector<char> work0;
    std::vector<char> work1;

public:
    BddLookahead(S const& s)
            : spec(s), work0(spec.datasize()), work1(spec.datasize()) {
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
            spec.get_copy(work0.data(), p);
            int level0 = spec.get_child(work0.data(), level, 0);

            for (int b = 1; b < Spec::ARITY; ++b) {
                spec.get_copy(work1.data(), p);
                int level1 = spec.get_child(work1.data(), level, b);
                if (!(level0 == level1
                        && (level0 <= 0
                                || spec.equal_to(work0.data(), work1.data(),
                                        level0)))) {
                    spec.destruct(work0.data());
                    spec.destruct(work1.data());
                    return level;
                }
                spec.destruct(work1.data());
            }

            spec.destruct(p);
            spec.get_copy(p, work0.data());
            spec.destruct(work0.data());
            level = level0;
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
BddLookahead<S> bddLookahead(S const& spec) {
    return BddLookahead<S>(spec);
}

} // namespace tdzdd
