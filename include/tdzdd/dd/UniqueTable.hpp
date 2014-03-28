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

#include "Node.hpp"
#include "NodeTable.hpp"
#include "../util/MyHashTable.hpp"
#include "../util/MyVector.hpp"

namespace tdzdd {

template<int ARITY>
class UniqueTable {
    class Hash {
        MyVector<Node<ARITY> > const& nodes;

    public:
        Hash()
                : nodes(MyVector<Node<ARITY> >()) {
        }

        Hash(MyVector<Node<ARITY> > const& nodes)
                : nodes(nodes) {
        }

        size_t operator()(size_t j) const {
            return nodes[j - 1].hash(); // offset = 1
        }

        bool operator()(size_t j1, size_t j2) const {
            return nodes[j1 - 1] == nodes[j2 - 1]; // offset = 1
        }
    };

    typedef MyHashTable<size_t,Hash,Hash> HashTable;

    NodeTableEntity<ARITY>* nodeTable;
    MyVector<HashTable> uniqTable;

public:
    UniqueTable()
            : nodeTable(0) {
    }

    UniqueTable(NodeTableEntity<ARITY>& nodeTable)
            : nodeTable(&nodeTable) {
        init();
    }

    /**
     * Replaces the node table and rebuilds the unique table.
     * @param newNodeTable new node table.
     */
    void init(NodeTableEntity<ARITY>& newNodeTable) {
        nodeTable = &newNodeTable;
        init();
    }

    /**
     * Rebuilds the unique table.
     */
    void init() {
        uniqTable.reserve(nodeTable->numRows());
        uniqTable.resize(1);

        for (int i = 1; i < nodeTable->numRows(); ++i) {
            typeof((*nodeTable)[i])& nodes = (*nodeTable)[i];
            size_t m = nodes.size();
            uniqTable.push_back(HashTable(m * 2, Hash(nodes), Hash(nodes)));
            HashTable& uniq = uniqTable.back();

            for (size_t j = 0; j < m; ++j) {
                uniq.add(j + 1); // offset = 1
            }
        }
    }

    /**
     * Gets a canonical node.
     * @param i level of the node.
     * @param node node to be copied.
     * @return node ID.
     */
    NodeId getNode(int i, Node<ARITY> const& node) {
        typeof((*nodeTable)[i])& nodes = (*nodeTable)[i];
        nodes.push_back(node);
        size_t j = nodes.size() - 1;
        size_t jj = uniqTable[i].add(j + 1) - 1; // offset = 1
        if (jj != j) nodes.pop_back();
        return NodeId(i, jj);
    }
};

} // namespace tdzdd
