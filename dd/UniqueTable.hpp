/*
 * Top-Down ZDD Construction Library for Frontier-Based Search
 * by Hiroaki Iwashita <iwashita@erato.ist.hokudai.ac.jp>
 * Copyright (c) 2012 Japan Science and Technology Agency
 * $Id: UniqueTable.hpp 517 2014-01-29 05:15:16Z iwashita $
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

//    /**
//     * Deletes the nodes that are not reachable from a given root node.
//     * @param root root node.
//     * return new node ID for the root node.
//     */
//    NodeId gcForSingleRoot(NodeId root) {
//        MyVector<NodeId> roots(1);
//        roots[0] = root;
//        gcForMultipleRoots(roots);
//        return roots[0];
//    }
//
//    /**
//     * Deletes the nodes that are not reachable from a given set of root nodes.
//     * @param roots modifiable collection of root nodes.
//     */
//    template<typename C>
//    void gcForMultipleRoots(C& roots) {
//        uniqTable.clear();
//        NodeProperty<NodeId> newId(*nodeTable);
//        MyVector<size_t> newSize(nodeTable->numRows());
//
//        // Mark the roots
//        for (typeof(roots.begin()) t = roots.begin(); t != roots.end(); ++t) {
//            newId.value(*t) = 1;
//        }
//
//        // Propagate marks and number live nodes
//        for (int i = nodeTable->numVars(); i > 0; --i) {
//            size_t m = nodeTable[i].size();
//            size_t jj = 0;
//
//            for (size_t j = 0; j < m; ++j) {
//                NodeId& f = newId[i][j];
//                if (f == 0) continue;
//
//                f = NodeId(i, jj++);
//                newId.value(nodeTable->child(i, j, 0)) = 1;
//                newId.value(nodeTable->child(i, j, 1)) = 1;
//            }
//
//            newSize[i] = jj;
//        }
//        newId[0][0] = 0;
//        newId[0][1] = 1;
//
//        // Renumber the live nodes
//        for (int i = nodeTable->numVars(); i > 0; --i) {
//            typeof((*nodeTable)[i])& nodes = (*nodeTable)[i];
//            size_t m = nodes.size();
//
//            for (size_t j = 0; j < m; ++j) {
//                NodeId f = newId[i][j];
//                if (f == 0) continue;
//                size_t jj = f.col();
//
//                for (int b = 0; b <= 1; ++b) {
//                    nodes[jj].branch[b] = newId.value(nodes[j].branch[b]);
//                }
//            }
//
//            nodes.resize(newSize[i]);
//        }
//
//        // Renumber the roots
//        for (typeof(roots.begin()) t = roots.begin(); t != roots.end(); ++t) {
//            *t = newId.value(*t);
//        }
//
//        init();
//    }

private:
//    template<typename V>
//    NodeId doAdd(NodeId f, V& items) {
//        std::sort(items.begin(), items.end());
//        int ni = std::unique(items.begin(), items.end()) - items.begin();
//        int nv = (ni >= 1) ? items[ni - 1] + 1 : 0;
//        if (nv > numVars()) {
//            int v = varAtLevel(f.row());
//            setNumVars(nv * 2);
//            f = NodeId(levelOfVar(v), f.col());
//        }
//
//        for (int k = 0; k < ni; ++k) {
//            items[k] = levelOfVar(items[k]);
//        }
//
//        return doAdd(f, items.data(), items.data() + ni);
//    }
//
//    NodeId doAdd(NodeId f, int* beg, int* end) {
//        int i = (beg < end) ? *beg : 0;
//        if (f.row() == 0 && i == 0) return 1;
//
//        NodeId f0, f1;
//        if (f.row() < i) {
//            f0 = f;
//            f1 = doAdd(0, beg + 1, end);
//        }
//        else if (f.row() > i) {
//            i = f.row();
//            f0 = doAdd(diagram->child(f, 0), beg, end);
//            f1 = diagram->child(f, 1);
//        }
//        else {
//            f0 = diagram->child(f, 0);
//            f1 = doAdd(diagram->child(f, 1), beg + 1, end);
//        }
//
//        return getNode(i, f0, f1);
//    }
};

} // namespace tdzdd
