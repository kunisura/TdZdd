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

#include <cassert>
#include <cmath>
#include <ostream>
#include <stdexcept>

#ifdef _OPENMP
#include <omp.h>
#endif

#include "DdSweeper.hpp"
#include "Node.hpp"
#include "NodeTable.hpp"
#include "../util/MemoryPool.hpp"
#include "../util/MessageHandler.hpp"
#include "../util/MyHashTable.hpp"
#include "../util/MyList.hpp"
#include "../util/MyVector.hpp"

namespace tdzdd {

class DdBuilderBase {
protected:
    static int const headerSize = 1;

    /* SpecNode
     * ┌────────┬────────┬────────┬─────
     * │ srcPtr │state[0]│state[1]│ ...
     * │ nodeId │        │        │
     * └────────┴────────┴────────┴─────
     */
    union SpecNode {
        NodeId* srcPtr;
        int64_t code;
    };

    static NodeId*& srcPtr(SpecNode* p) {
        return p[0].srcPtr;
    }

    static int64_t& code(SpecNode* p) {
        return p[0].code;
    }

    static NodeId& nodeId(SpecNode* p) {
        return *reinterpret_cast<NodeId*>(&p[0].code);
    }

    static void* state(SpecNode* p) {
        return p + headerSize;
    }

    static void const* state(SpecNode const* p) {
        return p + headerSize;
    }

    static int getSpecNodeSize(int n) {
        if (n < 0)
            throw std::runtime_error("storage size is not initialized!!!");
        return headerSize + (n + sizeof(SpecNode) - 1) / sizeof(SpecNode);
    }

    template<typename SPEC>
    struct Hasher {
        SPEC const& spec;
        int const level;

        Hasher(SPEC const& spec, int level) :
                spec(spec), level(level) {
        }

        size_t operator()(SpecNode const* p) const {
            return spec.hash_code(state(p), level);
        }

        size_t operator()(SpecNode const* p, SpecNode const* q) const {
            return spec.equal_to(state(p), state(q), level);
        }
    };
};

class DdBuilderMPBase {
protected:
    static int const headerSize = 2;

    /* SpecNode
     * ┌────────┬────────┬────────┬────────┬─────
     * │ srcPtr │ nodeId │state[0]│state[1]│ ...
     * └────────┴────────┴────────┴────────┴─────
     */
    union SpecNode {
        NodeId* srcPtr;
        int64_t code;
    };

//    /*
//     * NodeProperty of input node table
//     */
//    struct Work {
//        uint16_t parentRow;
//        uint16_t nextRow[2];
//        size_t parentCol;
//        size_t nextCol[2];
//        size_t numSibling;
//        size_t outColBase;
//        MyListOnPool<SpecNode> snodes[2];
//
//        Work()
//                : parentRow(), nextRow(), parentCol(), nextCol(), numSibling(0),
//                  outColBase() {
//        }
//    };

    static NodeId*& srcPtr(SpecNode* p) {
        return p[0].srcPtr;
    }

    static int64_t& code(SpecNode* p) {
        return p[1].code;
    }

    static NodeId& nodeId(SpecNode* p) {
        return *reinterpret_cast<NodeId*>(&p[1].code);
    }

    static NodeId nodeId(SpecNode const* p) {
        return *reinterpret_cast<NodeId const*>(&p[1].code);
    }

    static int row(SpecNode const* p) {
        return nodeId(p).row();
    }

    static size_t col(SpecNode const* p) {
        return nodeId(p).col();
    }

    static void* state(SpecNode* p) {
        return p + headerSize;
    }

    static void const* state(SpecNode const* p) {
        return p + headerSize;
    }

    static int getSpecNodeSize(int n) {
        if (n < 0)
            throw std::runtime_error("storage size is not initialized!!!");
        return headerSize + (n + sizeof(SpecNode) - 1) / sizeof(SpecNode);
    }

    template<typename SPEC>
    struct Hasher {
        SPEC const& spec;
        int const level;

        Hasher(SPEC const& spec, int level) :
                spec(spec), level(level) {
        }

        size_t operator()(SpecNode const* p) const {
            return spec.hash_code(state(p), level);
        }

        size_t operator()(SpecNode const* p, SpecNode const* q) const {
            return spec.equal_to(state(p), state(q), level);
        }
    };
};

/**
 * Basic breadth-first DD builder.
 */
template<typename S>
class DdBuilder: DdBuilderBase {
    typedef S Spec;
    typedef MyHashTable<SpecNode*,Hasher<Spec>,Hasher<Spec> > UniqTable;
    static int const AR = Spec::ARITY;

