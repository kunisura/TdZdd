/*
 * Top-Down ZDD Construction Library for Frontier-Based Search
 * by Hiroaki Iwashita <iwashita@erato.ist.hokudai.ac.jp>
 * Copyright (c) 2012 Japan Science and Technology Agency
 * $Id: DdBuilder.hpp 518 2014-03-18 05:29:23Z iwashita $
 */

#pragma once

#include <cassert>
#include <cmath>
#include <ostream>
#include <stdexcept>

#ifdef _OPENMP
#include <omp.h>
#endif

#include "Node.hpp"
#include "NodeTable.hpp"
//#include "../util/CPUAffinity.hpp"
#include "../util/MemoryPool.hpp"
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
        if (n < 0) throw std::runtime_error(
                "storage size is not initialized!!!");
        return headerSize + (n + sizeof(SpecNode) - 1) / sizeof(SpecNode);
    }

    template<typename SPEC>
    struct Hasher {
        SPEC const& spec;
        int const level;

        Hasher(SPEC const& spec, int level)
                : spec(spec), level(level) {
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
        if (n < 0) throw std::runtime_error(
                "storage size is not initialized!!!");
        return headerSize + (n + sizeof(SpecNode) - 1) / sizeof(SpecNode);
    }

    template<typename SPEC>
    struct Hasher {
        SPEC const& spec;
        int const level;

        Hasher(SPEC const& spec, int level)
                : spec(spec), level(level) {
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
 * Basic top-down DD builder.
 */
template<typename S>
class DdBuilder: DdBuilderBase {
    typedef S Spec;
    typedef MyHashTable<SpecNode*,Hasher<Spec>,Hasher<Spec> > UniqTable;
    static int const AR = Spec::ARITY;

    Spec spec;
    int const specNodeSize;
    NodeTableEntity<AR>& output;

    MyVector<MyList<SpecNode> > snodeTable;

    void init(int n) {
        snodeTable.resize(n + 1);
        if (n >= output.numRows()) output.setNumRows(n + 1);
    }

public:
    DdBuilder(Spec const& s, NodeTableHandler<AR>& output, int n = 0)
            : spec(s), specNodeSize(getSpecNodeSize(spec.datasize())),
              output(output.privateEntity()) {
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

            for (int b = 0; b < AR; ++b) {
                spec.get_copy(state(pp), state(p));
                int ii = spec.get_child(state(pp), i, b);

                if (ii <= 0) {
                    q->branch[b] = ii ? 1 : 0;
                    spec.destruct(state(pp));
                }
                else if (ii == i - 1) {
                    srcPtr(pp) = &q->branch[b];
                    pp = snodeTable[ii].alloc_front(specNodeSize);
                }
                else {
                    assert(ii < i - 1);
                    SpecNode* ppp = snodeTable[ii].alloc_front(specNodeSize);
                    spec.get_copy(state(ppp), state(pp));
                    spec.destruct(state(pp));
                    srcPtr(ppp) = &q->branch[b];
                }
            }

            spec.destruct(state(p));
            ++q;
        }

        assert(q == output[i].data() + m);
        snodeTable[i - 1].pop_front();
        spec.destructLevel(i);
    }

    /**
     * Wipe down the active states if needed.
     * @param i level.
     * @return true if wiped.
     */
    bool wipedown(int i) {
        if (!spec.needWipedown(i)) return false;

        for (int ii = i; ii >= 1; --ii) {
            MyList<SpecNode>& sii = snodeTable[ii];

            for (MyList<SpecNode>::iterator t = sii.begin(); t != sii.end();
                    ++t) {
                spec.set_wipedown_root(state(*t), ii);
            }
        }

        spec.doWipedown(i);
        return true;
    }
};

/**
 * Multi-threaded top-down DD builder.
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
    DdBuilderMP(Spec const& s, NodeTableHandler<AR>& output, int n = 0)
            :
#ifdef _OPENMP
              threads(omp_get_max_threads()),
              tasks(MyHashConstant::primeSize(TASKS_PER_THREAD * threads)),
#else
              threads(1),
              tasks(1),
#endif

              specs(threads, s),
              specNodeSize(getSpecNodeSize(s.datasize())),
              output(output.privateEntity()), snodeTables(threads) {
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

#ifdef DEBUG
        etcP1.start();
#endif

#ifdef _OPENMP
#pragma omp parallel
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
                        void* s[2] = {state(ptmp), state(p)};

                        for (int b = 0; b < AR; ++b) {
                            int ii = spec.get_child(s[b], i, b);

                            if (ii <= 0) {
                                q.branch[b] = ii ? 1 : 0;
                            }
                            else {
                                assert(ii <= i - 1);
                                int xx = spec.hash_code(s[b], ii) % tasks;
                                SpecNode* pp =
                                        snodeTables[yy][xx][ii].alloc_front(
                                                specNodeSize);
                                spec.get_copy(state(pp), s[b]);
                                srcPtr(pp) = &q.branch[b];
                            }

                            spec.destruct(s[b]);
                        }
                    }
                }
            }

            spec.destructLevel(i);
        }
#ifdef DEBUG
        etcP2.stop();
#endif
    }

    /**
     * Wipe down the active states if needed.
     * @param i level.
     * @return true if wiped.
     */
    bool wipedown(int i) {
        Spec& spec = specs[0];
        if (!spec.needWipedown(i)) return false;

        for (int y = 0; y < threads; ++y) {
            for (int x = 0; x < tasks; ++x) {
                for (int ii = i; ii >= 1; --ii) {
                    MyList<SpecNode>& sii = snodeTables[y][x][ii];

                    for (MyList<SpecNode>::iterator t = sii.begin();
                            t != sii.end(); ++t) {
                        spec.set_wipedown_root(state(*t), ii);
                    }
                }
            }
        }

        spec.doWipedown(i); // states in different slots may become equivallent!
        return true;
    }
};

/**
 * Another top-down DD builder.
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
            std::ostream& os = std::cout, bool cut = false)
            : output(output.privateEntity()), spec(s),
              specNodeSize(getSpecNodeSize(spec.datasize())), os(os), cut(cut) {
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
 * Top-down ZDD subset builder.
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

    MyVector<SpecNode> tmp;
    MemoryPools pools;

public:
    ZddSubsetter(NodeTableHandler<AR> const& input, Spec const& s,
            NodeTableHandler<AR>& output)
            : input(*input), output(output.privateEntity()), spec(s),
              specNodeSize(getSpecNodeSize(spec.datasize())),
              work(input->numRows()) {
    }

    /**
     * Initializes the builder.
     * @param root the root node.
     */
    int initialize(NodeId& root) {
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
                void* s[2] = {state(ptmp), state(p)};

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
                    }
                    else {
                        assert(ii == f.row() && ii == kk && ii < i);
                        if (work[ii].empty()) work[ii].resize(input[ii].size());
                        SpecNode* pp = work[ii][f.col()].alloc_front(pools[ii],
                                specNodeSize);
                        spec.get_copy(state(pp), s[b]);
                        srcPtr(pp) = &q->branch[b];
                    }
                }

                spec.destruct(state(p));
                spec.destruct(state(ptmp));
                ++q;
            }
        }

        assert(q == output[i].data() + mm);
        work[i].clear();
        pools[i].clear();
        spec.destructLevel(i);
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
 * Multi-threaded top-down ZDD subset builder.
 */
