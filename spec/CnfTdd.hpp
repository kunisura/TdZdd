/*
 * Top-Down ZDD Construction Library for Frontier-Based Search
 * by Hiroaki Iwashita <iwashita@erato.ist.hokudai.ac.jp>
 * Copyright (c) 2014 Japan Science and Technology Agency
 * $Id: CnfTdd.hpp 518 2014-03-18 05:29:23Z iwashita $
 */

#pragma once

#include <cassert>
#include <iostream>

#include "../dd/DdBuilder.hpp"
#include "../dd/DdReducer.hpp"
#include "../dd/DdSpec.hpp"
#include "../dd/NodeTable.hpp"
#include "../spec/CuddBdd.hpp"
#include "../util/MessageHandler.hpp"
#include "../util/MyVector.hpp"

namespace tdzdd {

class CnfTdd: public ScalarDdSpec<CnfTdd,uint32_t,3> {
public:
    typedef MyVector<int> Clause;
    typedef uint32_t NodeNumber;

    struct TddNode {
        union {
            struct {
                uint32_t level;
                NodeNumber branch[3];
            };
            uint64_t code[2];
        };

        size_t hash() const {
            return code[0] * 314159257 + code[1] * 271828171;
        }

        bool operator==(TddNode const& o) const {
            return code[1] == o.code[1] && code[0] == o.code[0];
        }

        friend std::ostream& operator<<(std::ostream& os, TddNode const& o) {
            return os << "(" << o.level << ":" << o.branch[0] << ","
                    << o.branch[1] << "," << o.branch[2] << ")";
        }
    };

private:
    int n; // the number of variables
    int nc; // the number of clauses
    NodeId rootId;
    NodeTableHandler<3> nodeTable;
    NodeNumber rootNumber;
    MyVector<NodeNumber> startNumber;
    MyVector<TddNode> nodeArray;
    MyVector<CuddBdd> guide;

    int levelOfVar(int var) const {
        assert(1 <= var && var <= n);
        return n - var + 1;
    }

    int literalAtLevel(int level) const {
        assert(level != 0 && std::abs(level) <= n);
        return (level > 0) ? n - level + 1 : -(n + level + 1);
    }

public:
    /**
     * Gets a node.
     * @param f node number.
     * @return node reference.
     */
    TddNode const& node(NodeNumber f) const {
        return nodeArray[f];
    }

    /**
     * Gets the root node.
     * @return the root node.
     */
    NodeNumber const& root() const {
        return rootNumber;
    }

    /**
     * Gets a child node.
     * @param f parent node ID.
     * @param b branch number.
     * @return child node ID.
     */
    NodeNumber const& child(NodeNumber f, int b) const {
        assert(0 <= b && b < 3);
        return nodeArray[f].branch[b];
    }

    /**
     * Gets the number of nonterminal nodes.
     * @return the number of nonterminal nodes.
     */
    size_t size() const {
        return nodeArray.size();
    }

    /**
     * Gets the level of root ZDD variable.
     * @return the level of root ZDD variable.
     */
    int topLevel() const {
        return nodeArray[rootNumber].level;
    }

    /**
     * Gets the number of variables.
     * @return the number of clauses.
     */
    int numVars() const {
        return n;
    }

    /**
     * Gets the number of clauses.
     * @return the number of clauses.
     */
    int numClauses() const {
        return nc;
    }

private:
    struct CompareVars {
        bool& tautology;

        CompareVars(bool& tautology)
                : tautology(tautology) {
        }

        bool operator()(int a, int b) {
            if (a == -b) tautology = true;
            return std::abs(a) > std::abs(b);
        }
    };

    struct CompareClause {
        bool operator()(Clause const* a, Clause const* b) const {
            int i = a->size() - 1;
            int j = b->size() - 1;

            while (i >= 0 && j >= 0) {
                if ((*a)[i] != (*b)[j]) {
                    if ((*a)[i] == -(*b)[j]) return (*a)[i] < (*b)[j];
                    return std::abs((*a)[i]) > std::abs((*b)[j]);
                }
                --i, --j;
            }

            return j > 0;
        }
    };

    struct EquivClause {
        bool operator()(Clause const* a, Clause const* b) const {
            return *a == *b;
        }
    };

