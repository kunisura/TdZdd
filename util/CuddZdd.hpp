/*
 * CUDD Wrapper
 * Hiroaki Iwashita <iwashita@erato.ist.hokudai.ac.jp>
 * Copyright (c) 2011 Japan Science and Technology Agency
 * $Id: CuddZdd.hpp 489 2013-10-16 10:15:08Z iwashita $
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
#include <stdexcept>

#include <cudd.h>

namespace tdzdd {

class CuddZdd {
    DdNode* dd;

public:
    static DdManager* manager;

    static int index2level(int index) {
        return Cudd_ReadZddSize(manager) - Cudd_ReadPermZdd(manager, index);
    }

    static int level2index(int level) {
        return Cudd_ReadInvPermZdd(manager, Cudd_ReadZddSize(manager) - level);
    }

    static void init(int numVars = 0, unsigned numSlots = CUDD_UNIQUE_SLOTS,
            unsigned cacheSize = CUDD_CACHE_SLOTS, size_t maxMemory = 0) {
        manager = Cudd_Init(0, numVars, numSlots, cacheSize, maxMemory);
        //Cudd_PrintInfo(manager, stdout);
    }

    CuddZdd()
            : dd(0) {
    }

    CuddZdd(int val)
            : dd(val ? Cudd_ReadOne(manager) : Cudd_ReadZero(manager)) {
        Cudd_Ref(dd);
    }

    CuddZdd(int level, CuddZdd const& f0, CuddZdd const& f1) {
        CuddZdd f = f0 | f1.change(level);
        dd = f.dd;
        Cudd_Ref(dd);
    }

    CuddZdd(DdNode* dd)
            : dd(dd) {
        if (dd == 0) throw std::runtime_error("CUDD returned NULL");
        Cudd_Ref(dd);
    }

    CuddZdd(CuddZdd const& o)
            : dd(o.dd) {
        assert(dd != 0);
        Cudd_Ref(dd);
    }

    CuddZdd& operator=(CuddZdd const& o) {
        this->~CuddZdd();
        dd = o.dd;
        Cudd_Ref(dd);
        return *this;
    }

    ~CuddZdd() {
        if (dd != 0) Cudd_RecursiveDerefZdd(manager, dd);
    }

    operator DdNode*() {
        return dd;
    }

    size_t size() const {
        return Cudd_DagSize(dd);
    }

    int index() const {
        return Cudd_NodeReadIndex(dd);
    }

    int level() const {
        return index2level(index());
    }

    CuddZdd change(int level) const {
        return Cudd_zddChange(manager, dd, level2index(level));
    }

    bool operator==(CuddZdd const& f) const {
        return dd == f.dd;
    }

    bool operator!=(CuddZdd const& f) const {
        return dd != f.dd;
    }

    bool operator<(CuddZdd const& f) const {
        return dd < f.dd;
    }

//    CuddZdd operator~() const {
//        return CuddZdd(Cudd_zddDiff(manager, universe, dd), true);
//    }

    CuddZdd operator&(CuddZdd const& f) const {
        return Cudd_zddIntersect(manager, dd, f.dd);
    }

    CuddZdd& operator&=(CuddZdd const& f) {
        return *this = *this & f;
    }

    CuddZdd operator|(CuddZdd const& f) const {
        return Cudd_zddUnion(manager, dd, f.dd);
    }

    CuddZdd& operator|=(CuddZdd const& f) {
        return *this = *this | f;
    }

    CuddZdd operator-(CuddZdd const& f) const {
        return Cudd_zddDiff(manager, dd, f.dd);
    }

    CuddZdd& operator-=(CuddZdd const& f) {
        return *this = *this - f;
    }

    static int reorder(Cudd_ReorderingType heuristic = CUDD_REORDER_SIFT,
            int minsize = 0) {
        return Cudd_zddReduceHeap(manager, heuristic, minsize);
    }

    int dumpDot() {
        return Cudd_zddDumpDot(manager, 1, &dd, NULL, NULL, stdout);
    }
};

DdManager* CuddZdd::manager;

} // namespace tdzdd
