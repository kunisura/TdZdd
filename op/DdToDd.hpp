/*
 * Top-Down ZDD Construction Library for Frontier-Based Search
 * by Hiroaki Iwashita <iwashita@erato.ist.hokudai.ac.jp>
 * Copyright (c) 2014 Japan Science and Technology Agency
 * $Id: DdToDd.hpp 518 2014-03-18 05:29:23Z iwashita $
 */

#pragma once

#include <cassert>
#include <iostream>

#include "../dd/DdSpec.hpp"
#include "../util/MyVector.hpp"

namespace tdzdd {

template<typename T, typename S>
class DdToDd: public PodArrayDdSpec<T,size_t> {
protected:
    typedef S Spec;
    typedef size_t Word;

    static size_t const levelWords = (sizeof(int) + sizeof(Word) - 1)
            / sizeof(Word);

    Spec spec;
    int const stateWords;

    static int wordSize(int size) {
        return (size + sizeof(Word) - 1) / sizeof(Word);
    }

    int& level(void* p) const {
        return *static_cast<int*>(p);
    }

    int level(void const* p) const {
        return *static_cast<int const*>(p);
    }

    void* state(void* p) const {
        return static_cast<Word*>(p) + levelWords;
    }

    void const* state(void const* p) const {
        return static_cast<Word const*>(p) + levelWords;
    }

public:
    DdToDd(S const& s)
            : spec(s), stateWords(wordSize(spec.datasize())) {
        DdToDd::setArraySize(levelWords + stateWords);
    }

    void get_copy(void* to, void const* from) {
        level(to) = level(from);
        spec.get_copy(state(to), state(from));
    }

    void destruct(void* p) {
        spec.destruct(state(p));
    }

    void destructLevel(int level) {
        spec.destructLevel(level);
    }

    size_t hash_code(void const* p, int i) const {
        size_t h = size_t(level(p)) * 314159257;
        if (level(p) > 0) h += spec.hash_code(state(p), level(p)) * 271828171;
        return h;
    }

    bool equal_to(void const* p, void const* q, int i) const {
        if (level(p) != level(q)) return false;
        if (level(p) > 0 && !spec.equal_to(state(p), state(q), level(p))) return false;
        return true;
    }

    void print_state(std::ostream& os, void const* p) const {
        Word const* q = static_cast<Word const*>(p);
        os << "<" << level(q) << ",";
        spec.print_state(os, state(q));
        os << ">";
    }
};

template<typename S>
class BddToZdd: public DdToDd<BddToZdd<S>,S> {
    typedef DdToDd<BddToZdd,S> base;
    typedef typename base::Word Word;

    int numVars;
    MyVector<bool> isHiddenVar;

public:
    template<typename Vars>
    BddToZdd(S const& s, Vars const& hiddenVars)
            : base(s) {
        numVars = 0;
        for (typeof(hiddenVars.begin()) t = hiddenVars.begin();
                t != hiddenVars.end(); ++t) {
            numVars = std::max(numVars, *t);
        }
        isHiddenVar.resize(numVars + 1);
        for (typeof(hiddenVars.begin()) t = hiddenVars.begin();
                t != hiddenVars.end(); ++t) {
            isHiddenVar[*t] = true;
        }
    }

    int getRoot(Word* p) {
        level(p) = base::spec.get_root(state(p));
        if (level(p) == 0) return 0;
        if (level(p) >= numVars) {
            numVars = level(p);
            isHiddenVar.resize(numVars + 1);
        }
        return (numVars > 0) ? numVars : -1;
    }

    int getChild(Word* p, int i, int take) {
        if (level(p) == i) {
            level(p) = base::spec.get_child(state(p), i, take);
            if (level(p) == 0) return 0;
        }

        do {
            --i;
        } while (level(p) < i && 0 < i && !isHiddenVar[i]);
        assert(level(p) <= i);
        return (i > 0) ? i : level(p);
    }
};

template<typename S>
class ZddToBdd: public DdToDd<ZddToBdd<S>,S> {
    typedef DdToDd<ZddToBdd,S> base;
    typedef typename base::Word Word;

    int const numVars;
    MyVector<bool> isHiddenVar;

public:
    template<typename Vars>
    ZddToBdd(S const& s, Vars const& hiddenVars)
            : base(s) {
        numVars = 0;
        for (typeof(hiddenVars.begin()) t = hiddenVars.begin();
                t != hiddenVars.end(); ++t) {
            numVars = std::max(numVars, *t);
        }
        isHiddenVar.resize(numVars + 1);
        for (typeof(hiddenVars.begin()) t = hiddenVars.begin();
                t != hiddenVars.end(); ++t) {
            isHiddenVar[*t] = true;
        }
    }

    int getRoot(Word* p) {
        level(p) = base::spec.get_root(state(p));
        if (level(p) == 0) return 0;
        if (level(p) >= numVars) {
            numVars = level(p);
            isHiddenVar.resize(numVars + 1);
        }
        return numVars;
    }

    int getChild(Word* p, int i, int take) {
        if (level(p) == i) {
            level(p) = base::spec.get_child(state(p), i, take);
            if (level(p) == 0) return 0;
        }
        else if (take) {
            return 0;
        }

        do {
            --i;
        } while (level(p) < i && 0 < i && !isHiddenVar[i]);
        assert(level(p) <= i);
        return (i > 0) ? i : level(p);
    }
};

template<typename S, typename Vars>
BddToZdd<S> bdd2zdd(S const& spec, Vars const& hiddenVars) {
    return BddToZdd<S>(spec, hiddenVars);
}

template<typename S, typename Vars>
ZddToBdd<S> zdd2bdd(S const& spec, Vars const& hiddenVars) {
    return ZddToBdd<S>(spec, hiddenVars);
}

} // namespace tdzdd