template<typename S>
class ZddSubsetterMP: DdBuilderMPBase {
    //typedef typename std::remove_const<typename std::remove_reference<S>::type>::type Spec;
    typedef S Spec;
    typedef MyHashTable<SpecNode*,Hasher<Spec>,Hasher<Spec> > UniqTable;
    static int const AR = Spec::ARITY;

    NodeTableEntity<AR> const& input;
    NodeTableEntity<AR>& output;
    Spec spec;
    int const specNodeSize;

    int const threads;
    MyVector<MyVector<MyVector<MyListOnPool<SpecNode> > > > snodeTables;
    MyVector<MemoryPools> pools;

public:
    ZddSubsetterMP(NodeTableHandler<AR> const& input, Spec const& s,
            NodeTableHandler<AR>& output)
            : input(*input), output(output.privateEntity()), spec(s),
              specNodeSize(getSpecNodeSize(spec.datasize())),
#ifdef _OPENMP
              threads(omp_get_max_threads()),
#else
              threads(1),
#endif
              snodeTables(threads),
              pools(threads) {
    }

    /**
     * Initializes the builder.
     * @param root the root node.
     */
    int initialize(NodeId& root) {
        MyVector<SpecNode> tmp(specNodeSize);
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

#ifdef _OPENMP
#pragma omp parallel
#endif
        {
#ifdef _OPENMP
            int yy = omp_get_thread_num();
#else
            int yy = 0;
#endif
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
                        void* s[2] = {state(ptmp), state(p)};

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
                                q.branch[b] = val;
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
                            }

                            spec.destruct(s[b]);
                        }
                    }
                }
            }

            snodeTables[yy][i].clear();
            pools[yy][i].clear();
        }

        spec.destructLevel(i);
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

