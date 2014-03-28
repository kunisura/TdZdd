/*
 * TdZdd: a Top-down/Breadth-first Decision Diagram Manipulation Framework
 * by Hiroaki Iwashita <iwashita@erato.ist.hokudai.ac.jp>
 * Copyright (c) 2014 ERATO MINATO Project
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 */

#pragma once

#include <string>

#include "../DdEval.hpp"
#include "../util/BigNumber.hpp"
#include "../util/MemoryPool.hpp"
#include "../util/MyVector.hpp"

namespace tdzdd {

template<typename E, typename T, bool BDD>
class CardinalityBase: public DdEval<E,T> {
    int numVars;
    int topLevel;

public:
    CardinalityBase(int numVars = 0)
            : numVars(numVars), topLevel(0) {
    }

    void initialize(int level) {
        topLevel = level;
    }

    void evalTerminal(T& n, bool one) const {
        n = one ? 1 : 0;
    }

    template<int ARITY>
    void evalNode(T& n, int i, DdValues<T,ARITY> const& values) const {
        if (BDD) {
            n = 0;
            for (int b = 0; b < ARITY; ++b) {
                T tmp = values.get(b);
                int ii = values.getLevel(b);
                while (++ii < i) {
                    tmp *= 2;
                }
                n += tmp;
            }
        }
        else {
            n = 0;
            for (int b = 0; b < ARITY; ++b) {
                n += values.get(b);
            }
        }
    }

    T getValue(T const& n) {
        if (BDD) {
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

template<typename E, bool BDD>
class CardinalityBase<E,std::string,BDD> :
        public DdEval<E,BigNumber,std::string> {
    int numVars;
    int topLevel;
    MemoryPools pools;
    BigNumber tmp1;
    BigNumber tmp2;

public:
    CardinalityBase(int numVars = 0)
            : numVars(numVars), topLevel(0) {
    }

    void initialize(int level) {
        topLevel = level;
        pools.resize(topLevel + 1);

        size_t max = topLevel / 63 + 1;
        tmp1.setArray(pools[topLevel].allocate<uint64_t>(max));
        tmp2.setArray(pools[topLevel].allocate<uint64_t>(max));
    }

    void evalTerminal(BigNumber& n, int value) {
        n.setArray(pools[0].allocate<uint64_t>(1));
        n.store(value);
    }

    template<int ARITY>
    void evalNode(BigNumber& n, int i, DdValues<BigNumber,ARITY> const& values) {
        assert(size_t(i) <= pools.size());
        if (BDD) {
            int w = tmp1.store(0);
            for (int b = 0; b < ARITY; ++b) {
                tmp2.store(values.get(b));
                tmp2.shiftLeft(i - values.getLevel(b) - 1);
                w = tmp1.add(tmp2);
            }
            n.setArray(pools[i].allocate<uint64_t>(w));
            n.store(tmp1);
        }
        else {
            int w;
            if (ARITY <= 0) {
                w = tmp1.store(0);
            }
            else {
                w = tmp1.store(values.get(0));
                for (int b = 1; b < ARITY; ++b) {
                    w = tmp1.add(values.get(b));
                }
            }
            n.setArray(pools[i].allocate<uint64_t>(w));
            n.store(tmp1);
        }
    }

    std::string getValue(BigNumber const& n) {
        if (BDD) {
            tmp1.store(n);
            tmp1.shiftLeft(numVars - topLevel);
            return tmp1;
        }
        else {
            return n;
        }
    }

    void destructLevel(int i) {
        pools[i].clear();
    }
};

/**
 * BDD evaluator that counts the number of elements.
 * @tparam T data type for counting the number,
 *          which can be integral, real, or std::string.
 * @tparam ARITY arity of the nodes.
 */
template<typename T = std::string>
struct BddCardinality: public CardinalityBase<BddCardinality<T>,T,true> {
    BddCardinality(int numVars)
            : CardinalityBase<BddCardinality<T>,T,true>(numVars) {
    }
};

/**
 * ZDD evaluator that counts the number of elements.
 * @tparam T data type for counting the number,
 *          which can be integral, real, or std::string.
 * @tparam ARITY arity of the nodes.
 */
template<typename T = std::string>
struct ZddCardinality: public CardinalityBase<ZddCardinality<T>,T,false> {
};

} // namespace tdzdd
