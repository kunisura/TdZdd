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

#ifdef __SIZEOF_INT__
#undef SIZEOF_INT
#define SIZEOF_INT __SIZEOF_INT__
#endif

#ifdef __SIZEOF_LONG__
#undef SIZEOF_LONG
#define SIZEOF_LONG __SIZEOF_LONG__
#endif

#ifdef __SIZEOF_POINTER__
#undef SIZEOF_VOID_P
#define SIZEOF_VOID_P __SIZEOF_POINTER__
#endif

#include <cassert>
#include <cstdio>
#include <stdexcept>

#include <cudd.h>

#include <tdzdd/DdSpec.hpp>

/**
 * CUDD wrapper.
 * CAUTION: Do not use in parallel. CUDD is not thread-safe.
 */
template<int N>
class Cudd_: public tdzdd::DdSpec<Cudd_<N>,Cudd_<N>,2> {
    DdNode* dd;

    static DdNode* varAtLevel(int level) {
        while (level >= Cudd_ReadSize(manager)) {
            if (Cudd_bddNewVarAtLevel(manager, 0) == 0) throw std::runtime_error(
                    "CUDD returned NULL");
        }
        DdNode* v = Cudd_bddIthVar(manager, level);
        if (v == 0) throw std::runtime_error("CUDD returned NULL");
        return v;
    }

public:
    static DdManager* manager;

    Cudd_()
            : dd(0) {
    }

    Cudd_(int val)
            : dd(val ? Cudd_ReadOne(manager) : Cudd_ReadLogicZero(manager)) {
        if (dd == 0) throw std::runtime_error("CUDD returned NULL");
        Cudd_Ref(dd);
    }

    Cudd_(int level, Cudd_ const& f0, Cudd_ const& f1)
            : dd(Cudd_bddIte(manager, varAtLevel(level), f1.dd, f0.dd)) {
        if (dd == 0) throw std::runtime_error("CUDD returned NULL");
        Cudd_Ref(dd);
    }

    Cudd_(DdNode* dd)
            : dd(dd) {
        if (dd != 0) Cudd_Ref(dd);
    }

    Cudd_(Cudd_ const& o)
            : dd(o.dd) {
        if (dd != 0) Cudd_Ref(dd);
    }

    Cudd_& operator=(Cudd_ const& o) {
        Cudd_Ref(o.dd);
        this->~Cudd_();
        dd = o.dd;
        return *this;
    }

    ~Cudd_() {
        if (dd != 0) Cudd_RecursiveDeref(manager, dd);
    }

    DdNode* ddNode() const {
        return dd;
    }

    static size_t peakLiveNodeCount() {
        return Cudd_ReadPeakLiveNodeCount(manager);
    }

    size_t size() const {
        return Cudd_DagSize(dd);
    }

    template<typename C>
    static size_t sharingSize(C const& vec) {
        size_t n = vec.size();
        tdzdd::MyVector<DdNode*> nodeVec(n);
        for (size_t i = 0; i < n; ++i) {
            nodeVec[i] = vec[i].dd;
        }
        return Cudd_SharingSize(nodeVec.data(), n);
    }

    double countMinterm(int nvars) const {
        return Cudd_CountMinterm(manager, dd, nvars);
    }

    bool isNull() const {
        return dd == 0;
    }

    bool isConstant() const {
        return Cudd_IsConstant(dd);
    }

    int level() const {
        if (Cudd_IsConstant(dd)) return 0;
        return Cudd_NodeReadIndex(dd);
    }

    Cudd_ child(int b) const {
        return Cudd_NotCond(b ? Cudd_T(dd) : Cudd_E(dd), Cudd_IsComplement(dd)) ;
    }

    size_t hash() const {
        return reinterpret_cast<size_t>(dd) * 314159257;
    }

    bool operator==(Cudd_ const& f) const {
        return dd == f.dd;
    }

    bool operator!=(Cudd_ const& f) const {
        return dd != f.dd;
    }

    bool operator==(int val) const {
        return dd == (val ? Cudd_ReadOne(manager) : Cudd_ReadLogicZero(manager));
    }

    bool operator!=(int val) const {
        return dd != (val ? Cudd_ReadOne(manager) : Cudd_ReadLogicZero(manager));
    }

    bool operator<(Cudd_ const& f) const {
        return dd < f.dd;
    }

    bool dependsOn(Cudd_ const& var) const {
        if (var.isConstant()) return false;
        return Cudd_bddVarIsDependent(manager, dd, var.dd);
    }

