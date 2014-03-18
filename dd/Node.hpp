/*
 * Top-Down ZDD Construction Library for Frontier-Based Search
 * by Hiroaki Iwashita <iwashita@erato.ist.hokudai.ac.jp>
 * Copyright (c) 2012 Japan Science and Technology Agency
 * $Id: Node.hpp 517 2014-01-29 05:15:16Z iwashita $
 */

#pragma once

#include <cassert>
#include <stdint.h>
#include <ostream>

namespace tdzdd {

int const NODE_ROW_BITS = 20;
int const NODE_ATTR_BITS = 1;
int const NODE_COL_BITS = 64 - NODE_ROW_BITS - NODE_ATTR_BITS;

int const NODE_ROW_OFFSET = NODE_COL_BITS + NODE_ATTR_BITS;
int const NODE_ATTR_OFFSET = NODE_COL_BITS;

uint64_t const NODE_ROW_MAX = (uint64_t(1) << NODE_ROW_BITS) - 1;
uint64_t const NODE_COL_MAX = (uint64_t(1) << NODE_COL_BITS) - 1;

uint64_t const NODE_ROW_MASK = NODE_ROW_MAX << NODE_ROW_OFFSET;
uint64_t const NODE_ATTR_MASK = uint64_t(1) << NODE_ATTR_OFFSET;

class NodeId {
    uint64_t code_;

public:
    NodeId() { // Member 'code_' was not initialized in this constructor for SPEED.
    }

    NodeId(size_t code)
            : code_(code) {
    }

    NodeId(size_t row, size_t col)
            : code_((row << NODE_ROW_OFFSET) | col) {
    }

    NodeId(size_t row, size_t col, bool attr)
            : code_((row << NODE_ROW_OFFSET) | col) {
        setAttr(attr);
    }

    int row() const {
        return code_ >> NODE_ROW_OFFSET;
    }

    size_t col() const {
        return code_ & NODE_COL_MAX;
    }

//    NodeId withAttr(NodeId f) const {
//        uint64_t attr = (f == 1) ? NODE_ATTR_MASK : (f.code_ & NODE_ATTR_MASK);
//        return NodeId((code_ & ~NODE_ATTR_MASK) | attr);
//    }
//
//    NodeId withoutAttr() const {
//        return NodeId(code_ & ~NODE_ATTR_MASK);
//    }

    void setAttr(bool val) {
        if (val) {
            code_ |= NODE_ATTR_MASK;
        }
        else {
            code_ &= ~NODE_ATTR_MASK;
        }
    }

    bool getAttr() const {
        return (code_ & NODE_ATTR_MASK) != 0;
    }

    NodeId withoutAttr() const {
        return code_ & ~NODE_ATTR_MASK;
    }

    bool hasEmpty() const {
        return code_ == 1 || getAttr();
    }

    size_t code() const {
        return code_ & ~NODE_ATTR_MASK;
    }

    size_t hash() const {
        return code() * 314159257;
    }

    bool operator==(NodeId const& o) const {
        return code() == o.code();
    }

    bool operator!=(NodeId const& o) const {
        return !(*this == o);
    }

    bool operator<(NodeId const& o) const {
        return code() < o.code();
    }

    bool operator>=(NodeId const& o) const {
        return !(*this < o);
    }

    bool operator>(NodeId const& o) const {
        return o < *this;
    }

    bool operator<=(NodeId const& o) const {
        return !(o < *this);
    }

    friend std::ostream& operator<<(std::ostream& os, NodeId const& o) {
        os << o.row() << ":" << o.col();
        if (o.code_ & NODE_ATTR_MASK) os << "+";
        return os;
    }
};

template<int ARITY>
struct Node {
    NodeId branch[ARITY];

    Node() {
    }

    Node(NodeId f0, NodeId f1) {
        branch[0] = f0;
        for (int i = 1; i < ARITY; ++i) {
            branch[i] = f1;
        }
    }

    Node(NodeId const* f) {
        for (int i = 0; i < ARITY; ++i) {
            branch[i] = f[i];
        }
    }

    size_t hash() const {
        size_t h = branch[0].code();
        for (int i = 1; i < ARITY; ++i) {
            h = h * 314159257 + branch[i].code() * 271828171;
        }
        return h;
    }

    bool operator==(Node const& o) const {
        for (int i = 0; i < ARITY; ++i) {
            if (branch[i] != o.branch[i]) return false;
        }
        return true;
    }

    friend std::ostream& operator<<(std::ostream& os, Node const& o) {
        os << "(" << o.branch[0];
        for (int i = 1; i < ARITY; ++i) {
            os << "," << o.branch[i];
        }
        return os << ")";
    }
};

struct NodeIdPair {
    NodeId first;
    NodeId second;

    /**
     * Default constructor.
     */
    NodeIdPair() {
    }

    /**
     * Normal constructor.
     */
    NodeIdPair(NodeId first, NodeId second)
            : first(first), second(second) {
    }

//    /**
//     * Special constructor for forwarding to an existing node.
//     */
//    NodeIdPair(NodeId first)
//            : first(first), second(NODE_ROW_MAX, NODE_COL_MAX) {
//    }
//
//    /**
//     * Checks if this is a forwarding object.
//     * @return true if forwarding.
//     */
//    bool isForwarding() const {
//        return second == NodeId(NODE_ROW_MAX, NODE_COL_MAX);
//    }

    size_t hash() const {
        return first.code() * 314159257 + second.code() * 271828171;
    }

    bool operator==(NodeIdPair const& o) const {
        return first == o.first && second == o.second;
    }

    friend std::ostream& operator<<(std::ostream& os, NodeIdPair const& o) {
        return os << "(" << o.first << "," << o.second << ")";
    }
};

} // namespace tdzdd