    /**
     * Reads DIMACS CNF.
     * @param is input stream to read.
     * @param cl output for clauses.
     */
    void readDimacs(std::istream& is, MyVector<Clause>& cl) {
        std::string line;
        n = nc = 0;

        while (is) {
            std::getline(is, line);
            if (sscanf(line.c_str(), "p cnf %d %d", &n, &nc) == 2) break;
        }
        if (n < 1 || nc < 1) throw std::runtime_error(
                "CNF header line not found");

        cl.reserve(cl.size() + nc);
        Clause c;
        int mm = 0;

        while (is) {
            int x;
            is >> x;

            if (x == 0) {
                if (!c.empty()) {
                    bool tautology = false;
                    std::sort(c.begin(), c.end(), CompareVars(tautology));
                    if (!tautology) {
                        int k = std::unique(c.begin(), c.end()) - c.begin();
                        c.resize(k);
                        cl.push_back(c);
                    }
                    c.resize(0);
                    ++mm;
                }
            }
            else {
                int v = std::abs(x);
                if (v > n) throw std::runtime_error(
                        "Variable ID is out of range");
                if (mm >= nc) throw std::runtime_error("Too many clauses");

                c.push_back(x);
            }
        }

        if (mm < nc) throw std::runtime_error("Too few clauses");
    }

    void buildTable(MyVector<Clause*>& cp) {
        // inStack ... cmd(-var), end, fst+, fst-, fst0
        MyVector<int> inStack;
        MyVector<NodeId> outStack;
        NodeTableEntity<3>& diagram = nodeTable.privateEntity();
        if (n >= diagram.numRows()) diagram.setNumRows(n + 1);
        UniqueTable<3> uniq(diagram);
//        for (size_t i = 0; i < cp.size(); ++i) {
//            std::cerr << "\n" << *cp[i];
//        }

        inStack.push_back(cp.size());
        inStack.push_back(0);

        while (inStack.size() >= 2) {
            int fst = inStack.back();
            inStack.pop_back();
            int lst = inStack.back();
            int i = lst - 1;

            if (lst < 0) {
                inStack.pop_back();
                int level = levelOfVar(-lst);
                Node<3> node;
                assert(outStack.size() >= 3);

                for (int k = 2; k >= 1; --k) {
                    node.branch[k] = outStack.back();
                    outStack.pop_back();
                }

                node.branch[0] = outStack.back();

                if (node.branch[0] != 1) {
                    bool zsupp = true;

                    for (int k = 2; k >= 1; --k) {
                        if (node.branch[k] == node.branch[0]) {
                            node.branch[k] = 0;
                        }
                        else if (zsupp && node.branch[k] != 0) {
                            zsupp = false;
                        }
                    }

                    if (!zsupp) {
                        outStack.back() = uniq.getNode(level, node);
                    }
                }
            }
            else if (i < fst) {
                outStack.push_back(0);
            }
            else if (cp[i]->empty()) {
                assert(i == fst);
                outStack.push_back(1);
            }
            else {
                int var = std::abs(cp[i]->back());
                inStack.push_back(-var);
                inStack.push_back(lst);

                while (fst <= i && !cp[i]->empty() && cp[i]->back() == var) {
                    cp[i]->pop_back();
                    --i;
                }
                inStack.push_back(i + 1);

                while (fst <= i && !cp[i]->empty() && cp[i]->back() == -var) {
                    cp[i]->pop_back();
                    --i;
                }
                inStack.push_back(i + 1);
                inStack.push_back(fst);
            }
        }

        assert(outStack.size() == 1);
        rootId = outStack.back();
    }

    void reduceTable() {
        int n = rootId.row();
        DdReducer<3,false,true> zr(nodeTable);
        zr.setRoot(rootId);
        zr.garbageCollect();
        for (int i = 1; i <= n; ++i) {
            zr.reduce(i);
        }
    }

    void buildList() {
        NodeTableEntity<3> const& diagram = *nodeTable;
        nodeArray.clear();
        nodeArray.reserve(diagram.totalSize());
        startNumber.clear();
        startNumber.resize(diagram.numRows() + 1);

        startNumber[0] = 0;
        for (int i = 0; i < diagram.numRows(); ++i) {
            startNumber[i + 1] = startNumber[i] + diagram[i].size();
        }

        for (int i = 0; i < diagram.numRows(); ++i) {
            size_t m = diagram[i].size();
            for (size_t j = 0; j < m; ++j) {
                TddNode node;
                node.level = i;
                for (int b = 0; b < 3; ++b) {
                    NodeId f = diagram[i][j].branch[b];
                    node.branch[b] = startNumber[f.row()] + f.col();
                }
                nodeArray.push_back(node);
            }
        }

        rootNumber = startNumber[rootId.row()] + rootId.col();
    }