    Spec spec;
    int const specNodeSize;
    NodeTableEntity<AR>& output;
    DdSweeper<AR> sweeper;

    MyVector<MyList<SpecNode> > snodeTable;

    void init(int n) {
        snodeTable.resize(n + 1);
        if (n >= output.numRows()) output.setNumRows(n + 1);
    }

public:
    DdBuilder(Spec const& s, NodeTableHandler<AR>& output, int n = 0) :
            spec(s), specNodeSize(getSpecNodeSize(spec.datasize())),
            output(output.privateEntity()),
            sweeper(this->output) {
        if (n >= 1) init(n);
    }

    /**
     * Schedules a top-down event.
     * @param fp result storage.
     * @param level node level of the event.
     * @param s node state of the event.
     */
    void schedule(NodeId* fp, int level, void* s) {
        SpecNode* p0 = snodeTable[level].alloc_front(specNodeSize);
        spec.get_copy(state(p0), s);
        srcPtr(p0) = fp;
    }

    /**
     * Initializes the builder.
     * @param root result storage.
     */
    int initialize(NodeId& root) {
        sweeper.setRoot(root);
        MyVector<SpecNode> tmp(specNodeSize);
        SpecNode* ptmp = tmp.data();
        int n = spec.get_root(state(ptmp));

        if (n <= 0) {
            root = n ? 1 : 0;
            n = 0;
        }
        else {
            init(n);
            schedule(&root, n, state(ptmp));
        }

        spec.destruct(state(ptmp));
        return n;
    }

    /**
     * Builds one level.
     * @param i level.
     */
    void construct(int i) {
        assert(0 < i && size_t(i) < snodeTable.size());

        MyList<SpecNode>& snodes = snodeTable[i];
        size_t j0 = output[i].size();
        size_t m = j0;
        int lowestChild = i - 1;
        size_t deadCount = 0;

        {
            Hasher<Spec> hasher(spec, i);
            UniqTable uniq(snodes.size() * 2, hasher, hasher);

            for (MyList<SpecNode>::iterator t = snodes.begin();
                    t != snodes.end(); ++t) {
                SpecNode* p = *t;
                SpecNode* pp = uniq.add(p);

                if (pp == p) {
                    nodeId(p) = *srcPtr(p) = NodeId(i, m++);
                }
                else {
                    *srcPtr(p) = nodeId(pp);
                    nodeId(p) = 0;
                }
            }
//#ifdef DEBUG
//            MessageHandler mh;
//            mh << "table_size[" << i << "] = " << uniq.tableSize() << "\n";
//#endif
        }

        output[i].resize(m);
        Node<AR>* q = output[i].data() + j0;
        SpecNode* pp = snodeTable[i - 1].alloc_front(specNodeSize);

        for (; !snodes.empty(); snodes.pop_front()) {
            SpecNode* p = snodes.front();
            if (nodeId(p) == 0) {
                spec.destruct(state(p));
                continue;
            }

            bool allZero = true;

            for (int b = 0; b < AR; ++b) {
                spec.get_copy(state(pp), state(p));
                int ii = spec.get_child(state(pp), i, b);

                if (ii <= 0) {
                    q->branch[b] = ii ? 1 : 0;
                    spec.destruct(state(pp));
                    if (ii) allZero = false;
                }
                else if (ii == i - 1) {
                    srcPtr(pp) = &q->branch[b];
                    pp = snodeTable[ii].alloc_front(specNodeSize);
                    allZero = false;
                }
                else {
                    assert(ii < i - 1);
                    SpecNode* ppp = snodeTable[ii].alloc_front(specNodeSize);
                    spec.get_copy(state(ppp), state(pp));
                    spec.destruct(state(pp));
                    srcPtr(ppp) = &q->branch[b];
                    if (ii < lowestChild) lowestChild = ii;
                    allZero = false;
                }
            }

            spec.destruct(state(p));
            ++q;
            if (allZero) ++deadCount;
        }

        assert(q == output[i].data() + m);
        snodeTable[i - 1].pop_front();
        spec.destructLevel(i);
        sweeper.update(i, lowestChild, deadCount);
    }
};

/**
 * Multi-threaded breadth-first DD builder.
 */
template<typename S>
class DdBuilderMP: DdBuilderMPBase {
    typedef S Spec;
    typedef MyHashTable<SpecNode*,Hasher<Spec>,Hasher<Spec> > UniqTable;
    static int const AR = Spec::ARITY;
    static int const TASKS_PER_THREAD = 10;