//template<bool suppressDontCare = false>
//class ZddReducer {
//    NodeTableEntity& input;
//    NodeTableHandler oldDiagram;
//    NodeTableHandler newDiagram;
//    NodeTableEntity& output;
//    MyVector<MyVector<NodeId> > newIdTable;
//    MyVector<MyVector<NodeId*> > rootPtr;
//
//public:
//    ZddReducer(NodeTableHandler& diagram)
//            : input(diagram.privateEntity()), oldDiagram(diagram),
//              newDiagram(input.numRows()), output(newDiagram.privateEntity()),
//              newIdTable(input.numRows()), rootPtr(input.numRows()) {
//        diagram = newDiagram;
//#ifdef DEBUG
//        size_t dead = 0;
//#endif
//
//        /* apply zero-suppress and dc-suppress rules */
//        for (int i = 2; i < input.numRows(); ++i) {
//            size_t const m = input[i].size();
//            Node* const tt = input[i].data();
//
//            for (size_t j = 0; j < m; ++j) {
//                for (int b = 0; b < AR; ++b) {
//                    NodeId& f = tt[j].branch[b];
//                    if (f.row() == 0) continue;
//
//                    NodeId f0 = input.child(f, 0);
//                    NodeId f1 = input.child(f, 1);
//                    if (f1 == 0 || (suppressDontCare && f1 == f0)) {
//                        f = f0;
//#ifdef DEBUG
//                        if (f == 0) ++dead;
//#endif
//                    }
//                }
//            }
//        }
//
//#ifdef DEBUG
//        std::cerr << "[#dead = " << dead << "]";
//#endif
//
//        input[0].resize(2);
//        input.makeIndex();
//
//        newIdTable[0].resize(2);
//        newIdTable[0][0] = 0;
//        newIdTable[0][1] = 1;
//    }
//
//    /**
//     * Sets a root node.
//     * @param root reference to a root node ID storage.
//     */
//    void setRoot(NodeId& root) {
//        rootPtr[root.row()].push_back(&root);
//    }
//
//    /**
//     * Reduces one level.
//     * @param i level.
//     */
//    void reduce(int i) {
//        size_t const m = input[i].size();
//        Node* const tt = input[i].data();
//
//        MyVector<NodeId>& newId = newIdTable[i];
//        newId.resize(m);
//
//        for (size_t j = m - 1; j + 1 > 0; --j) {
//            NodeId const mark(i, m);
//            NodeId& f0 = tt[j].branch[0];
//            NodeId& f1 = tt[j].branch[1];
//
//            if (f0.row() != 0) f0 = newIdTable[f0.row()][f0.col()];
//            if (f1.row() != 0) f1 = newIdTable[f1.row()][f1.col()];
//
//            if (f1 == 0 || (suppressDontCare && f1 == f0)) { // f1 can be 0
//                newId[j] = f0;
//            }
//            else {
//                NodeId& f00 = input.child(f0, 0);
//                NodeId& f01 = input.child(f0, 1);
//
//                if (f01 != mark) {        // the first touch from this level
//                    f01 = mark;        // mark f0 as touched
//                    newId[j] = NodeId(i + 1, m); // tail of f0-equivalent list
//                }
//                else {
//                    newId[j] = f00;         // next of f0-equivalent list
//                }
//                f00 = NodeId(i + 1, j);  // new head of f0-equivalent list
//            }
//        }
//
//        {
//            MyVector<int> const& levels = input.lowerLevels(i);
//            for (int const* t = levels.begin(); t != levels.end(); ++t) {
//                newIdTable[*t].clear();
//            }
//        }
//        size_t mm = 0;
//
//        for (size_t j = 0; j < m; ++j) {
//            NodeId const f(i, j);
//            assert(newId[j].row() <= i + 1);
//            if (newId[j].row() <= i) continue;
//
//            for (size_t k = j; k < m;) { // for each g in f0-equivalent list
//                assert(j <= k);
//                NodeId const g(i, k);
//                NodeId& g0 = tt[k].branch[0];
//                NodeId& g1 = tt[k].branch[1];
//                NodeId& g10 = input.child(g1, 0);
//                NodeId& g11 = input.child(g1, 1);
//                assert(newId[k].row() == i + 1);
//                size_t next = newId[k].col();
//
//                if (g11 != f) { // the first touch to g1 in f0-equivalent list
//                    g11 = f; // mark g1 as touched
//                    g10 = g; // record g as a canonical node for <f0,g1>
//                    newId[k] = NodeId(i, mm++, g0.hasEmpty());
//                }
//                else {
//                    g0 = g10;       // make a forward link
//                    g1 = 0;       // mark g as forwarded
//                    newId[k] = 0;
//                }
//
//                k = next;
//            }
//        }
//
//        if (!suppressDontCare) {
//            MyVector<int> const& levels = input.lowerLevels(i);
//            for (int const* t = levels.begin(); t != levels.end(); ++t) {
//                input[*t].clear();
//            }
//        }
//
//        Node* nt = output.initRow(i, mm);
//
//        for (size_t j = 0; j < m; ++j) {
//            NodeId const& f0 = tt[j].branch[0];
//            NodeId const& f1 = tt[j].branch[1];
//
//            if (f1 == 0 || (suppressDontCare && f1 == f0)) { // forwarded
//                assert(newId[j].row() <= i);
//                if (f0.row() == i) {
//                    assert(newId[j] == 0);
//                    newId[j] = newId[f0.col()];
//                }
//            }
//            else {
//                assert(newId[j].row() == i);
//                size_t k = newId[j].col();
//                nt[k] = tt[j];
//            }
//        }
//
//        for (size_t k = 0; k < rootPtr[i].size(); ++k) {
//            NodeId& root = *rootPtr[i][k];
//            root = newId[root.col()];
//        }
//    }
//};
//
//template<bool suppressDontCare = false>
//class ZddReducerMP {
//    struct ReducNodeInfo {
//        Node children;
//        size_t column;
//
//        size_t hash() const {
//            return children.hash();
//        }
//
//        bool operator==(ReducNodeInfo const& o) const {
//            return children == o.children;
//        }
//
//        friend std::ostream& operator<<(std::ostream& os,
//                ReducNodeInfo const& o) {
//            return os << "(" << o.children << " -> " << o.column << ")";
//        }
//    };
//
//    static int const TASKS_PER_THREAD = 10;
//
//    NodeTableEntity& input;
//    NodeTableHandler oldDiagram;
//    NodeTableHandler newDiagram;
//    NodeTableEntity& output;
//    MyVector<MyVector<NodeId> > newIdTable;
//    MyVector<MyVector<NodeId*> > rootPtr;
//
//    int const threads;
//    int const tasks;
//    MyVector<MyVector<MyList<ReducNodeInfo> > > taskMatrix;
//    MyVector<size_t> baseColumn;
//
//    //        ElapsedTimeCounter etcP1, etcP2, etcP3, etcS1, etcS2;
//
//public:
//    ZddReducerMP(NodeTableHandler& diagram)
//            : input(diagram.privateEntity()), oldDiagram(diagram),
//              newDiagram(input.numRows()), output(newDiagram.privateEntity()),
//              newIdTable(input.numRows()), rootPtr(input.numRows()),
//#ifdef _OPENMP
//              threads(omp_get_max_threads()),
//              tasks(MyHashConstant::primeSize(TASKS_PER_THREAD * threads)),
//#else
//              threads(1),
//              tasks(1),
//#endif
//              taskMatrix(threads),
//              baseColumn(tasks + 1) {
//        diagram = newDiagram;
//
//        input[0].resize(2);
//        input.makeIndex();
//
//        newIdTable[0].resize(2);
//        newIdTable[0][0] = 0;
//        newIdTable[0][1] = 1;
//
//        for (int y = 0; y < threads; ++y) {
//            taskMatrix[y].resize(tasks);
//        }
//    }
//
//    /**
//     * Sets a root node.
//     * @param root reference to a root node ID storage.
//     */
//    void setRoot(NodeId& root) {
//        rootPtr[root.row()].push_back(&root);
//    }
//
//    /**
//     * Reduces one level.
//     * @param i level.
//     */
//    void reduce(int i) {
//        size_t const m = input[i].size();
//        newIdTable[i].resize(m);
////            etcP1.start();
//
//#ifdef _OPENMP
//#pragma omp parallel
//#endif
//        {
//#ifdef _OPENMP
//            int y = omp_get_thread_num();
//#else
//            int y = 0;
//#endif
//            MyHashTable<ReducNodeInfo const*> uniq;
//
//#ifdef _OPENMP
//#pragma omp for schedule(static)
//#endif
//            for (size_t j = 0; j < m; ++j) {
//                Node& f = input[i][j];
//
//                // make f canonical
//                NodeId& f0 = f.branch[0];
//                NodeId& f1 = f.branch[1];
//                f0 = newIdTable[f0.row()][f0.col()];
//                f1 = newIdTable[f1.row()][f1.col()];
//                if (suppressDontCare && f1 == f0) f1 = 0;
//                if (f1 == 0) { // f is redundant
//                    newIdTable[i][j] = f0;
//                    continue;
//                }
//
//                // schedule a task
//                int x = f.hash() % tasks;
//                ReducNodeInfo* p = taskMatrix[y][x].alloc_front();
//                p->children = f;
//                p->column = j;
//            }
//
//#ifdef _OPENMP
//#pragma omp single
//#endif
//            {
////                    etcP1.stop();
////                    etcS1.start();
//
//                MyVector<int> const& levels = input.lowerLevels(i);
//                for (int const* t = levels.begin(); t != levels.end(); ++t) {
//                    newIdTable[*t].clear();
//                }
//
////                    etcS1.stop();
////                    etcP2.start();
//            }
//
//#ifdef _OPENMP
//#pragma omp for schedule(dynamic)
//#endif
//            for (int x = 0; x < tasks; ++x) {
//                size_t mm = 0;
//                for (int yy = 0; yy < threads; ++yy) {
//                    mm += taskMatrix[yy][x].size();
//                }
//                if (mm == 0) {
//                    baseColumn[x + 1] = 0;
//                    continue;
//                }
//
//                uniq.initialize(mm * 2);
//                size_t j = 0;
//
//                for (int yy = 0; yy < threads; ++yy) {
//                    MyList<ReducNodeInfo>& taskq = taskMatrix[yy][x];
//
//                    for (typename MyList<ReducNodeInfo>::iterator t =
//                            taskq.begin(); t != taskq.end(); ++t) {
//                        ReducNodeInfo const* p = *t;
//                        ReducNodeInfo const* pp = uniq.add(p);
//
//                        if (pp == p) {
//                            newIdTable[i][p->column] = NodeId(x, j++,
//                                    p->children.branch[0].hasEmpty());
//                        }
//                        else {
//                            newIdTable[i][p->column] =
//                                    newIdTable[i][pp->column];
//                        }
//                    }
//                }
//
//                baseColumn[x + 1] = j;
//
//                for (int yy = 0; yy < threads; ++yy) {
//                    taskMatrix[yy][x].clear();
//                }
//            }
//
//#ifdef _OPENMP
//#pragma omp single
//#endif
//            {
////                    etcP2.stop();
////                    etcS2.start();
//
//                for (int x = 1; x < tasks; ++x) {
//                    baseColumn[x + 1] += baseColumn[x];
//                }
//
////                for (int k = 1; k < tasks; k <<= 1) {
////#ifdef _OPENMP
////#pragma omp for schedule(static)
////#endif
////                    for (int x = k; x < tasks; ++x) {
////                        if (x & k) {
////                            int w = (x & ~k) | (k - 1);
////                            baseColumn[x + 1] += baseColumn[w + 1];
////                        }
////                    }
////                }
//
//                output.initRow(i, baseColumn[tasks]);
////                    etcS2.stop();
////                    etcP3.start();
//            }
//
//#ifdef _OPENMP
//#pragma omp for schedule(static)
//#endif
//            for (size_t j = 0; j < m; ++j) {
//                Node const& f = input[i][j];
//                if (f.branch[1] == 0) continue;
//                NodeId& g = newIdTable[i][j];
//                g = NodeId(i, g.col() + baseColumn[g.row()]);
//                output[i][g.col()] = f;
//            }
//        }
////            etcP3.stop();
//
//        input[i].clear();
//
//        for (size_t k = 0; k < rootPtr[i].size(); ++k) {
//            NodeId& root = *rootPtr[i][k];
//            root = newIdTable[i][root.col()];
//        }
//    }
//};

}// namespace tdzdd