    void buildGuide(size_t limit) {
        MyVector<CuddBdd> y(rootNumber + 1);
        {
            y[0] = 1;
            y[1] = 0;
            for (NodeNumber j = 2; j <= rootNumber; ++j) {
                y[j] = CuddBdd(j, 0, 1);
            }
            for (NodeNumber j = rootNumber; j >= 2; j = child(j, 0)) {
                y[j] = 1;
            }
        }

        MyVector<CuddBdd> firstTouch(n + 1);
        {
            for (int i = 0; i <= n; ++i) {
                firstTouch[i] = 1;
            }
            MyVector<bool> mark(rootNumber + 1);
            for (NodeNumber j = rootNumber; j >= 2; --j) {
                for (int b = 0; b < 3; ++b) {
                    NodeNumber k = child(j, b);
                    if (mark[k]) continue;
                    mark[k] = true;
                    if (y[k].level() == 0) continue;
                    firstTouch[node(j).level] &= y[k];
                }
            }
        }

        MyVector<int> numClauseVars(n + 1);
        {
            CuddBdd support = 1;
            NodeNumber j = 2;
            for (int i = 1; i <= n; ++i) {
                for (; j <= rootNumber && int(node(j).level) == i; ++j) {
                    support &= y[j];
                }
                support = support.abstract(firstTouch[i]);
                numClauseVars[i] = support.size() - 1;
            }
        }

        guide.clear();
        guide.resize(n + 1);
        {
            guide[0] = 1;
            guide[n] = 1;
            CuddBdd g = 1;
            NodeNumber j = rootNumber;
            for (int i = n; i >= 2; --i) {
                MessageHandler mh;
                mh.begin("down") << " " << i << " ...";
                CuddBdd vars = 1;
                CuddBdd g0 = g;
                CuddBdd g1 = 1;
                CuddBdd g2 = 1;
                assert(int(node(j).level) <= i);

                for (; int(node(j).level) == i; --j) {
                    vars &= y[j];
                    g0 &= ~y[j] | y[child(j, 0)];
                    g1 &= ~y[j] | y[child(j, 1)];
                    g2 &= ~y[j] | y[child(j, 2)];
                }

                guide[i - 1] = g = g0.andAbstract(g1 | g2, vars);

                if (limit > 0 && g.size() > limit) {
                    mh << " " << g.countMinterm(numClauseVars[i - 1]);
                    mh << " <" << g.size() << "> ...";
                    while (g.size() > limit) {
                        // g = g.child(0) | g.child(1);
                        CuddBdd v = g.support();
                        if (v.isConstant()) break;
                        while (!v.child(1).isConstant()) {
                            v = v.child(1);
                        }
                        g = g.abstract(v);
                    }
                }
                mh << " " << g.countMinterm(numClauseVars[i - 1]);
                mh.end(g.size());
//                std::stringstream ss;
//                ss << "Level " << i << ": #node = " << g.size();
//                g.dumpDot(std::cout, ss.str());
            }
        }

#ifdef OLD_UP
        {
            NodeNumber j = 2;
            for (int i = 1; i < n; ++i) {
                MessageHandler mh;
                mh.begin("up") << " " << i << " ...";
                CuddBdd vars = 1;
                CuddBdd g0 = guide[i - 1];
                CuddBdd g1 = 1;
                CuddBdd g2 = 1;
                assert(int(node(j).level) >= i);

                for (; int(node(j).level) == i; ++j) {
                    if (!y[child(j, 0)].isConstant()) vars &= y[child(j, 0)];
                    if (!y[child(j, 1)].isConstant()) vars &= y[child(j, 1)];
                    if (!y[child(j, 2)].isConstant()) vars &= y[child(j, 2)];
                    g0 &= ~y[j] | y[child(j, 0)];
                    g1 &= ~y[j] | y[child(j, 1)];
                    g2 &= ~y[j] | y[child(j, 2)];
                }

                guide[i] &= g0.andAbstract(g1 | g2, vars);
                mh << " " << guide[i].countMinterm(numClauseVars[i]);
                mh.end(guide[i].size());
//                std::stringstream ss;
//                ss << "Level " << i << ": #node = " << guide[i].size();
//                guide[i].dumpDot(std::cout, ss.str());
            }
        }
#else
        {
            CuddBdd g = 1;
            NodeNumber j = 2;
            for (int i = 1; i < n; ++i) {
                MessageHandler mh;
                mh.begin("up") << " " << i << " ...";
                CuddBdd vars = 1;
                CuddBdd g1 = g;
                CuddBdd g2 = g;
                MyVector<CuddBdd> tr1(rootNumber + 1);
                MyVector<CuddBdd> tr2(rootNumber + 1);
                assert(int(node(j).level) >= i);

                for (NodeNumber k = j; int(node(k).level) == i; ++k) {
                    for (int b = 0; b < 3; ++b) {
                        NodeNumber kk = child(k, b);
                        if (!y[kk].isConstant() && tr1[kk].isNull()) {
                            tr1[kk] = tr2[kk] =
                                    firstTouch[i].dependsOn(y[kk]) ? 0 : y[kk];
                        }
                    }
                }

                for (; int(node(j).level) == i; ++j) {
                    NodeNumber j0 = child(j, 0);
                    if (!y[j0].isConstant()) {
                        tr1[j0] |= y[j];
                        tr2[j0] |= y[j];
                    }
                    else if (j0 == 1) {
                        g1 &= ~y[j];
                        g2 &= ~y[j];
                    }

                    NodeNumber j1 = child(j, 1);
                    if (!y[j1].isConstant()) {
                        tr1[j1] |= y[j];
                    }
                    else if (j1 == 1) {
                        g1 &= ~y[j];
                    }

                    NodeNumber j2 = child(j, 2);
                    if (!y[j2].isConstant()) {
                        tr2[j2] |= y[j];
                    }
                    else if (j2 == 1) {
                        g2 &= ~y[j];
                    }
                }

                g1 = g1.compose(tr1);
                g2 = g2.compose(tr2);
                guide[i] &= g1 | g2;
                //guide[i] = (g1 | g2).minimize(guide[i]);
                g = guide[i];

                if (limit > 0 && g.size() > limit) {
                    mh << " " << g.countMinterm(numClauseVars[i - 1]);
                    mh << " <" << g.size() << "> ...";
                    while (g.size() > limit) {
//                        g = g.child(0) | g.child(1);
                        CuddBdd v = g.support();
                        if (v.isConstant()) break;
                        while (!v.child(1).isConstant()) {
                            v = v.child(1);
                        }
                        g = g.abstract(v);
                    }
                }
                mh << " " << g.countMinterm(numClauseVars[i]);
                mh.end(g.size());
//                std::stringstream ss;
//                ss << "Level " << i << ": #node = " << guide[i].size();
//                guide[i].dumpDot(std::cout, ss.str());
            }
        }
#endif
    }

