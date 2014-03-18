/*
 * Top-Down ZDD Construction Library for Frontier-Based Search
 * by Hiroaki Iwashita <iwashita@erato.ist.hokudai.ac.jp>
 * Copyright (c) 2014 Japan Science and Technology Agency
 * $Id: TddHitting.hpp 518 2014-03-18 05:29:23Z iwashita $
 */

#pragma once

#include <cassert>
#include <iostream>

#include "../dd/DdBuilder.hpp"
#include "../dd/DdSpec.hpp"
#include "../spec/CnfTdd.hpp"
#include "../util/MySet.hpp"
#include "../util/MyVector.hpp"

namespace tdzdd {

template<bool method1, bool method2>
class TddHitting: public ScalarDdSpec<TddHitting<method1,method2>,
        MySmallSetOnPool<CnfTdd::NodeNumber>*> {
    typedef CnfTdd::NodeNumber NodeNumber;
    typedef MySmallSetOnPool<NodeNumber> SetOnPool;

    CnfTdd const& tdd;
    NodeNumber const root;
    MemoryPools pools;

    struct Assignment {
        int const level;
        int const value;

        Assignment(int level, int value)
                : level(level), value(value) {
        }

        friend std::ostream& operator<<(std::ostream& os, Assignment const& o) {
            return os << o.level << "=" << o.value;
        }
    };

    typedef MyVector<Assignment> Consequent;
    MyVector<Consequent> implications[3];

    MyVector<NodeNumber> work;
    MyVector<int> fixedValue;

    bool fixValue(int level, int value) {
        if (fixedValue.size() <= size_t(level)) return false;
        if (fixedValue[level] == value) return false;
        if (fixedValue[level] != 0) return true;
        fixedValue[level] = value;

        Consequent const& conseq = implications[value][level];
        for (typeof(conseq.begin()) t = conseq.begin(); t != conseq.end();
                ++t) {
            if (fixValue(t->level, t->value)) return true;
        }

        return false;
    }

    bool conflicting(int level, SetOnPool const& clauses) {
        fixedValue.resize(0);
        fixedValue.resize(level + 1); // clear

        for (NodeNumber const* p = clauses.begin(); p != clauses.end(); ++p) {
            for (NodeNumber f = *p;; f = tdd.child(f, 0)) {
                int i = tdd.node(f).level;
                if (i <= 0) break;

                for (int b = 1; b <= 2; ++b) {
                    if (tdd.child(f, b) == 1) { // value must be `b'
                        if (fixValue(i, b)) return true;
                    }
                }
            }
        }

        return false;
    }

    int update(SetOnPool*& s, int level, bool take) {
        work.resize(0);

        for (NodeNumber const* p = s->begin(); p != s->end(); ++p) {
            NodeNumber f = *p;
            int i = tdd.node(f).level;
            assert(i <= level);

            NodeNumber f0 = (i == level) ? tdd.child(f, 0) : f;
            if (f0 == 1) return 0;
            if (f0 != 0) work.push_back(f0);

            NodeNumber fb = (i == level) ? tdd.child(f, take ? 1 : 2) : 0;
            if (fb == 1) return 0;
            if (fb != 0) work.push_back(fb);
        }

        if (--level == 0) return -1;

        //std::sort(work.begin(), work.end());
        s = SetOnPool::newInstance(pools[level], work);
        if (method2 && tdd.conflictsWith(level, *s)) return 0;
        return level;
    }

    bool untakable(SetOnPool* const & s, int level) {
        for (NodeNumber const* p = s->begin(); p != s->end(); ++p) {
            NodeNumber f = *p;
            int i = tdd.node(f).level;
            assert(i <= level);

            if (i == level) {
                if (tdd.child(f, 0) == 1) return true;
                if (tdd.child(f, 1) == 1) return true;
            }
        }
        return false;
    }

public:
    TddHitting(CnfTdd const& tdd)
            : tdd(tdd), root(tdd.root()), pools(tdd.topLevel() + 1) {
        if (method1) {
            implications[1].resize(tdd.topLevel() + 1);
            implications[2].resize(tdd.topLevel() + 1);

            for (NodeNumber f = root;; f = tdd.child(f, 0)) {
                int i = tdd.node(f).level;
                if (i <= 0) break;

                for (int b = 1; b <= 2; ++b) {
                    for (NodeNumber ff = tdd.child(f, b);;
                            ff = tdd.child(ff, 0)) {
                        int ii = tdd.node(ff).level;
                        if (ii <= 0) break;

                        for (int bb = 1; bb <= 2; ++bb) {
                            if (tdd.child(ff, bb) == 1) {
                                implications[3 - b][i].push_back(
                                        Assignment(ii, bb));
//                            implications[3 - bb][ii].push_back(
//                                    Assignment(i, b));
                            }
                        }
                    }
//                std::cerr << "\n" << f.row() << "=" << (3 - b) << " -> "
//                        << conseq;
                }
            }
        }
    }

    int getRoot(SetOnPool*& s) {
        int i = tdd.node(root).level;
        if (i == 0) return root == 0 ? -1 : 0;
        s = SetOnPool::newInstance(pools[i], 1);
        s->add(root);
        return i;
    }

    int getChild(SetOnPool*& s, int level, int take) {
        assert(level > 0);

        level = update(s, level, take);
        if (level <= 0) return level;

//        for (;;) {
//            SetOnPool* tmp = s;
//            if (update(tmp, level, true) != 0 && !conflicting(tmp)) break;
//            fixedValue.resize(0);
//            fixedValue.resize(level + 1); // clear
//            level = update(s, level, false);
//            if (level <= 0) return level;
//        }
        while (untakable(s, level)) {
            level = update(s, level, false);
            if (level <= 0) return level;
        }

        if (method1 && conflicting(level, *s)) return 0;

        return level;
    }

    void destructLevel(int i) {
        pools[i].clear();
    }

    size_t hashCode(SetOnPool* const & s) const {
        return s->hash();
    }

    bool equalTo(SetOnPool* const & s1, SetOnPool* const & s2) const {
        return *s1 == *s2;
    }

    std::ostream& printState(std::ostream& os, SetOnPool* const & s) const {
        return os << *s;
    }
};

//class TddHitting2: public ScalarDdSpec<TddHitting2,CuddBdd> {
//    typedef CnfTdd::NodeNumber NodeNumber;
//
//    CnfTdd const& tdd;
//    NodeNumber const root;
//
//public:
//    TddHitting2(CnfTdd const& tdd)
//            : tdd(tdd), root(tdd.root()) {
//    }
//
//    int getRoot(CuddBdd& f) {
//        int i = tdd.node(root).level;
//        if (i == 0) return root == 0 ? -1 : 0;
//        f = 1;
//        return i;
//    }
//
//    int getChild(CuddBdd& f, int level, int take) {
//        assert(level > 0);
//
//        level = update(f, level, take);
//        if (level <= 0) return level;
//
//        while (untakable(f, level)) {
//            level = update(f, level, false);
//            if (level <= 0) return level;
//        }
//
//        if (tdd.conflictsWithBdd(level, f)) return 0;
//
//        return level;
//    }
//
//    size_t hashCode(CuddBdd const& f) const {
//        return f.hash();
//    }
//
//    bool equalTo(CuddBdd const& f1, CuddBdd const& f2) const {
//        return f1 == f2;
//    }
//
//    std::ostream& printState(std::ostream& os, CuddBdd const& f) const {
//        return os << f;
//    }
//};

}// namespace tdzdd
