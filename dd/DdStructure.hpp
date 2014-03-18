/*
 * Top-Down ZDD Construction Library for Frontier-Based Search
 * by Hiroaki Iwashita <iwashita@erato.ist.hokudai.ac.jp>
 * Copyright (c) 2012 Japan Science and Technology Agency
 * $Id: DdStructure.hpp 518 2014-03-18 05:29:23Z iwashita $
 */

#pragma once

#include <algorithm>
#include <cassert>
#include <climits>
#include <ostream>
#include <stdexcept>
#include <vector>

#include "DdBuilder.hpp"
#include "DdBuilderDF.hpp"
#include "DdEval.hpp"
#include "DdReducer.hpp"
#include "DdSpec.hpp"
#include "Node.hpp"
#include "NodeTable.hpp"
#include "../util/demangle.hpp"
#include "../util/MessageHandler.hpp"
#include "../util/MyHashTable.hpp"
#include "../util/MyVector.hpp"

namespace tdzdd {

class DdStructure: public ScalarDdSpec<DdStructure,NodeId,2> {
    NodeTableHandler<2> diagram; ///< The diagram structure.
    NodeId root_;         ///< Root node ID.

public:
    /**
     * Default constructor.
     */
    DdStructure()
            : root_(0) {
    }

    /**
     * Low level constructor.
     */
    DdStructure(NodeTableHandler<2> const& diagram, NodeId root)
            : diagram(diagram), root_(root) {
    }

//    /*
//     * Imports the ZDD that can be broken.
//     * @param o the ZDD.
//     */
//    void moveFrom(DdStructure& o) {
//        diagram.moveAssign(o.diagram);
//    }

    /**
     * Universal ZDD constructor.
     * @param n the number of variables.
     */
    DdStructure(int n)
            : diagram(n + 1), root_(1) {
        assert(n >= 0);
        NodeTableEntity<2>& table = diagram.privateEntity();
        NodeId f(1);

        for (int i = 1; i <= n; ++i) {
            table.initRow(i, 1);
            table[i][0].branch[0] = f;
            table[i][0].branch[1] = f;
            f = NodeId(i, 0);
        }

        root_ = f;
    }

    /**
     * DD construction.
     * @param spec DD spec.
     * @param useMP use an algorithm for multiple processors.
     */
    template<typename SPEC>
    DdStructure(DdSpec<SPEC,2> const& spec, bool useMP = false) {
#ifdef _OPENMP
        if (useMP) constructMP_(spec.entity());
        else
#endif
        construct_(spec.entity());
    }

private:
    template<typename SPEC>
    void construct_(SPEC const& spec) {
        MessageHandler mh;
        mh.begin(typenameof(spec));
        DdBuilder<SPEC> zc(spec, diagram);
        int n = zc.initialize(root_);

        if (n > 0) {
            mh.setSteps(n);
            for (int i = n; i > 0; --i) {
                bool wiped = zc.wipedown(i);
                zc.construct(i);
                mh.step(wiped ? ':' : '.');
            }
        }
        else {
            mh << " ...";
        }

        mh.end(size());
    }

    template<typename SPEC>
    void constructMP_(SPEC const& spec) {
        MessageHandler mh;
        mh.begin(typenameof(spec));
        DdBuilderMP<SPEC> zc(spec, diagram);
        int n = zc.initialize(root_);

        if (n > 0) {
#ifdef _OPENMP
            mh << " " << omp_get_max_threads() << "x";
#endif
            mh.setSteps(n);
            for (int i = n; i > 0; --i) {
                bool wiped = zc.wipedown(i);
                zc.construct(i);
                mh.step(wiped ? ':' : '.');
            }
        }
        else {
            mh << " ...";
        }

        mh.end(size());
    }

public:
    /**
     * ZDD subsetting.
     * @param spec ZDD spec.
     * @param useMP use an algorithm for multiple processors.
     */
    template<typename SPEC>
    void zddSubset(DdSpec<SPEC,2> const& spec, bool useMP = false) {
#ifdef _OPENMP
        if (useMP) zddSubsetMP_(spec.entity());
        else
#endif
        zddSubset_(spec.entity());
    }

private:
    template<typename SPEC>
    void zddSubset_(SPEC const& spec) {
        MessageHandler mh;
        mh.begin(typenameof(spec));
        NodeTableHandler<2> tmpTable;
        ZddSubsetter<SPEC> zs(diagram, spec, tmpTable);
        int n = zs.initialize(root_);

        if (n > 0) {
            mh.setSteps(n);
            for (int i = n; i > 0; --i) {
                zs.subset(i);
                diagram.derefLevel(i);
                mh.step();
            }
        }
        else {
            mh << " ...";
        }

        diagram = tmpTable;
        mh.end(size());
    }

