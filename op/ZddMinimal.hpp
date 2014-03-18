/*
 * Top-Down ZDD Construction Library for Frontier-Based Search
 * by Hiroaki Iwashita <iwashita@erato.ist.hokudai.ac.jp>
 * Copyright (c) 2012 Japan Science and Technology Agency
 * $Id: ZddMinimal.hpp 518 2014-03-18 05:29:23Z iwashita $
 */

#pragma once

#include <cassert>
#include <iostream>
#include <utility>

#include "../dd/DdBuilder.hpp"
#include "../dd/DdSpec.hpp"
#include "../dd/DdStructure.hpp"
#include "../util/MyList.hpp"
#include "../util/MySet.hpp"
#include "../util/MyVector.hpp"

//#define ZDDMINIMALUNION

namespace tdzdd {

struct ZddMinimalState {
    typedef MySmallSetOnPool<NodeId> SetOnPool;

    NodeId nodeId;
    SetOnPool* set;

    ZddMinimalState()
            : set(0) {
    }

    size_t hash() const {
        size_t h = nodeId.hash();
        return set ? h + set->hash() : h;
    }

    bool operator==(ZddMinimalState const& o) const {
        return nodeId == o.nodeId
                && ((set && o.set && *set == *o.set) || (!set && !o.set));
    }

