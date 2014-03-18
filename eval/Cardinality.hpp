/*
 * Top-Down ZDD Construction Library for Frontier-Based Search
 * by Hiroaki Iwashita <iwashita@erato.ist.hokudai.ac.jp>
 * Copyright (c) 2012 Japan Science and Technology Agency
 * $Id: Cardinality.hpp 518 2014-03-18 05:29:23Z iwashita $
 */

#pragma once

#include <cassert>
#include <string>

#include "../dd/DdEval.hpp"
#include "../util/BigNumber.hpp"
#include "../util/MemoryPool.hpp"
#include "../util/MyVector.hpp"

namespace tdzdd {

template<typename T = std::string>
class Cardinality: public DdEval<Cardinality<T>,T> {
    int numVars;
    int topLevel;

public:
    Cardinality(int numVars = 0)
            : numVars(numVars), topLevel(0) {
    }

    void initialize(int level) {
        topLevel = level;
    }

    void evalTerminal(T& n, bool one) const {
        n = one ? 1 : 0;
    }

    void evalNode(T& n, int i, T const& n0, int i0, T const& n1, int i1) const {
        if (numVars > 0) {
            T tmp0 = n0;
            T tmp1 = n1;
            while (++i0 < i) {
                tmp0 *= 2;
            }
            while (++i1 < i) {
                tmp1 *= 2;
            }
            n = tmp0 + tmp1;
        }
        else {
            n = n0 + n1;
        }
    }

    T getValue(T const& n) {
        if (numVars > 0) {
            T tmp = n;
            for (int i = topLevel; i < numVars; ++i) {
                tmp *= 2;
            }
            return tmp;
        }
        else {
            return n;
        }
    }
};

template<>
class Cardinality<std::string> : public DdEval<Cardinality<std::string>,
        BigNumber,std::string> {
    int numVars;
    int topLevel;
    int numberSize;
    MemoryPools pools;
    MyVector<uint64_t> work0;
    MyVector<uint64_t> work1;
    BigNumber tmp0;
    BigNumber tmp1;

public:
    Cardinality(int numVars = 0)
            : numVars(numVars), topLevel(0), numberSize(2) {
        size_t size = (numVars + 62) / 63;
        work0.resize(size);
        work1.resize(size);
        tmp0.setArray(work0.data());
        tmp1.setArray(work1.data());
    }

    void initialize(int level) {
        topLevel = level;
        pools.resize(topLevel + 1);
    }

    void evalTerminal(BigNumber& n, bool one) {
        n.setArray(pools[0].allocate<uint64_t>(1));
        n.store(one ? 1 : 0);
    }

    void evalNode(BigNumber& n, int i, BigNumber const& n0, int i0,
            BigNumber const& n1, int i1) {
        assert(numberSize >= 1);
        assert(size_t(i) <= pools.size());
        assert(i0 < i && i1 < i);
        int w;
        if (numVars > 0) {
            tmp0.store(n0);
            tmp0.shiftLeft(i - i0 - 1);
            tmp1.store(n1);
            tmp1.shiftLeft(i - i1 - 1);
            w = tmp0.add(tmp1);
            n.setArray(pools[i].allocate<uint64_t>(w));
            n.store(tmp0);
        }
        else {
            n.setArray(pools[i].allocate<uint64_t>(numberSize));
            n.store(n0);
            w = n.add(n1);
        }
        if (numberSize <= w) numberSize = w + 1;
    }

    std::string getValue(BigNumber const& n) {
        if (numVars > 0) {
            tmp0.store(n);
            tmp0.shiftLeft(numVars - topLevel);
            return std::string(tmp0);
        }
        else {
            return std::string(n);
        }
    }

    void destructLevel(int i) {
        pools[i].clear();
    }
};

} // namespace tdzdd