    template<typename SPEC>
    void zddSubsetMP_(SPEC const& spec) {
        MessageHandler mh;
        mh.begin(typenameof(spec));
        NodeTableHandler<2> tmpTable;
        ZddSubsetterMP<SPEC> zs(diagram, spec, tmpTable);
        int n = zs.initialize(root_);

        if (n > 0) {
#ifdef _OPENMP
            mh << " " << omp_get_max_threads() << "x";
#endif
            mh.setSteps(n);
            for (int i = n; i > 0; --i) {
                zs.subset(i);
                diagram.derefLevel(i);
                mh.step();
            }
        }
        else {
            mh << " ...";
        }

        diagram = tmpTable;
        mh.end(size());
    }

public:
    /**
     * Depth-first ZDD construction without top-down cache.
     * @param spec ZDD spec.
     */
    template<typename SPEC>
    void constructDF(DdSpec<SPEC,2> const& spec) {
        constructDF_(spec.entity());
    }

private:
    template<typename SPEC>
    void constructDF_(SPEC const& spec) {
        MessageHandler mh;
        mh.begin(typenameof(spec)) << " ...";
        DdBuilderDF<SPEC> zc(spec, diagram);
        root_ = zc.construct();
        mh.end(size());
    }

public:
    /**
     * BDD reduction.
     * The node of which two edges points to the identical node is deleted.
     * @param useMP use an algorithm for multiple processors.
     */
    void bddReduce(bool useMP = false) {
        reduce<true,false>(useMP);
    }

    /**
     * ZDD reduction.
     * The node of which 1-edge points to the 0-terminal is deleted.
     * @param useMP use an algorithm for multiple processors.
     */
    void zddReduce(bool useMP = false) {
        reduce<false,true>(useMP);
    }

    /**
     * BDD/ZDD reduction.
     * @param BDD enable BDD reduction.
     * @param ZDD enable ZDD reduction.
     * @param useMP use an algorithm for multiple processors.
     */
    template<bool BDD, bool ZDD>
    void reduce(bool useMP) {
        MessageHandler mh;
        mh.begin("reduction");
        int n = root_.row();

#ifdef _OPENMP
        if (useMP) mh << " " << omp_get_max_threads() << "x";
#endif

        DdReducer<2,BDD,ZDD> zr(diagram, useMP);
        zr.setRoot(root_);

        mh.setSteps(n);
        for (int i = 1; i <= n; ++i) {
            zr.reduce(i, useMP);
            mh.step();
        }

        mh.end(size());
    }

public:
    int getRoot(NodeId& f) const {
        f = root_;
        return (f == 1) ? -1 : f.row();
    }

    int getChild(NodeId& f, int level, int value) const {
        assert(level > 0 && level == f.row());
        assert(0 <= value && value < 2);
        f = child(f, value);
        return (f == 1) ? -1 : f.row();
    }

    size_t hashCode(NodeId const& f) const {
        return f.hash();
    }

    /**
     * Gets the root node.
     * @return root node ID.
     */
    NodeId root() const {
        return root_;
    }

    /**
     * Gets a child node.
     * @param f parent node ID.
     * @param b branch number.
     * @return child node ID.
     */
    NodeId child(NodeId f, int b) const {
        return diagram->child(f, b);
    }

    /**
     * Gets the diagram.
     * @return the node table handler.
     */
    NodeTableHandler<2>& getDiagram() {
        return diagram;
    }

    /**
     * Gets the diagram.
     * @return the node table handler.
     */
    NodeTableHandler<2> const& getDiagram() const {
        return diagram;
    }

    /**
     * Gets the number of nonterminal nodes.
     * @return the number of nonterminal nodes.
     */
    size_t size() const {
        return diagram->size();
    }

    /**
     * Gets the level of root ZDD variable.
     * @return the level of root ZDD variable.
     */
    int topLevel() const {
        return root_.row();
    }

    /**
     * Evaluates the DD from the bottom to the top.
     * @param eval the driver class that implements <tt>evalTerminal(int b, Val& v)</tt> and
     *   <tt>evalNode(int i, Val& v, int i0, Val const& v0, int i1, Val const& v1)</tt> methods.
     * @param useMP use an algorithm for multiple processors.
     * @return value at the root.
     */
    template<typename T>
    typename T::RetVal evaluate(T eval, bool useMP = false) const {
        typedef typename T::Val Val;
        int n = root_.row();
        eval.initialize(n);

        Val t0, t1;
        eval.evalTerminal(t0, 0);
        eval.evalTerminal(t1, 1);
        if (root_ == 0) return t0;
        if (root_ == 1) return t1;

        DataTable<Val> work(diagram->numRows());
        work[0].resize(2);
        work[0][0] = t0;
        work[0][1] = t1;

#ifdef _OPENMP
        int threads = useMP ? omp_get_max_threads() : 0;
        MyVector<T> evals(threads, eval);
#endif

        for (int i = 1; i <= n; ++i) {
            MyVector<Node<2> > const& node = (*diagram)[i];
            size_t const m = node.size();
            work[i].resize(m);

#ifdef _OPENMP
            if (useMP)
#pragma omp parallel
            {
                int k = omp_get_thread_num();

#pragma omp for schedule(static)
                for (size_t j = 0; j < m; ++j) {
                    NodeId f0 = node[j].branch[0];
                    NodeId f1 = node[j].branch[1];
                    evals[k].evalNode(work[i][j], i, work[f0.row()][f0.col()],
                            f0.row(), work[f1.row()][f1.col()], f1.row());
                }
            }
            else
#endif
            for (size_t j = 0; j < m; ++j) {
                NodeId f0 = node[j].branch[0];
                NodeId f1 = node[j].branch[1];
                eval.evalNode(work[i][j], i, work[f0.row()][f0.col()], f0.row(),
                        work[f1.row()][f1.col()], f1.row());
            }

            MyVector<int> const& levels = diagram->lowerLevels(i);
            for (int const* t = levels.begin(); t != levels.end(); ++t) {
                work[*t].clear();
                eval.destructLevel(*t);
#ifdef _OPENMP
                for (int k = 0; k < threads; ++k) {
                    evals[k].destructLevel(*t);
                }
#endif
            }
        }

        return eval.getValue(work[root_.row()][root_.col()]);
    }