    int const threads;
    int const tasks;

    MyVector<Spec> specs;
    int const specNodeSize;
    NodeTableEntity<AR>& output;
    DdSweeper<AR> sweeper;

    MyVector<MyVector<MyVector<MyList<SpecNode> > > > snodeTables;

#ifdef DEBUG
    ElapsedTimeCounter etcP1, etcP2, etcS1;
#endif

    void init(int n) {
        for (int y = 0; y < threads; ++y) {
            snodeTables[y].resize(tasks);
            for (int x = 0; x < tasks; ++x) {
                snodeTables[y][x].resize(n + 1);
            }
        }
        if (n >= output.numRows()) output.setNumRows(n + 1);
    }

public:
    DdBuilderMP(Spec const& s, NodeTableHandler<AR>& output, int n = 0) :
#ifdef _OPENMP
            threads(omp_get_max_threads()),
            tasks(MyHashConstant::primeSize(TASKS_PER_THREAD * threads)),
            #else
            threads(1),
            tasks(1),
#endif
            specs(threads, s),
            specNodeSize(getSpecNodeSize(s.datasize())),
            output(output.privateEntity()),
            sweeper(this->output),
            snodeTables(threads) {
        if (n >= 1) init(n);
#ifdef DEBUG
        MessageHandler mh;
        mh << "#thread = " << threads << ", #task = " << tasks;
#endif
    }

#ifdef DEBUG
    ~DdBuilderMP() {
        MessageHandler mh;
        mh << "P1: " << etcP1 << "\n";
        mh << "P2: " << etcP2 << "\n";
        mh << "S1: " << etcS1 << "\n";
    }
#endif

    /**
     * Schedules a top-down event.
     * @param fp result storage.
     * @param level node level of the event.
     * @param s node state of the event.
     */
    void schedule(NodeId* fp, int level, void* s) {
        SpecNode* p0 = snodeTables[0][0][level].alloc_front(specNodeSize);
        specs[0].get_copy(state(p0), s);
        srcPtr(p0) = fp;
    }

    /**
     * Initializes the builder.
     * @param root result storage.
     */
    int initialize(NodeId& root) {
        sweeper.setRoot(root);
        MyVector<SpecNode> tmp(specNodeSize);
        SpecNode* ptmp = tmp.data();
        int n = specs[0].get_root(state(ptmp));

        if (n <= 0) {
            root = n ? 1 : 0;
            n = 0;
        }
        else {
            init(n);
            schedule(&root, n, state(ptmp));
        }

        specs[0].destruct(state(ptmp));
        return n;
    }