    int countClauses() const {
        MyVector<int> count(rootNumber + 1);
        count[0] = 0;
        count[1] = 1;

        for (NodeNumber j = 2; j <= rootNumber; ++j) {
            for (int b = 0; b < 3; ++b) {
                NodeNumber k = child(j, b);
                assert(0 <= k && k < j);
                count[j] += count[k];
            }
        }

        return count[rootNumber];
    }

public:
    /**
     * Reads DIMACS CNF and constructs a TDD.
     * @param is input stream to read.
     */
    void load(std::istream& is) {
        MessageHandler mh;
        mh.begin("loading");

        MyVector<Clause> cl;
        readDimacs(is, cl);
        mh << " #var = " << n << ", #clause = " << nc;

        MyVector<Clause*> cp(cl.size());
        for (size_t i = 0; i < cl.size(); ++i) {
            cp[i] = &cl[i];
        }

        // sort clauses
        int tmp = nc;
        std::sort(cp.begin(), cp.end(), CompareClause());
        nc = std::unique(cp.begin(), cp.end(), EquivClause()) - cp.begin();
        cp.resize(nc);
        if (nc != tmp) mh << " -> " << nc;

        buildTable(cp);
        reduceTable();
        buildList();

        tmp = nc;
        nc = countClauses();
        if (nc != tmp) mh << " -> " << nc;
        mh << " ...";

        mh.end(nodeArray.size());
    }

    /**
     * Makes rich reachability information.
     * @param limit maximum BDD size for state sets.
     */
    void compile(size_t limit) {
        MessageHandler mh;
        mh.begin("compiling") << " ...";

        buildGuide(limit);

        mh.end();
    }

