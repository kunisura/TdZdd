/*
 * CUDD Wrapper
 * Hiroaki Iwashita <iwashita@erato.ist.hokudai.ac.jp>
 * Copyright (c) 2014 Japan Science and Technology Agency
 * $Id: CuddBdd.hpp 518 2014-03-18 05:29:23Z iwashita $
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

#include "../dd/DdSpec.hpp"
#include "../util/MyHashTable.hpp"
#include "../util/MyVector.hpp"

namespace tdzdd {

template<int N>
class CuddBdd_: public ScalarDdSpec<CuddBdd_<N>,CuddBdd_<N> > {
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

//    static int index2level(int index) {
//        return Cudd_ReadZddSize(manager) - Cudd_ReadPermZdd(manager, index);
//    }
//
//    static int level2index(int level) {
//        return Cudd_ReadInvPermZdd(manager, Cudd_ReadZddSize(manager) - level);
//    }

    CuddBdd_()
            : dd(0) {
    }

    CuddBdd_(int val)
            : dd(val ? Cudd_ReadOne(manager) : Cudd_ReadLogicZero(manager)) {
        if (dd == 0) throw std::runtime_error("CUDD returned NULL");
        Cudd_Ref(dd);
    }

    CuddBdd_(int level, CuddBdd_ const& f0, CuddBdd_ const& f1)
            : dd(Cudd_bddIte(manager, varAtLevel(level), f1.dd, f0.dd)) {
        if (dd == 0) throw std::runtime_error("CUDD returned NULL");
        Cudd_Ref(dd);
    }

    CuddBdd_(DdNode* dd)
            : dd(dd) {
        if (dd != 0) Cudd_Ref(dd);
    }

    CuddBdd_(CuddBdd_ const& o)
            : dd(o.dd) {
        if (dd != 0) Cudd_Ref(dd);
    }

    CuddBdd_& operator=(CuddBdd_ const& o) {
        Cudd_Ref(o.dd);
        this->~CuddBdd_();
        dd = o.dd;
        return *this;
    }

    ~CuddBdd_() {
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
        MyVector<DdNode*> nodeVec(n);
        for (size_t i = 0; i < n; ++i) {
            nodeVec[i] = vec[i].dd;
        }
        return Cudd_SharingSize(nodeVec.data(), n);
    }

    double countMinterm(int nvars) const {
        return Cudd_CountMinterm(manager, dd, nvars);
    }

//    int index() const {
//        return Cudd_NodeReadIndex(dd);
//    }

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

    CuddBdd_ child(int b) const {
        return Cudd_NotCond(b ? Cudd_T(dd) : Cudd_E(dd), Cudd_IsComplement(dd)) ;
    }

    size_t hash() const {
        return reinterpret_cast<size_t>(dd) * 314159257;
    }

    bool operator==(CuddBdd_ const& f) const {
        return dd == f.dd;
    }

    bool operator!=(CuddBdd_ const& f) const {
        return dd != f.dd;
    }

    bool operator==(int val) const {
        return dd == (val ? Cudd_ReadOne(manager) : Cudd_ReadLogicZero(manager));
    }

    bool operator!=(int val) const {
        return dd != (val ? Cudd_ReadOne(manager) : Cudd_ReadLogicZero(manager));
    }

    bool operator<(CuddBdd_ const& f) const {
        return dd < f.dd;
    }

    bool dependsOn(CuddBdd_ const& var) const {
        if (var.isConstant()) return false;
        return Cudd_bddVarIsDependent(manager, dd, var.dd);
    }

    bool contains(CuddBdd_ const& f) const {
        return Cudd_bddLeq(manager, f.dd, dd);
    }

    bool intersects(CuddBdd_ const& f) const {
        return !Cudd_bddLeq(manager, dd, Cudd_Not(f.dd) );
    }

    CuddBdd_ operator~() const {
        return Cudd_Not(dd) ;
    }

    CuddBdd_ operator&(CuddBdd_ const& f) const {
        return Cudd_bddAnd(manager, dd, f.dd);
    }

    CuddBdd_& operator&=(CuddBdd_ const& f) {
        return *this = *this & f;
    }

    CuddBdd_ operator|(CuddBdd_ const& f) const {
        return Cudd_bddOr(manager, dd, f.dd);
    }

    CuddBdd_& operator|=(CuddBdd_ const& f) {
        return *this = *this | f;
    }

    CuddBdd_ operator^(CuddBdd_ const& f) const {
        return Cudd_bddXor(manager, dd, f.dd);
    }

    CuddBdd_& operator^=(CuddBdd_ const& f) {
        return *this = *this ^ f;
    }

    CuddBdd_ ite(CuddBdd_ const& ft, CuddBdd_ const& fe) const {
        return Cudd_bddIte(manager, dd, ft.dd, fe.dd);
    }

    CuddBdd_ support() const {
        return Cudd_Support(manager, dd);
    }

    CuddBdd_ abstract(CuddBdd_ const& cube) const {
        if (cube.isConstant()) return *this;
        return Cudd_bddExistAbstract(manager, dd, cube.dd);
    }

    CuddBdd_ andAbstract(CuddBdd_ const& f, CuddBdd_ const& cube) const {
        if (cube.isConstant()) return *this & f;
        return Cudd_bddAndAbstract(manager, dd, f.dd, cube.dd);
    }

    CuddBdd_ cofactor(CuddBdd_ const& c) const {
        return Cudd_bddConstrain(manager, dd, c.dd);
    }

    CuddBdd_ minimize(CuddBdd_ const& c) const {
        return Cudd_bddMinimize(manager, dd, c.dd);
    }

    template<typename T>
    CuddBdd_ compose(T const& vector) const {
        MyVector<DdNode*> v(Cudd_ReadSize(manager));

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

    CuddBdd_ abstract1(CuddBdd_ const& cube) const {
        MyHashMap<CuddBdd_,CuddBdd_> cache(size() * 2);
        return abstract1_step(cache, cube);
    }

private:
    CuddBdd_ abstract1_step(MyHashMap<CuddBdd_,CuddBdd_>& cache,
            CuddBdd_ cube) const {
        int i = level();
        if (i < 1) return *this;

        CuddBdd_& f = cache[*this];
        if (!f.isNull()) return f;

        while (cube.level() > i) {
            cube = cube.child(1);
        }

        CuddBdd_ f0 = child(0).abstract1_step(cache, cube);
        CuddBdd_ f1 = child(1).abstract1_step(cache, cube);
        if (cube.level() == i) f1 |= f0;

        f = CuddBdd_(i, f0, f1);
        return f;
    }

public:
    int getRoot(CuddBdd_& f) {
        f = *this;
        return (f == 1) ? -1 : f.level();
    }

    int getChild(CuddBdd_& f, int level, int take) {
        DdNode* v = varAtLevel(level);
        f = Cudd_Cofactor(manager, f.dd, take ? v : Cudd_Not(v) );
        assert(level - Cudd_NodeReadIndex(f.dd) > 0);
        return (f == 1) ? -1 : f.level();
    }

    size_t hashCode(CuddBdd_ const& f) const {
        return f.hash();
    }

    void printState(std::ostream& os, CuddBdd_ const& f) const {
        os << f.level();
    }

    friend std::ostream& operator<<(std::ostream& os, CuddBdd_ const& f) {
        return os << "BDD(" << f.dd << ")";
    }
};

template<int N>
DdManager* CuddBdd_<N>::manager = Cudd_Init(0, 0, CUDD_UNIQUE_SLOTS,
        CUDD_CACHE_SLOTS, 0);

typedef CuddBdd_<0> CuddBdd;

} // namespace tdzdd