    /**
     * Builds one level.
     * @param i level.
     */
    void construct(int i) {
        assert(0 < i && i < output.numRows());
        assert(output.numRows() - snodeTables[0][0].size() == 0);

        MyVector<size_t> nodeColumn(tasks);
        int lowestChild = i - 1;
        size_t deadCount = 0;

#ifdef DEBUG
        etcP1.start();
#endif

#ifdef _OPENMP
#pragma omp parallel reduction(min:lowestChild) reduction(+:deadCount)
#endif
        {
#ifdef _OPENMP
            int yy = omp_get_thread_num();
            //CPUAffinity().bind(yy);
#else
            int yy = 0;
#endif

            Spec& spec = specs[yy];
            MyVector<SpecNode> tmp(specNodeSize);
            SpecNode* ptmp = tmp.data();
            Hasher<Spec> hasher(spec, i);
            UniqTable uniq(hasher, hasher);

#ifdef _OPENMP
#pragma omp for schedule(dynamic)
#endif
            for (int x = 0; x < tasks; ++x) {
                size_t m = 0;
                for (int y = 0; y < threads; ++y) {
                    m += snodeTables[y][x][i].size();
                }
                if (m == 0) continue;

                uniq.initialize(m * 2);
                size_t j = 0;

                for (int y = 0; y < threads; ++y) {
                    MyList<SpecNode>& snodes = snodeTables[y][x][i];

                    for (MyList<SpecNode>::iterator t = snodes.begin();
                            t != snodes.end(); ++t) {
                        SpecNode* p = *t;
                        SpecNode* pp = uniq.add(p);

                        if (pp == p) {
                            code(p) = j++;
                        }
                        else {
                            code(p) = ~code(pp);
                        }
                    }
                }

                nodeColumn[x] = j;
//#ifdef DEBUG
//                MessageHandler mh;
//#ifdef _OPENMP
//#pragma omp critical
//#endif
//                mh << "table_size[" << i << "][" << x << "] = " << uniq.tableSize() << "\n";
//#endif
            }

#ifdef _OPENMP
#pragma omp single
#endif
            {
#ifdef DEBUG
                etcP1.stop();
                etcS1.start();
#endif
                size_t m = output[i].size();
                for (int x = 0; x < tasks; ++x) {
                    size_t j = nodeColumn[x];
                    nodeColumn[x] = (j >= 1) ? m : -1; // -1 for skip
                    m += j;
                }

                output.initRow(i, m);
#ifdef DEBUG
                etcS1.stop();
                etcP2.start();
#endif
            }

#ifdef _OPENMP
#pragma omp for schedule(dynamic)
#endif
            for (int x = 0; x < tasks; ++x) {
                size_t j0 = nodeColumn[x];
                if (j0 + 1 == 0) continue; // -1 for skip

                for (int y = 0; y < threads; ++y) {
                    MyList<SpecNode>& snodes = snodeTables[y][x][i];

                    for (; !snodes.empty(); snodes.pop_front()) {
                        SpecNode* p = snodes.front();

                        if (code(p) < 0) {
                            *srcPtr(p) = NodeId(i, j0 + ~code(p));
                            spec.destruct(state(p));
                            continue;
                        }

                        size_t j = j0 + code(p);
                        *srcPtr(p) = NodeId(i, j);

                        Node<AR>& q = output[i][j];
                        spec.get_copy(state(ptmp), state(p));
                        void* s[2] = { state(ptmp), state(p) };
                        bool allZero = true;

                        for (int b = 0; b < AR; ++b) {
                            int ii = spec.get_child(s[b], i, b);

                            if (ii <= 0) {
                                q.branch[b] = ii ? 1 : 0;
                                if (ii) allZero = false;
                            }
                            else {
                                assert(ii <= i - 1);
                                int xx = spec.hash_code(s[b], ii) % tasks;
                                SpecNode* pp =
                                        snodeTables[yy][xx][ii].alloc_front(
                                                specNodeSize);
                                spec.get_copy(state(pp), s[b]);
                                srcPtr(pp) = &q.branch[b];
                                if (ii < lowestChild) lowestChild = ii;
                                allZero = false;
                            }

                            spec.destruct(s[b]);
                        }

                        if (allZero) ++deadCount;
                    }
                }
            }

            spec.destructLevel(i);
        }

        sweeper.update(i, lowestChild, deadCount);
#ifdef DEBUG
        etcP2.stop();
#endif
    }
};

/**
 * Another breadth-first DD builder.
 * A node table for the <I>i</I>-th level becomes available instantly
 * after @p construct(i) is called, and is destructable at any time.
 */
template<typename S, bool dumpDot = false>
class InstantDdBuilder: DdBuilderBase {
    //typedef typename std::remove_const<typename std::remove_reference<S>::type>::type Spec;
    typedef S Spec;
    typedef MyHashTable<SpecNode*,Hasher<Spec>,Hasher<Spec> > UniqTable;
    static int const AR = Spec::ARITY;

    NodeTableEntity<AR>& output;
    Spec spec;
    int const specNodeSize;
    std::ostream& os;
    bool cut;

    MyVector<MyList<SpecNode> > snodeTable;
    MyVector<UniqTable> uniqTable;
    MyVector<Hasher<Spec> > hasher;
    NodeId top;

public:
    InstantDdBuilder(Spec const& s, NodeTableHandler<AR>& output,
            std::ostream& os = std::cout, bool cut = false) :
            output(output.privateEntity()), spec(s),
            specNodeSize(getSpecNodeSize(spec.datasize())),
            os(os), cut(cut) {
    }

    /**
     * Initializes the builder.
     * @param root the root node.
     */
    void initialize(NodeId& root) {
        MyVector<SpecNode> tmp(specNodeSize);
        SpecNode* ptmp = tmp.data();
        int n = spec.get_root(state(ptmp));

        if (n <= 0) {
            root = n ? 1 : 0;
            n = 0;
        }
        else {
            root = NodeId(n, 0);
            snodeTable.resize(n + 1);
            SpecNode* p0 = snodeTable[n].alloc_front(specNodeSize);
            spec.get_copy(state(p0), state(ptmp));

            uniqTable.reserve(n + 1);
            hasher.reserve(n + 1);
            for (int i = 0; i <= n; ++i) {
                hasher.push_back(Hasher<Spec>(spec, i));
                uniqTable.push_back(UniqTable(hasher.back(), hasher.back()));
            }
        }

        spec.destruct(state(ptmp));
        output.init(n + 1);
        top = root;
    }