    /**
     * Checks unsatisfiability of the conjunction of clauses.
     * @param level variable level.
     * @param clauses collection of clause numbers.
     * @return true if unsatisfiable, false if satisfiable or unknown.
     */
    template<typename C>
    bool conflictsWith(int level, C const& clauses) const {
        CuddBdd g = guide[level];
        typeof(clauses.rbegin()) t = clauses.rbegin();

        while (t != clauses.rend()) {
            if (g.level() == int(*t)) { // 1
                g = g.child(1);
                if (g.isConstant()) return g == 0;
                ++t;
            }
            else {
                while (g.level() > int(*t)) { // 0
                    g = g.child(0);
                    if (g.isConstant()) return g == 0;
                }
                while (int(*t) > g.level()) { // don't care
                    ++t;
                }
            }
        }

        while (!g.isConstant()) {
            g = g.child(0);
        }
        return g == 0;
    }

    /**
     * Checks unsatisfiability of the conjunction of clauses.
     * @param level variable level.
     * @param f BDD for clause set.
     * @return true if unsatisfiable, false if satisfiable or unknown.
     */
    bool conflictsWithBdd(int level, CuddBdd const& f) const {
        return !guide[level].contains(f);
    }

    int getRoot(NodeNumber& f) const {
        f = rootNumber;
        return (f == 1) ? -1 : node(f).level;
    }

    int getChild(NodeNumber& f, int level, int value) const {
        assert(level > 0 && level == int(node(f).level));
        assert(0 <= value && value < 3);
        f = child(f, value);
        return (f == 1) ? -1 : node(f).level;
    }

    size_t hashCode(NodeNumber const& f) const {
        return f * 314159257;
    }

    void printLevel(std::ostream& os, int level) const {
        os << literalAtLevel(level);
    }

    class const_iterator {
        struct Selection {
            NodeNumber node;
            int val;

            Selection()
                    : node(0), val(0) {
            }

            Selection(NodeNumber node, int val)
                    : node(node), val(val) {
            }

            bool operator==(Selection const& o) const {
                return node == o.node && val == o.val;
            }
        };

        CnfTdd const& tdd;
        int cursor;
        std::vector<Selection> path;
        std::vector<int> clause;

    public:
        const_iterator(CnfTdd const& tdd, bool begin)
                : tdd(tdd), cursor(begin ? -1 : -2) {
            if (begin) next(tdd.rootNumber);
        }

        const_iterator& operator++() {
            next(0);
            return *this;
        }

        std::vector<int> const& operator*() const {
            return clause;
        }

        std::vector<int> const* operator->() const {
            return &clause;
        }

        bool operator==(const_iterator const& o) const {
            return cursor == o.cursor && path == o.path;
        }

        bool operator!=(const_iterator const& o) const {
            return !operator==(o);
        }

    private:
        void next(NodeNumber f) {
            for (;;) {
                while (f != 0) { /* down */
                    if (f == 1) return;
                    int level = tdd.node(f).level;

                    if (tdd.child(f, 0) != 0) {
                        cursor = path.size();
                        path.push_back(Selection(f, 0));
                        f = tdd.child(f, 0);
                    }
                    else if (tdd.child(f, 1) != 0) {
                        cursor = path.size();
                        clause.push_back(-level);
                        path.push_back(Selection(f, 1));
                        f = tdd.child(f, 1);
                    }
                    else if (tdd.child(f, 2) != 0) {
                        clause.push_back(level);
                        path.push_back(Selection(f, 2));
                        f = tdd.child(f, 2);
                    }
                    else {
                        f = 0;
                    }
                }

                for (; cursor >= 0; --cursor) { /* up */
                    f = path[cursor].node;
                    int& v = path[cursor].val;

                    while (++v <= 2 && tdd.child(f, v) == 0)
                        ;

                    if (v <= 2) {
                        int level = tdd.node(f).level;
                        path.resize(cursor + 1);
                        while (!clause.empty()
                                && std::abs(clause.back()) <= level) {
                            clause.pop_back();
                        }
                        clause.push_back(v == 1 ? -level : level);
                        f = tdd.child(f, v);
                        break;
                    }
                }

                if (cursor < 0) { /* end() state */
                    cursor = -2;
                    path.clear();
                    clause.clear();
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
     * Dumps the CNF in DIMACS format.
     * @param os output stream.
     */
    void dumpDimacs(std::ostream& os) const {
        os << "p cnf " << n << " " << nc << "\n";
        for (const_iterator c = begin(); c != end(); ++c) {
            bool spc = false;
            for (typeof(c->begin()) l = c->begin(); l != c->end(); ++l) {
                if (spc) os << " ";
                os << literalAtLevel(*l);
                spc = true;
            }
            os << " 0\n";
        }
    }
}
;

} // namespace tdzdd
