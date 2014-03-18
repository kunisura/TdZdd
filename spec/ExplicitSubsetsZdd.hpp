/*
 * Top-Down ZDD Construction Library for Frontier-Based Search
 * by Hiroaki Iwashita <iwashita@erato.ist.hokudai.ac.jp>
 * Copyright (c) 2013 Japan Science and Technology Agency
 * $Id: ExplicitSubsetsZdd.hpp 517 2014-01-29 05:15:16Z iwashita $
 */

#pragma once

#include "../dd/DdSpec.hpp"
#include "../util/ExplicitSubsets.hpp"

namespace tdzdd {

struct ExplicitSubsetsZddState {
    int bitpos;
    size_t first;
    size_t last;
    size_t bound;
};

class ExplicitSubsetsZdd: public ScalarDdSpec<ExplicitSubsetsZdd,
        ExplicitSubsetsZddState> {
    ExplicitSubsets const& itemsets;
    int const numVars;

    int levOfBit(int bitpos) const {
        assert(0 <= bitpos && bitpos < numVars);
        return numVars - bitpos;
    }

    int bitAtLev(int level) const {
        assert(0 < level && level <= numVars);
        return numVars - level;
    }

    size_t searchBoundary(int bitpos, size_t first, size_t last) const {
        assert(first < last);
        while (last - first >= 2) {
            size_t mid = (last - first) / 2 + first;
            (itemsets.element(mid).get(bitpos) ? last : first) = mid;
        }
        return itemsets.element(first).get(bitpos) ? first : last;
    }

    int goDown(ExplicitSubsetsZddState& s) const {
        if (s.first == s.last) return 0;
        while (s.bitpos < numVars) {
            s.bound = searchBoundary(s.bitpos, s.first, s.last);
            if (s.bound < s.last) return levOfBit(s.bitpos);
            ++s.bitpos;
        }
        return -1;
    }

public:
    ExplicitSubsetsZdd(ExplicitSubsets& itemsets)
            : itemsets(itemsets.sortAndUnique()), numVars(itemsets.vectorBits()) {
    }

    struct Mapper {
        int n;

    public:
        Mapper(int n = 0)
                : n(n) {
        }

        int operator()(int level) const {
            return n - level;
        }

        int operator()(NodeId f) const {
            return n - f.row();
        }
    };

    /**
     * Gets a function from levels to item numbers.
     * @return function.
     */
    Mapper mapper() const {
        return Mapper(numVars);
    }

    int getRoot(ExplicitSubsetsZddState& s) const {
        if (itemsets.size() == 0) return 0;
        if (itemsets.vectorBits() == 0) return -1;
        s.bitpos = 0;
        s.first = 0;
        s.last = itemsets.size();
        return goDown(s);
    }

    int getChild(ExplicitSubsetsZddState& s, int level, int take) {
        assert(s.bitpos == bitAtLev(level));
        assert(s.first <= s.bound && s.bound < s.last);
        ++s.bitpos;
        (take ? s.first : s.last) = s.bound;
        return goDown(s);
    }

    size_t hashCode(ExplicitSubsetsZddState const& s) const {
        size_t h = s.bitpos * 271828171;
        for (size_t i = s.first; i < s.last; ++i) {
            h += itemsets.element(i).hash(s.bitpos);
            h *= 314159257;
        }
        return h;
    }

    bool equalTo(ExplicitSubsetsZddState const& s1,
            ExplicitSubsetsZddState const& s2) const {
        if (s1.last - s1.first != s2.last - s2.first) return false;
        if (s1.bitpos != s2.bitpos) return false;
        for (size_t i1 = s1.first, i2 = s2.first; i1 < s1.last; ++i1, ++i2) {
            if (!itemsets.element(i1).equal(itemsets.element(i2), s1.bitpos)) return false;
        }
        return true;
    }

    std::ostream& printState(std::ostream& os,
            ExplicitSubsetsZddState const& s) const {
        return os << "[" << s.first << "," << (s.last - 1) << "]";
    }
};

} // namespace tdzdd