    /**
     * Builds one level.
     * @param i level.
     */
    void construct(int i) {
        if (i <= 0) return;
        assert(i < output.numRows());
        assert(output.numRows() - snodeTable.size() == 0);

        MyList<SpecNode>& snodes = snodeTable[i];
        size_t m = snodes.size();
        output.initRow(i, m);
        Node<AR>* q = output[i].data() + m; // reverse of snodeTable
        SpecNode* pp = snodeTable[i - 1].alloc_front(specNodeSize);

        for (; !snodes.empty(); snodes.pop_front()) {
            SpecNode* p = snodes.front();
            --q;

            if (dumpDot) {
                NodeId f(i, q - output[i].data());

                if (cut && f == top) {
                    os << "  \"" << f << "^\" [shape=none,label=\"\"];\n";
                    os << "  \"" << f << "^\" -> \"" << f << "\" [style=dashed";
                    os << "];\n";
                }
                os << "  \"" << f << "\" [label=\"";
                spec.print_state(os, state(p));
                os << "\"];\n";
            }

            for (int b = 0; b < AR; ++b) {
                spec.get_copy(state(pp), state(p));
                int ii = spec.get_child(state(pp), i, b);

                if (ii <= 0) {
                    q->branch[b] = ii ? 1 : 0;
                    spec.destruct(state(pp));
                }
                else if (ii == i - 1) {
                    SpecNode* pp1 = uniqTable[ii].add(pp);
                    if (pp1 == pp) {
                        size_t jj = snodeTable[ii].size() - 1;
                        nodeId(pp1) = NodeId(ii, jj);
                        pp = snodeTable[ii].alloc_front(specNodeSize);
                    }
                    else {
                        spec.destruct(state(pp));
                    }
                    q->branch[b] = nodeId(pp1);
                }
                else {
                    assert(ii < i - 1);
                    SpecNode* pp2 = snodeTable[ii].alloc_front(specNodeSize);
                    spec.get_copy(state(pp2), state(pp));
                    spec.destruct(state(pp));

                    SpecNode* pp1 = uniqTable[ii].add(pp2);
                    if (pp1 == pp2) {
                        size_t j = snodeTable[ii].size() - 1;
                        nodeId(pp1) = NodeId(ii, j);
                    }
                    else {
                        spec.destruct(state(pp2));
                        snodeTable[ii].pop_front();
                    }
                    q->branch[b] = nodeId(pp1);
                }

                if (dumpDot) {
                    NodeId f(i, q - output[i].data());
                    NodeId ff = q->branch[b];
                    if (ff == 0) continue;

                    if (cut && ff == 1) {
                        os << "  \"" << f
                                << "$\" [shape=square,fixedsize=true,width=0.2,label=\"\"];\n";
                        os << "  \"" << f << "\" -> \"" << f << "$\"";
                    }
                    else if (cut && f == top && b == 0) {
                        top = ff;
                        continue;
                    }
                    else {
                        os << "  \"" << f << "\" -> \"" << ff << "\"";
                    }

                    os << " [style=";
                    if (b == 0) {
                        os << "dashed";
                    }
                    else {
                        os << "solid";
                        if (AR > 2) {
                            os << ",color="
                                    << ((b == 1) ? "blue" :
                                        (b == 2) ? "red" : "green");
                        }
                    }
                    os << "];\n";
                }
            }

            spec.destruct(state(p));
        }

        if (dumpDot) {
            os << "  {rank=same; " << i;
            for (size_t j = 0; j < m; ++j) {
                os << "; \"" << NodeId(i, j) << "\"";
            }
            os << "}\n";
        }

        assert(q == output[i].data());
        snodeTable[i - 1].pop_front();
        uniqTable[i - 1].clear();
        spec.destructLevel(i);
    }
};

/**
 * Breadth-first ZDD subset builder.
 */
template<typename S>
class ZddSubsetter: DdBuilderBase {
    //typedef typename std::remove_const<typename std::remove_reference<S>::type>::type Spec;
    typedef S Spec;
    typedef MyHashTable<SpecNode*,Hasher<Spec>,Hasher<Spec> > UniqTable;
    static int const AR = Spec::ARITY;

    NodeTableEntity<AR> const& input;
    NodeTableEntity<AR>& output;
    Spec spec;
    int const specNodeSize;
    DataTable<MyListOnPool<SpecNode> > work;
    DdSweeper<AR> sweeper;

