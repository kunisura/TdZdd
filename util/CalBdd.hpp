/*
 * CAL Wrapper
 * Hiroaki Iwashita <iwashita@erato.ist.hokudai.ac.jp>
 * Copyright (c) 2011 Japan Science and Technology Agency
 * $Id: CalBdd.hpp 450 2013-07-29 01:49:19Z iwashita $
 */

#pragma once

#include <cassert>
#include <stdexcept>

extern "C" {
#include <cal.h>
}

namespace tdzdd {

class CalBdd {
    static Cal_BddManager manager;

    Cal_Bdd dd;

    CalBdd(Cal_Bdd dd)
            : dd(dd) {
        if (dd == 0) throw std::runtime_error("CAL returned NULL");
    }

    int index2level(int index) const {
        CalBdd var = Cal_BddManagerGetVarWithId(manager, index);
        return var.level();
    }

    int level2index(int level) const {
        CalBdd var = Cal_BddManagerGetVarWithIndex(manager, Cal_BddVars(manager) - level);
        return Cal_BddGetIfId(manager, var);
    }

public:
    static void init() {
        manager = Cal_BddManagerInit();
    }

    CalBdd()
            : dd(0) {
    }

    CalBdd(int val)
            : dd(val ? Cal_BddOne(manager) : Cal_BddZero(manager)) {
    }

    CalBdd(int level, CalBdd const& f0, CalBdd const& f1) {
        while (Cal_BddVars(manager) <= level) {
            Cal_BddManagerCreateNewVarFirst(manager);
        }
        CalBdd var = Cal_BddManagerGetVarWithId(manager, level2index(level));
        dd = Cal_BddITE(manager, var.dd, f1.dd, f0.dd);
    }

    CalBdd(CalBdd const& o)
            : dd(Cal_BddIdentity(manager, o.dd)) {
    }

    CalBdd& operator=(CalBdd const& o) {
        this->~CalBdd();
        dd = Cal_BddIdentity(manager, o.dd);
        return *this;
    }

    ~CalBdd() {
        if (dd != 0) Cal_BddFree(manager, dd);
    }

    operator Cal_Bdd() {
        return dd;
    }

    size_t size() const {
        return Cal_BddSize(manager, dd, false);
    }

    int index() const {
        return Cal_BddGetIfId(manager, dd);
    }

    int level() const {
        return Cal_BddVars(manager) - Cal_BddGetIfIndex(manager, dd);
    }

    bool operator==(CalBdd const& f) const {
        return dd == f.dd;
    }

    bool operator!=(CalBdd const& f) const {
        return dd != f.dd;
    }

    bool operator<(CalBdd const& f) const {
        return dd < f.dd;
    }

    CalBdd operator~() const {
        return Cal_BddNot(manager, dd);
    }

    CalBdd operator&(CalBdd const& f) const {
        return Cal_BddAnd(manager, dd, f.dd);
    }

    CalBdd& operator&=(CalBdd const& f) {
        return *this = *this & f;
    }

    CalBdd operator|(CalBdd const& f) const {
        return Cal_BddOr(manager, dd, f.dd);
    }

    CalBdd& operator|=(CalBdd const& f) {
        return *this = *this | f;
    }

    CalBdd operator-(CalBdd const& f) const {
        return *this & ~f;
    }

    CalBdd& operator-=(CalBdd const& f) {
        return *this = *this - f;
    }

    void print() const {
        Cal_BddPrintBdd(manager, dd, 0, 0, 0, stdout);
    }
};

Cal_BddManager CalBdd::manager;

} // namespace tdzdd