    bool contains(Cudd_ const& f) const {
        return Cudd_bddLeq(manager, f.dd, dd);
    }

    bool intersects(Cudd_ const& f) const {
        return !Cudd_bddLeq(manager, dd, Cudd_Not(f.dd) );
    }

    Cudd_ operator~() const {
        return Cudd_Not(dd) ;
    }

    Cudd_ operator&(Cudd_ const& f) const {
        return Cudd_bddAnd(manager, dd, f.dd);
    }

    Cudd_& operator&=(Cudd_ const& f) {
        return *this = *this & f;
    }

    Cudd_ operator|(Cudd_ const& f) const {
        return Cudd_bddOr(manager, dd, f.dd);
    }

    Cudd_& operator|=(Cudd_ const& f) {
        return *this = *this | f;
    }

    Cudd_ operator^(Cudd_ const& f) const {
        return Cudd_bddXor(manager, dd, f.dd);
    }

    Cudd_& operator^=(Cudd_ const& f) {
        return *this = *this ^ f;
    }

    Cudd_ ite(Cudd_ const& ft, Cudd_ const& fe) const {
        return Cudd_bddIte(manager, dd, ft.dd, fe.dd);
    }

    Cudd_ support() const {
        return Cudd_Support(manager, dd);
    }

    Cudd_ abstract(Cudd_ const& cube) const {
        if (cube.isConstant()) return *this;
        return Cudd_bddExistAbstract(manager, dd, cube.dd);
    }

    Cudd_ andAbstract(Cudd_ const& f, Cudd_ const& cube) const {
        if (cube.isConstant()) return *this & f;
        return Cudd_bddAndAbstract(manager, dd, f.dd, cube.dd);
    }

    Cudd_ cofactor(Cudd_ const& c) const {
        return Cudd_bddConstrain(manager, dd, c.dd);
    }

    Cudd_ minimize(Cudd_ const& c) const {
        return Cudd_bddMinimize(manager, dd, c.dd);
    }

    template<typename T>
    Cudd_ compose(T const& vector) const {
        tdzdd::MyVector<DdNode*> v(Cudd_ReadSize(manager));

        for (size_t i = 0; i < v.size(); ++i) {
            if (i < vector.size() && !vector[i].isNull()) {
                v[i] = vector[i].dd;
            }
            else {
                v[i] = Cudd_bddIthVar(manager, i);
            }
        }

        return Cudd_bddVectorCompose(manager, dd, v.data());
    }

    static int reorder(Cudd_ReorderingType heuristic = CUDD_REORDER_SIFT,
            int minsize = 0) {
        return Cudd_ReduceHeap(manager, heuristic, minsize);
    }

    Cudd_ abstract1(Cudd_ const& cube) const {
        tdzdd::MyHashMap<Cudd_,Cudd_> cache(size() * 2);
        return abstract1_step(cache, cube);
    }

private:
    Cudd_ abstract1_step(tdzdd::MyHashMap<Cudd_,Cudd_>& cache,
            Cudd_ cube) const {
        int i = level();
        if (i < 1) return *this;

        Cudd_& f = cache[*this];
        if (!f.isNull()) return f;

        while (cube.level() > i) {
            cube = cube.child(1);
        }

        Cudd_ f0 = child(0).abstract1_step(cache, cube);
        Cudd_ f1 = child(1).abstract1_step(cache, cube);
        if (cube.level() == i) f1 |= f0;

        f = Cudd_(i, f0, f1);
        return f;
    }

public:
    int getRoot(Cudd_& f) {
        f = *this;
        return (f == 1) ? -1 : f.level();
    }

    int getChild(Cudd_& f, int level, int take) {
        DdNode* v = varAtLevel(level);
        f = Cudd_Cofactor(manager, f.dd, take ? v : Cudd_Not(v) );
        assert(level - Cudd_NodeReadIndex(f.dd) > 0);
        return (f == 1) ? -1 : f.level();
    }

    size_t hashCode(Cudd_ const& f) const {
        return f.hash();
    }

    void printState(std::ostream& os, Cudd_ const& f) const {
        os << f.level();
    }

    friend std::ostream& operator<<(std::ostream& os, Cudd_ const& f) {
        return os << "BDD(" << f.dd << ")";
    }
};

template<int N>
DdManager* Cudd_<N>::manager = Cudd_Init(0, 0, CUDD_UNIQUE_SLOTS,
                                         CUDD_CACHE_SLOTS, 0);

typedef Cudd_<0> Cudd;