    MyVector<SpecNode> tmp;
    MemoryPools pools;

public:
    ZddSubsetter(NodeTableHandler<AR> const& input, Spec const& s,
            NodeTableHandler<AR>& output) :
            input(*input), output(output.privateEntity()), spec(s),
            specNodeSize(getSpecNodeSize(spec.datasize())),
            work(input->numRows()), sweeper(this->output) {
    }

    /**
     * Initializes the builder.
     * @param root the root node.
     */
    int initialize(NodeId& root) {
        sweeper.setRoot(root);
        tmp.resize(specNodeSize);
        SpecNode* ptmp = tmp.data();
        int n = spec.get_root(state(ptmp));

        int k = (root == 1) ? -1 : root.row();

        while (n != 0 && k != 0 && n != k) {
            if (n < k) {
                assert(k >= 1);
                k = downTable(root, 0, n);
            }
            else {
                assert(n >= 1);
                n = downSpec(state(ptmp), n, 0, k);
            }
        }

        if (n <= 0 || k <= 0) {
            assert(n == 0 || k == 0 || (n == -1 && k == -1));
            root = NodeId(0, n != 0 && k != 0);
            n = 0;
        }
        else {
            assert(n == k);
            assert(n == root.row());

            pools.resize(n + 1);
            work[n].resize(input[n].size());

            SpecNode* p0 = work[n][root.col()].alloc_front(pools[n],
                    specNodeSize);
            spec.get_copy(state(p0), state(ptmp));
            srcPtr(p0) = &root;
        }

        spec.destruct(state(ptmp));
        output.init(n + 1);
        return n;
    }

    /**
     * Builds one level.
     * @param i level.
     */
    void subset(int i) {
        assert(0 < i && i < output.numRows());
        assert(output.numRows() - pools.size() == 0);

        Hasher<Spec> const hasher(spec, i);
        SpecNode* const ptmp = tmp.data();
        size_t const m = input[i].size();
        size_t mm = 0;
        int lowestChild = i - 1;
        size_t deadCount = 0;

        if (work[i].empty()) work[i].resize(m);
        assert(work[i].size() == m);

        for (size_t j = 0; j < m; ++j) {
            MyListOnPool<SpecNode>& list = work[i][j];
            size_t n = list.size();

            if (n >= 2) {
                UniqTable uniq(n * 2, hasher, hasher);

                for (MyListOnPool<SpecNode>::iterator t = list.begin();
                        t != list.end(); ++t) {
                    SpecNode* p = *t;
                    SpecNode* pp = uniq.add(p);

                    if (pp == p) {
                        nodeId(p) = *srcPtr(p) = NodeId(i, mm++);
                    }
                    else {
                        *srcPtr(p) = nodeId(pp);
                        nodeId(p) = 0;
                    }
                }
            }
            else if (n == 1) {
                SpecNode* p = list.front();
                nodeId(p) = *srcPtr(p) = NodeId(i, mm++);
            }
        }

        output.initRow(i, mm);
        Node<AR>* q = output[i].data();

        for (size_t j = 0; j < m; ++j) {
            MyListOnPool<SpecNode>& list = work[i][j];

            for (MyListOnPool<SpecNode>::iterator t = list.begin();
                    t != list.end(); ++t) {
                SpecNode* p = *t;
                if (nodeId(p) == 0) {
                    spec.destruct(state(p));
                    continue;
                }

                spec.get_copy(state(ptmp), state(p));
                void* s[2] = { state(ptmp), state(p) };
                bool allZero = true;

                for (int b = 0; b < AR; ++b) {
                    NodeId f(i, j);
                    int kk = downTable(f, b, i - 1);
                    int ii = downSpec(s[b], i, b, kk);

                    while (ii != 0 && kk != 0 && ii != kk) {
                        if (ii < kk) {
                            assert(kk >= 1);
                            kk = downTable(f, 0, ii);
                        }
                        else {
                            assert(ii >= 1);
                            ii = downSpec(s[b], ii, 0, kk);
                        }
                    }

                    if (ii <= 0 || kk <= 0) {
                        bool val = ii != 0 && kk != 0;
                        q->branch[b] = val;
                        if (val) allZero = false;
                    }
                    else {
                        assert(ii == f.row() && ii == kk && ii < i);
                        if (work[ii].empty()) work[ii].resize(input[ii].size());
                        SpecNode* pp = work[ii][f.col()].alloc_front(pools[ii],
                                specNodeSize);
                        spec.get_copy(state(pp), s[b]);
                        srcPtr(pp) = &q->branch[b];
                        if (ii < lowestChild) lowestChild = ii;
                        allZero = false;
                    }
                }

                spec.destruct(state(p));
                spec.destruct(state(ptmp));
                ++q;
                if (allZero) ++deadCount;
            }
        }

        assert(q == output[i].data() + mm);
        work[i].clear();
        pools[i].clear();
        spec.destructLevel(i);
        sweeper.update(i, lowestChild, deadCount);
    }

private:
    int downTable(NodeId& f, int b, int zerosupLevel) const {
        if (zerosupLevel < 0) zerosupLevel = 0;

        f = input.child(f, b);
        while (f.row() > zerosupLevel) {
            f = input.child(f, 0);
        }
        return (f == 1) ? -1 : f.row();
    }

