/*
 * Top-Down ZDD Construction Library for Frontier-Based Search
 * by Hiroaki Iwashita <iwashita@erato.ist.hokudai.ac.jp>
 * Copyright (c) 2013 Japan Science and Technology Agency
 * $Id: BddHitting.hpp 518 2014-03-18 05:29:23Z iwashita $
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

namespace tdzdd {

class BddHittingUnion: public ScalarDdSpec<BddHittingUnion,
        MySmallSetOnPool<NodeId>*> {
    typedef MySmallSetOnPool<NodeId> SetOnPool;

    NodeTableHandler<2> diagram;
    MemoryPools pools;
    MyVector<NodeId> work;

public:
    BddHittingUnion(NodeTableHandler<2> const& diagram, int n)
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
            if (f.hasEmpty()) return -1; // special case

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

class BddHitting: public ScalarDdSpec<BddHitting,MySmallSetOnPool<NodeId>*> {
    typedef MySmallSetOnPool<NodeId> SetOnPool;

    NodeTableHandler<2> diagram0; // a local copy
    NodeTableHandler<2>& diagram; // reference to the first copy
    NodeId root;
    MemoryPools pools;
    MyVector<NodeId> work;
    size_t totalSize;
    size_t totalCount;

    bool useMP;
    DdBuilder<BddHittingUnion>* unionBuilder;
    DdBuilderMP<BddHittingUnion>* unionBuilderMP;

    struct UnionRoot {
        SetOnPool*& set;
        NodeId nodeId;

        UnionRoot(SetOnPool*& set)
                : set(set) {
        }
    };

    MyList<UnionRoot> unionRoots;

public:
    BddHitting(DdStructure const& dd, bool useMP = false)
            : diagram0(dd.getDiagram()), diagram(diagram0), root(dd.root()),
              pools(root.row() + 1), totalSize(0), totalCount(0), useMP(useMP),
              unionBuilder(0), unionBuilderMP(0) {
    }

    ~BddHitting() {
        delete unionBuilder;
        delete unionBuilderMP;
    }

    int getRoot(SetOnPool*& s) {
        int i = root.row();
        if (i == 0) return root == 0 ? -1 : 0;
        s = SetOnPool::newInstance(pools[i], 1);
        s->add(root);
        return i;
    }

    int getChild(SetOnPool*& s, int level, int take) {
        assert(level > 0);
        work.resize(0);
        int nextLevel = 0;

        if (take) {
            for (NodeId const* p = s->begin(); p != s->end(); ++p) {
                NodeId f = *p;
                assert(f.row() <= level);

                NodeId f0 = (f.row() == level) ? diagram->child(f, 0) : f;
                if (f0.hasEmpty()) return 0;
                if (f0.row() > nextLevel) nextLevel = f0.row();
                if (f0 != 0) work.push_back(f0);
            }
        }
        else {
            for (NodeId const* p = s->begin(); p != s->end(); ++p) {
                NodeId f = *p;
                assert(f.row() <= level);

                NodeId f0 = (f.row() == level) ? diagram->child(f, 0) : f;
                if (f0.hasEmpty()) return 0;
                if (f0.row() > nextLevel) nextLevel = f0.row();
                if (f0 != 0) work.push_back(f0);

                NodeId f1 = (f.row() == level) ? diagram->child(f, 1) : 0;
                if (f1.hasEmpty()) return 0;
                if (f1.row() > nextLevel) nextLevel = f1.row();
                if (f1 != 0 && f1 != f0) work.push_back(f1);
            }
        }

        assert(0 <= nextLevel && nextLevel < level);
        if (nextLevel == 0) return -1;

        s = SetOnPool::newInstance(pools[nextLevel], work);
        totalSize += s->size();
        ++totalCount;
        return nextLevel;
    }

    void destructLevel(int i) {
        pools[i].clear();
    }

    bool needWipedown(int i) {
        if ((*diagram)[i].empty()) return false;
        if (totalSize <= totalCount * 3) { // magic number
            totalSize = totalCount = 0;
            return false;
        }
        //std::cerr << "[" << totalSize << "/" << totalCount << "]";
        BddHittingUnion spec(diagram, i);
        diagram.init();
        if (useMP) {
            delete unionBuilderMP;
            unionBuilderMP = new DdBuilderMP<BddHittingUnion>(spec, diagram, i);
        }
        else {
            delete unionBuilder;
            unionBuilder = new DdBuilder<BddHittingUnion>(spec, diagram, i);
        }
        return true;
    }

    void setWipedownRoot(SetOnPool*& s, int i) {
        UnionRoot* p = new (unionRoots.alloc_front()) UnionRoot(s);
        if (useMP) {
            unionBuilderMP->schedule(&p->nodeId, i, &s);
        }
        else {
            unionBuilder->schedule(&p->nodeId, i, &s);
        }
    }

    void doWipedown(int i) {
        if (useMP) {
            for (int ii = i; ii >= 1; --ii) {
                unionBuilderMP->construct(ii);
                pools[ii].clear();
            }
            delete unionBuilderMP;
            unionBuilderMP = 0;
        }
        else {
            for (int ii = i; ii >= 1; --ii) {
                unionBuilder->construct(ii);
                pools[ii].clear();
            }
            delete unionBuilder;
            unionBuilder = 0;
        }

        DdReducer<2,true,true> reducer(diagram, useMP);
        for (MyList<UnionRoot>::iterator t = unionRoots.begin();
                t != unionRoots.end(); ++t) {
            reducer.setRoot(t->nodeId);
        }
        for (int ii = 1; ii <= i; ++ii) {
            reducer.reduce(ii, useMP);
        }

        totalSize = totalCount = unionRoots.size();

        while (!unionRoots.empty()) {
            UnionRoot* p = unionRoots.front();
            SetOnPool*& s = p->set;
            NodeId f = p->nodeId;
            s = SetOnPool::newInstance(pools[f.row()], 1);
            s->add(f);
            unionRoots.pop_front();
        }
    }

    size_t hashCode(SetOnPool* const & s) const {
        return s->hash();
    }

    bool equalTo(SetOnPool* const & s1, SetOnPool* const & s2) const {
        return *s1 == *s2;
    }
};

} // namespace tdzdd