    friend std::ostream& operator<<(std::ostream& os,
            ZddMinimalState const& o) {
        os << o.nodeId << "-";
        if (o.set) os << *o.set;
        else os << "{}";
        return os;
    }
};

#ifdef ZDDMINIMALUNION
class ZddMinimalUnion: public ScalarDdSpec<ZddMinimalUnion,
        MySmallSetOnPool<NodeId>*> {
    typedef MySmallSetOnPool<NodeId> SetOnPool;

    NodeTableHandler diagram;
    MemoryPools pools;
    MyVector<NodeId> work;

public:
    ZddMinimalUnion(NodeTableHandler const& diagram, int n)
            : diagram(diagram), pools(n + 1) {
    }

    int getChild(SetOnPool*& s, int level, int take) {
        assert(level > 0);
        work.resize(0);
        int nextLevel = 0;

        for (NodeId const* p = s->begin(); p != s->end(); ++p) {
            NodeId f = *p;
            assert(f.row() <= level);

            if (f.row() == level) {
                f = diagram->child(f, take);
            }
            else if (take) {
                f = 0;
            }

            if (f == 0) continue;
            if (f == 1) return -1;

            work.push_back(f);
            if (f.row() > nextLevel) nextLevel = f.row();
        }

        assert(0 <= nextLevel && nextLevel < level);
        if (nextLevel == 0) return 0;

        s = SetOnPool::newInstance(pools[nextLevel], work);
        return nextLevel;
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
};

#endif //ZDDMINIMALUNION
template<bool assumeMonotonic = true>
class ZddMinimal: public ScalarDdSpec<ZddMinimal<assumeMonotonic>,
        ZddMinimalState> {
    typedef MySmallSetOnPool<NodeId> SetOnPool;

    NodeTableHandler<2> diagram0; // a local copy
    NodeTableHandler<2>& diagram; // reference to the first copy
    NodeId root;
    int fromLevel;
    int toLevel;
    MemoryPools pools;
    MyVector<NodeId> work;

#ifdef ZDDMINIMALUNION
    size_t totalSize;
    size_t totalCount;
    DdBuilder<ZddMinimalUnion>* unionBuilder;
#endif //ZDDMINIMALUNION
    struct UnionRoot {
        ZddMinimalState& state;
        NodeId f;
        NodeId g;

        UnionRoot(ZddMinimalState& state)
                : state(state) {
        }
    };

    MyList<UnionRoot> unionRoots;

public:
    ZddMinimal(DdStructure const& dd, int fromLevel = 0, int toLevel = 0)
            : diagram0(dd.getDiagram()), diagram(diagram0), root(dd.root()),
              fromLevel(fromLevel >= 1 ? fromLevel : root.row()),
              toLevel(toLevel >= 1 ? toLevel : 1), pools(root.row() + 1)
#ifdef ZDDMINIMALUNION
                      ,
              totalSize(0), totalCount(0), unionBuilder(0)
#endif //ZDDMINIMALUNION
    {
    }

#ifdef ZDDMINIMALUNION
    ~ZddMinimal() {
        delete unionBuilder;
    }
#endif //ZDDMINIMALUNION
    int getRoot(ZddMinimalState& s) {
        int i = root.row();
        if (i == 0) return root == 1 ? -1 : 0;
        s.nodeId = root;
        s.set = 0;
        return i;
    }

    int getChild(ZddMinimalState& s, int level, int take) {
        assert(level > 0);
        NodeId f = s.nodeId;
        int nextLevel;
        work.resize(0);

        if (take) {
            if (f.hasEmpty()) return 0;
            NodeId f1 = diagram->child(f, 1);
            if (f1 == 0) return 0;

            s.nodeId = f1;
            nextLevel = f1.row();

            if (fromLevel >= level && level >= toLevel) {
                if (f1 == 1) {
                    assert(!f.hasEmpty());
                }
                else {
                    NodeId f0 = diagram->getZeroDescendant(f, nextLevel);
                    if (f0 == 1 || f0 == f1) return 0;
                    if (f0 != 0) work.push_back(f0);
                }
            }

            if (s.set) {
                for (NodeId const* p = s.set->begin(); p != s.set->end(); ++p) {
                    NodeId g = *p;
                    assert(g.row() <= level);

                    NodeId g1 = 0;
                    if (g.row() == level) {
                        g1 = diagram->child(g, 1);
                        g1 = diagram->getZeroDescendant(g1, nextLevel);
                        if (g1 == 1 || g1 == f1) return 0;
                        if (g1 != 0) work.push_back(g1);
                    }

                    if (assumeMonotonic) continue;

                    NodeId g0 = diagram->getZeroDescendant(g, nextLevel);
                    if (g0 == 1 || g0 == f1) return 0;
                    if (g0 != 0 && g0 != g1) work.push_back(g0);
                }
            }
        }
        else {
            NodeId f0 = diagram->child(f, 0);
            if (f0 == 0) return 0;

            s.nodeId = f0;
            nextLevel = f0.row();

            if (s.set) {
                for (NodeId const* p = s.set->begin(); p != s.set->end(); ++p) {
                    NodeId g = *p;
                    assert(g.row() <= level);
                    NodeId g0 = diagram->getZeroDescendant(g, nextLevel);
                    if (g0 == 1 || g0 == f0) return 0;
                    if (g0 != 0) work.push_back(g0);
                }
            }
        }

        if (work.empty()) {
            s.set = 0;
        }
        else {
            s.set = SetOnPool::newInstance(pools[nextLevel], work);
#ifdef ZDDMINIMALUNION
            totalSize += s.set->size();
#endif //ZDDMINIMALUNION
        }
#ifdef ZDDMINIMALUNION
        ++totalCount;
#endif //ZDDMINIMALUNION
        return (s.nodeId == 1) ? -1 : nextLevel;
    }

    void destructLevel(int i) {
        pools[i].clear();
    }

#ifdef ZDDMINIMALUNION
    bool needWipedown(int i) {
        if ((*diagram)[i].empty()) return false;
        if (totalSize <= totalCount * 3) { // magic number
            totalSize = totalCount = 0;
            return false;
        }
        std::cerr << "[" << totalSize << "/" << totalCount << "]";
        delete unionBuilder;
        ZddMinimalUnion spec(diagram, i);
        diagram.init();
        unionBuilder = new DdBuilder<ZddMinimalUnion>(spec, diagram, i);
        return true;
    }

    void setWipedownRoot(ZddMinimalState& s, int i) {
        UnionRoot* p = new (unionRoots.alloc_front()) UnionRoot(s);
        SetOnPool* set = SetOnPool::newInstance(pools[i], 1);
        set->add(s.nodeId);
        unionBuilder->schedule(&p->f, i, &set);
        unionBuilder->schedule(&p->g, i, &s.set);
    }

    void doWipedown(int i) {
        for (int ii = i; ii >= 1; --ii) {
            unionBuilder->construct(ii);
            pools[ii].clear();
        }
        delete unionBuilder;
        unionBuilder = 0;

        ZddReducer<> reducer(diagram);
        for (typename MyList<UnionRoot>::iterator t = unionRoots.begin();
                t != unionRoots.end(); ++t) {
            reducer.setRoot(t->f);
            reducer.setRoot(t->g);
        }
        for (int ii = 1; ii <= i; ++ii) {
            reducer.reduce(ii);
        }

        totalSize = totalCount = unionRoots.size();

        while (!unionRoots.empty()) {
            UnionRoot* p = unionRoots.front();
            ZddMinimalState& s = p->state;
            s.nodeId = p->f;
            s.set = SetOnPool::newInstance(pools[p->f.row()], 1);
            s.set->add(p->g);
            unionRoots.pop_front();
        }
    }

#endif //ZDDMINIMALUNION
    size_t hashCode(ZddMinimalState const& s) const {
        return s.hash();
    }
};

} // namespace tdzdd