    int downSpec(void* p, int level, int b, int zerosupLevel) {
        if (zerosupLevel < 0) zerosupLevel = 0;
        assert(level > zerosupLevel);

        int i = spec.get_child(p, level, b);
        while (i > zerosupLevel) {
            i = spec.get_child(p, i, 0);
        }
        return i;
    }
};

/**
 * Multi-threaded breadth-first ZDD subset builder.
 */
template<typename S>
class ZddSubsetterMP: DdBuilderMPBase {
    //typedef typename std::remove_const<typename std::remove_reference<S>::type>::type Spec;
    typedef S Spec;
    typedef MyHashTable<SpecNode*,Hasher<Spec>,Hasher<Spec> > UniqTable;
    static int const AR = Spec::ARITY;

    int const threads;

    MyVector<Spec> specs;
    int const specNodeSize;
    NodeTableEntity<AR> const& input;
    NodeTableEntity<AR>& output;
    DdSweeper<AR> sweeper;

    MyVector<MyVector<MyVector<MyListOnPool<SpecNode> > > > snodeTables;
    MyVector<MemoryPools> pools;

public:
    ZddSubsetterMP(NodeTableHandler<AR> const& input, Spec const& s,
            NodeTableHandler<AR>& output) :
#ifdef _OPENMP
            threads(omp_get_max_threads()),
#else
            threads(1),
#endif
            specs(threads, s),
            specNodeSize(getSpecNodeSize(s.datasize())), input(*input),
            output(output.privateEntity()),
            sweeper(this->output),
            snodeTables(threads),
            pools(threads) {
    }

    /**
     * Initializes the builder.
     * @param root the root node.
     */
    int initialize(NodeId& root) {
        sweeper.setRoot(root);
        MyVector<SpecNode> tmp(specNodeSize);
        SpecNode* ptmp = tmp.data();
        Spec& spec = specs[0];
        int n = spec.get_root(state(ptmp));

        int k = (root == 1) ? -1 : root.row();

        while (n != 0 && k != 0 && n != k) {
            if (n < k) {
                assert(k >= 1);
                k = downTable(root, 0, n);
            }
            else {
                assert(n >= 1);
                n = downSpec(spec, state(ptmp), n, 0, k);
            }
        }

        if (n <= 0 || k <= 0) {
            assert(n == 0 || k == 0 || (n == -1 && k == -1));
            root = NodeId(0, n != 0 && k != 0);
            n = 0;
        }
        else {
            assert(n == k);
            assert(n == root.row());

            for (int y = 0; y < threads; ++y) {
                snodeTables[y].resize(n + 1);
                pools[y].resize(n + 1);
            }

            snodeTables[0][n].resize(input[n].size());
            SpecNode* p0 = snodeTables[0][n][root.col()].alloc_front(
                    pools[0][n], specNodeSize);
            spec.get_copy(state(p0), state(ptmp));
            srcPtr(p0) = &root;
        }

        spec.destruct(state(ptmp));
        output.init(n + 1);
        return n;
    }