    class const_iterator {
        struct Selection {
            NodeId node;
            bool val;

            Selection()
                    : val(false) {
            }

            Selection(NodeId node, bool val)
                    : node(node), val(val) {
            }

            bool operator==(Selection const& o) const {
                return node == o.node && val == o.val;
            }
        };

        DdStructure const& dd;
        int cursor;
        std::vector<Selection> path;
        std::vector<int> itemset;

    public:
        const_iterator(DdStructure const& dd, bool begin)
                : dd(dd), cursor(begin ? -1 : -2), path(), itemset() {
            if (begin) next(dd.root_);
        }

        const_iterator& operator++() {
            next(NodeId(0, 0));
            return *this;
        }

        std::vector<int> const& operator*() const {
            return itemset;
        }

        std::vector<int> const* operator->() const {
            return &itemset;
        }

        bool operator==(const_iterator const& o) const {
            return cursor == o.cursor && path == o.path;
        }

        bool operator!=(const_iterator const& o) const {
            return !operator==(o);
        }

    private:
        void next(NodeId f) {
            for (;;) {
                while (f != 0) { /* down */
                    if (f == 1) return;
                    Node<2> const& s = (*dd.diagram)[f.row()][f.col()];

                    if (s.branch[0] != 0) {
                        cursor = path.size();
                        path.push_back(Selection(f, false));
                        f = s.branch[0];
                    }
                    else {
                        itemset.push_back(f.row());
                        path.push_back(Selection(f, true));
                        f = s.branch[1];
                    }
                }

                for (; cursor >= 0; --cursor) { /* up */
                    Selection& sel = path[cursor];
                    Node<2> const& ss =
                            (*dd.diagram)[sel.node.row()][sel.node.col()];
                    if (sel.val == false && ss.branch[1] != 0) {
                        f = sel.node;
                        sel.val = true;
                        int i = f.row();
                        path.resize(cursor + 1);
                        while (!itemset.empty() && itemset.back() <= i) {
                            itemset.pop_back();
                        }
                        itemset.push_back(i);
                        f = dd.diagram->child(f, 1);
                        break;
                    }
                }

                if (cursor < 0) { /* end() state */
                    cursor = -2;
                    path.clear();
                    itemset.clear();
                    return;
                }
            }
        }
    };

    const_iterator begin() const {
        return const_iterator(*this, true);
    }

    const_iterator end() const {
        return const_iterator(*this, false);
    }

    /**
     * Dumps the node table in Sapporo ZDD format.
     * @param os the output stream.
     */
    void dumpSapporo(std::ostream& os) const {
        int const n = diagram->numRows() - 1;
        size_t const l = size();

        os << "_i " << n << "\n";
        os << "_o 1\n";
        os << "_n " << l << "\n";

        DataTable<size_t> nodeId(diagram->numRows());
        size_t k = 0;

        for (int i = 1; i <= n; ++i) {
            size_t const m = (*diagram)[i].size();
            Node<2> const* p = (*diagram)[i].data();
            nodeId[i].resize(m);

            for (size_t j = 0; j < m; ++j, ++p) {
                k += 2;
                nodeId[i][j] = k;
                os << k << " " << i;

                for (int c = 0; c <= 1; ++c) {
                    NodeId fc = p->branch[c];
                    if (fc == 0) {
                        os << " F";
                    }
                    else if (fc == 1) {
                        os << " T";
                    }
                    else {
                        os << " " << nodeId[fc.row()][fc.col()];
                    }
                }

                os << "\n";
            }

            MyVector<int> const& levels = diagram->lowerLevels(i);
            for (int const* t = levels.begin(); t != levels.end(); ++t) {
                nodeId[*t].clear();
            }
        }

        os << nodeId[root_.row()][root_.col()] << "\n";
        assert(k == l * 2);
    }
};

} // namespace tdzdd
