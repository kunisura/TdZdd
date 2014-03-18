/*
 * Top-Down ZDD Construction Library for Frontier-Based Search
 * by Hiroaki Iwashita <iwashita@erato.ist.hokudai.ac.jp>
 * Copyright (c) 2012 Japan Science and Technology Agency
 * $Id: DdEncoder.hpp 518 2014-03-18 05:29:23Z iwashita $
 */

#pragma once

#include <cassert>
#include <ostream>

#include "DdBuilder.hpp"
#include "NodeTable.hpp"
#include "DdSpec.hpp"
#include "../util/MyHashTable.hpp"
#include "../util/MyList.hpp"
#include "../util/MyVector.hpp"

namespace tdzdd {

/**
 * Wrapper for mapping the spec's state to @p NodeId.
 * @param S ZDD spec.
 */
template<typename S, int ARITY>
class DdEncoder: public ScalarDdSpec<DdEncoder<S,ARITY>,NodeId,ARITY> {
    NodeTableHandler<ARITY> diagram;
    NodeId root;
    InstantDdBuilder<S> idb;
    int readyLevel;

public:
    explicit DdEncoder(S const& spec)
            : idb(spec, diagram), readyLevel(0) {
    }

    NodeId getRoot() {
        idb.initialize(root);
        readyLevel = root.row();
        idb.construct(readyLevel);
        return root;
    }

    NodeId getChild(NodeId f, int b) {
        assert(0 < f.row() && f.row() < diagram->numRows());

        while (readyLevel > f.row()) {
            idb.construct(--readyLevel);
        }

        assert(f.col() < (*diagram)[f.row()].size());
        return diagram->child(f, b);
    }

    void destructLevel(int i) {
        diagram.derefLevel(i);
    }
};

//template<typename S>
//DdEncoder<S> encode(S const& spec) {
//    return DdEncoder<S>(spec);
//}

} // namespace tdzdd