    /**
     * Builds one level.
     * @param i level.
     */
    void subset(int i) {
        assert(0 < i && i < output.numRows());
        size_t const m = input[i].size();

        MyVector<size_t> nodeColumn(m);
        int lowestChild = i - 1;
        size_t deadCount = 0;

#ifdef _OPENMP
#pragma omp parallel reduction(min:lowestChild) reduction(+:deadCount)
#endif
        {
#ifdef _OPENMP
            int yy = omp_get_thread_num();
#else
            int yy = 0;
#endif
            Spec& spec = specs[yy];
            MyVector<SpecNode> tmp(specNodeSize);
            SpecNode* ptmp = tmp.data();
            Hasher<Spec> hasher(spec, i);
            UniqTable uniq(hasher, hasher);

#ifdef _OPENMP
#pragma omp for schedule(dynamic)
#endif
            for (size_t j = 0; j < m; ++j) {
                size_t mm = 0;
                for (int y = 0; y < threads; ++y) {
                    if (snodeTables[y][i].empty()) continue;
                    MyListOnPool<SpecNode>& snodes = snodeTables[y][i][j];
                    mm += snodes.size();
                }
                uniq.initialize(mm * 2);
                size_t jj = 0;

                for (int y = 0; y < threads; ++y) {
                    if (snodeTables[y][i].empty()) continue;
                    MyListOnPool<SpecNode>& snodes = snodeTables[y][i][j];

                    for (MyListOnPool<SpecNode>::iterator t = snodes.begin();
                            t != snodes.end(); ++t) {
                        SpecNode* p = *t;
                        SpecNode* pp = uniq.add(p);

                        if (pp == p) {
                            code(p) = jj++;
                        }
                        else {
                            code(p) = ~code(pp);
                        }
                    }
                }

                nodeColumn[j] = jj;
            }

#ifdef _OPENMP
#pragma omp single
#endif
            {
                size_t mm = 0;
                for (size_t j = 0; j < m; ++j) {
                    size_t jj = nodeColumn[j];
                    nodeColumn[j] = mm;
                    mm += jj;
                }

                output.initRow(i, mm);
            }

#ifdef _OPENMP
#pragma omp for schedule(dynamic)
#endif
            for (size_t j = 0; j < m; ++j) {
                size_t const jj0 = nodeColumn[j];

                for (int y = 0; y < threads; ++y) {
                    if (snodeTables[y][i].empty()) continue;

                    MyListOnPool<SpecNode>& snodes = snodeTables[y][i][j];

                    for (MyListOnPool<SpecNode>::iterator t = snodes.begin();
                            t != snodes.end(); ++t) {
                        SpecNode* p = *t;

                        if (code(p) < 0) {
                            *srcPtr(p) = NodeId(i, jj0 + ~code(p));
                            spec.destruct(state(p));
                            continue;
                        }

                        size_t const jj = jj0 + code(p);
                        *srcPtr(p) = NodeId(i, jj);
                        Node<AR>& q = output[i][jj];
                        spec.get_copy(state(ptmp), state(p));
                        void* s[2] = { state(ptmp), state(p) };
                        bool allZero = true;

                        for (int b = 0; b < AR; ++b) {
                            NodeId f(i, j);
                            int kk = downTable(f, b, i - 1);
                            int ii = downSpec(spec, s[b], i, b, kk);

                            while (ii != 0 && kk != 0 && ii != kk) {
                                if (ii < kk) {
                                    assert(kk >= 1);
                                    kk = downTable(f, 0, ii);
                                }
                                else {
                                    assert(ii >= 1);
                                    ii = downSpec(spec, s[b], ii, 0, kk);
                                }
                            }

                            if (ii <= 0 || kk <= 0) {
                                bool val = ii != 0 && kk != 0;
                                q.branch[b] = val;
                                if (val) allZero = false;
                            }
                            else {
                                assert(ii == f.row() && ii == kk && ii < i);
                                size_t jj = f.col();

                                if (snodeTables[yy][ii].empty()) {
                                    snodeTables[yy][ii].resize(
                                            input[ii].size());
                                }

                                SpecNode* pp =
                                        snodeTables[yy][ii][jj].alloc_front(
                                                pools[yy][ii], specNodeSize);
                                spec.get_copy(state(pp), s[b]);
                                srcPtr(pp) = &q.branch[b];
                                if (ii < lowestChild) lowestChild = ii;
                                allZero = false;
                            }

                            spec.destruct(s[b]);
                        }

                        if (allZero) ++deadCount;
                    }
                }
            }

            snodeTables[yy][i].clear();
            pools[yy][i].clear();
            spec.destructLevel(i);
        }

        sweeper.update(i, lowestChild, deadCount);
    }

private:
    int downTable(NodeId& f, int b, int zerosupLevel) const {
        if (zerosupLevel < 0) zerosupLevel = 0;

        f = input.child(f, b);
        while (f.row() > zerosupLevel) {
            f = input.child(f, 0);
        }
        return (f == 1) ? -1 : f.row();
    }

    int downSpec(Spec& spec, void* p, int level, int b, int zerosupLevel) {
        if (zerosupLevel < 0) zerosupLevel = 0;
        assert(level > zerosupLevel);

        int i = spec.get_child(p, level, b);
        while (i > zerosupLevel) {
            i = spec.get_child(p, i, 0);
        }
        return i;
    }
};

} // namespace tdzdd
