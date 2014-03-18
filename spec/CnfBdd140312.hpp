/*
 * Top-Down ZDD Construction Library for Frontier-Based Search
 * by Hiroaki Iwashita <iwashita@erato.ist.hokudai.ac.jp>
 * Copyright (c) 2014 Japan Science and Technology Agency
 * $Id: CnfBdd140312.hpp 518 2014-03-18 05:29:23Z iwashita $
 */

#pragma once

#include <cassert>
#include <iostream>
#include <utility>

#include "../dd/DataTable.hpp"
#include "../dd/DdSpec.hpp"
#include "../op/DdToDd.hpp"
#include "../spec/CuddBdd.hpp"
#include "../util/MessageHandler.hpp"
#include "../util/MySet.hpp"
#include "../util/MyVector.hpp"

namespace tdzdd {

struct CnfBdd140312State {
    int packedId;
    int actualId;
};

class CnfBdd140312: public ScalarDdSpec<CnfBdd140312,CnfBdd140312State> {
    typedef int ClauseNumber;
    typedef MyVector<ClauseNumber> ClauseList;
    typedef MyVector<int> Clause;

    int n; // the number of variables
    int m; // the number of clauses
    MyVector<Clause> cnf; // sorted list of clauses

    MyVector<ClauseList> posiClauses; // list of positive clauses for each level
    MyVector<ClauseList> negaClauses; // list of negative clauses for each level
    MyVector<ClauseList> enterClauses; // list of entering clauses for each level
    MyVector<ClauseList> leaveClauses; // list of leaving clauses for each level
    MyVector<ClauseList> frontierClauses; // list of active clauses for each level

    MyVector<CuddBdd> clauseVar; // BDD variable for each clause
    MyVector<CuddBdd> posiCube; // cube of positive clauses for each level
    MyVector<CuddBdd> negaCube; // cube of negative clauses for each level
    MyVector<CuddBdd> posiMask; // mask of positive clauses for each level
    MyVector<CuddBdd> negaMask; // mask of negative clauses for each level
    MyVector<CuddBdd> enterCube; // cube of entering clauses for each level
    MyVector<CuddBdd> leaveCube; // cube of leaving clauses for each level
    MyVector<CuddBdd> enterConstraint; // entering constraint for each level
    MyVector<CuddBdd> leaveConstraint; // leaving constraint for each level
    MyVector<CuddBdd> frontierCube; // cube of active clauses for each level
    int completingLevel; // the highest level where all clauses are touched

    MyVector<ClauseList> clauseMap; // mapping to canonical clause for each level
    MyVector<MyVector<CuddBdd> > clauseMapCube; // cube of source clauses for each level
    MyVector<MyVector<CuddBdd> > clauseMapFrom; // union of source clauses for each level
    MyVector<MyVector<CuddBdd> > clauseMapTo; // mapped set of clauses for each level

    MyVector<CuddBdd> frontierSet; // reachable state set for each level
    MyVector<CuddBdd> packedFrontierSet; // mapped reachable state set for each level

    MemoryPool pool;
    MyBitSetOnPool* packedClause;

public:
    /**
     * Gets the variable number at given level.
     * @param level variable level.
     * @return valiable number.
     */
    int varAtLevel(int level) const {
        assert(1 <= level && level <= n);
        return n - level + 1;
    }

    /**
     * Gets the variable level of a given variable.
     * @param var variable number.
     * @return valiable level.
     */
    int levelOfVar(int var) const {
        assert(1 <= var && var <= n);
        return n - var + 1;
    }

    /**
     * Gets the variable level of a given literal.
     * @param lit literal code.
     * @return valiable level.
     */
    int levelOfLiteral(int lit) const {
        assert(lit != 0 && std::abs(lit) <= n);
        return n - std::abs(lit) + 1;
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
        return m;
    }

    /**
     * Reads DIMACS CNF and constructs a TDD.
     * @param is input stream to read.
     * @param sort enables clause sorting.
     * @param limit the maximum BDD size in top-down traversal.
     */
    void load(std::istream& is, bool sort = false, size_t limit = 0) {
        readDimacs(is);
        if (sort) sortClauses();
        prepare();
        traverse(limit);
    }

private:
    struct VarCmp {
        bool& tautology;

        VarCmp(bool& tautology)
                : tautology(tautology) {
        }

        bool operator()(int a, int b) {
            if (a == -b) tautology = true;
            return std::abs(a) < std::abs(b);
        }
    };

    struct ClauseCmpForOrdering {
        bool operator()(Clause const& a, Clause const& b) const {
            for (size_t i = 0; i < a.size() && i < b.size(); ++i) {
                int const v = std::abs(a[i]);
                int const w = std::abs(b[i]);
                if (v != w) return v < w;
            }
            return a.size() < b.size();
//            for (int i = a.size() - 1, j = b.size() - 1; i >= 0 && j >= 0; --i, --j) {
//                int const v = std::abs(a[i]);
//                int const w = std::abs(b[j]);
//                if (v != w) return v < w;
//            }
//            return a.size() < b.size();
        }

        bool operator()(Clause const* a, Clause const* b) const {
            return operator()(*a, *b);
        }
    };

    struct ClauseCmpForRelaxing {
        bool operator()(Clause const& a, Clause const& b) const {
//            for (int i = a.size(), j = b.size(); i >= 0 && j >= 0; --i, --j) {
//                int const v = std::abs(a[i]);
//                int const w = std::abs(b[j]);
//                if (v != w) return v < w;
//                if (a[i] != b[j]) return a[i] < b[j];
//            }
            for (size_t i = 0; i < a.size() && i < b.size(); ++i) {
                int const v = std::abs(a[i]);
                int const w = std::abs(b[i]);
                if (v != w) return v > w;
                if (a[i] != b[i]) return a[i] > b[i];
            }
            return a.size() > b.size();
        }

        bool operator()(Clause const* a, Clause const* b) const {
            return operator()(*a, *b);
        }
    };

    struct ClauseEq {
        bool operator()(Clause const& a, Clause const& b) const {
            return a == b;
        }

        bool operator()(Clause const* a, Clause const* b) const {
            return *a == *b;
        }
    };

    /**
     * Reads DIMACS CNF.
     * @param is input stream to read.
     */
    void readDimacs(std::istream& is) {
        MessageHandler mh;
        mh.begin("reading CNF") << " ...";

        std::string line;
        n = m = 0;

        while (is) {
            std::getline(is, line);
            if (sscanf(line.c_str(), "p cnf %d %d", &n, &m) == 2) break;
        }
        if (n < 1 || m < 1) throw std::runtime_error(
                "CNF header line not found");

        cnf.reserve(cnf.size() + m);
        Clause c;
        int mm = 0;

        while (is) {
            int x;
            is >> x;

            if (x == 0) {
                if (!c.empty()) {
                    bool tautology = false;
                    std::sort(c.begin(), c.end(), VarCmp(tautology));
                    if (!tautology) {
                        int k = std::unique(c.begin(), c.end()) - c.begin();
                        c.resize(k);
                        cnf.push_back(c);
                    }
                    c.resize(0);
                    ++mm;
                }
            }
            else {
                if (std::abs(x) > n) throw std::runtime_error(
                        "Variable ID is out of range");
                if (mm >= m) throw std::runtime_error("Too many clauses");

                c.push_back(x);
            }
        }

        if (mm < m) throw std::runtime_error("Too few clauses");
        mm = cnf.size();
        if (mm != m) {
            mh << "\n" << (m - mm) << " redundant clause"
                    << (m - mm == 1 ? "" : "s") << " found.";
            m = mm;
        }

        mh.end();
        mh << "#var = " << n << ", #clause = " << m << "\n";
    }

    /**
     * Sorts the clauses.
     */
    void sortClauses() {
        MessageHandler mh;
        mh.begin("sorting clauses") << " ...";
//        MyVector<Clause> cv(m);
//        MyVector<Clause*> cp(m);
//        for (int j = 0; j < m; ++j) {
//            cv[j] = cnf[j];
//            cp[j] = &cv[j];
//        }
//
//        std::sort(cp.begin(), cp.end(), ClauseCmpForOrdering());
//        m = std::unique(cp.begin(), cp.end(), ClauseEq()) - cp.begin();
//
//        cnf.resize(m);
//        for (int j = 0; j < m; ++j) {
//            cnf[j].swap(*cp[j]);
//        }
        std::sort(cnf.begin(), cnf.end(), ClauseCmpForOrdering());
        m = std::unique(cnf.begin(), cnf.end(), ClauseEq()) - cnf.begin();
        cnf.resize(m);
        mh.end();
    }

    /**
     * Translates CNF information into BDDs.
     */
    void prepare() {
        clauseVar.resize(m + 1);
        for (int j = 1; j <= m; ++j) {
            clauseVar[j] = CuddBdd(j, 0, 1);
        }

        posiClauses.resize(n + 1);
        negaClauses.resize(n + 1);
        enterClauses.resize(n + 1);
        leaveClauses.resize(n + 1);
        frontierClauses.resize(n + 1);
        posiCube.resize(n + 1);
        negaCube.resize(n + 1);
        posiMask.resize(n + 1);
        negaMask.resize(n + 1);
        enterCube.resize(n + 1);
        leaveCube.resize(n + 1);
        enterConstraint.resize(n + 1);
        leaveConstraint.resize(n + 1);
        frontierCube.resize(n + 1);
        completingLevel = n;
        frontierSet.resize(n + 1);
        packedFrontierSet.resize(n + 1);
        for (int i = 0; i <= n; ++i) {
            posiCube[i] = 1;
            negaCube[i] = 1;
            posiMask[i] = 1;
            negaMask[i] = 1;
            enterCube[i] = 1;
            leaveCube[i] = 1;
            enterConstraint[i] = 1;
            leaveConstraint[i] = 1;
            frontierCube[i] = 1;
            frontierSet[i] = 1;
            packedFrontierSet[i] = 1;
        }

        for (int j = 1; j <= m; ++j) {
            Clause const& c = cnf[j - 1];
            int const enterLevel = n - std::abs(c[0]) + 1;
            int const leaveLevel = n - std::abs(c.back()) + 1;
            assert(
                    1 <= leaveLevel && leaveLevel <= enterLevel && enterLevel <= n);

            for (typeof(c.begin()) t = c.begin(); t != c.end(); ++t) {
                if (*t > 0) {
                    posiClauses[n - *t + 1].push_back(j);
                    posiCube[n - *t + 1] &= clauseVar[j];
                    posiMask[n - *t + 1] &= ~clauseVar[j];
                }
                else {
                    negaClauses[n + *t + 1].push_back(j);
                    negaCube[n + *t + 1] &= clauseVar[j];
                    negaMask[n + *t + 1] &= ~clauseVar[j];
                }
            }

            enterClauses[enterLevel].push_back(j);
            enterCube[enterLevel] &= clauseVar[j];
            enterConstraint[enterLevel] &= clauseVar[j];

            leaveClauses[leaveLevel].push_back(j);
            leaveCube[leaveLevel] &= clauseVar[j];
            leaveConstraint[leaveLevel] &= ~clauseVar[j];

            for (int i = leaveLevel - 1; i < enterLevel; ++i) {
                frontierClauses[i].push_back(j);
                frontierCube[i] &= clauseVar[j];
            }

            completingLevel = std::min(completingLevel, enterLevel - 1);
        }

        clauseMap.resize(n + 1);
        clauseMapCube.resize(n + 1);
        clauseMapFrom.resize(n + 1);
        clauseMapTo.resize(n + 1);
        for (int i = 0; i <= n; ++i) {
            clauseMap[i].resize(m + 1);
            clauseMapCube[i].resize(m + 1);
            clauseMapFrom[i].resize(m + 1);
            clauseMapTo[i].resize(m + 1);
            for (int j = 1; j <= m; ++j) {
                clauseMap[i][j] = j;
            }
        }

        MyVector<Clause> rev(m);
        MyVector<Clause const*> cp(m);
        for (int j = 0; j < m; ++j) {
            rev[j].reserve(cnf[j].size() + 1);
            rev[j] = cnf[j];
            std::reverse(rev[j].begin(), rev[j].end());
            rev[j].push_back(0); // add dummy terminator
            cp[j] = &rev[j];
        }
        std::sort(cp.begin(), cp.end(), ClauseCmpForRelaxing());

        makeClauseMap(rev.data(), cp.data(), cp.data() + m, 0);
        packedClause = MyBitSetOnPool::newInstance(pool, m + 1);
    }

    void makeClauseMap(Clause const* base, Clause const* const * from,
            Clause const* const * to, int k) {
        for (Clause const* const * p = from; p < to - 1; ++p) {
            Clause const* const * q = p + 1;
            int t1 = (**p)[k];
            if (t1 == 0) continue;
            if ((**q)[k] != t1) continue;

            int i1 = levelOfLiteral(t1);
            int t2 = (**p)[k + 1];
            int i2 = (t2 != 0) ? levelOfLiteral(t2) : n + 1;

            int j1 = (*p - base) + 1;
            int j2 = (*q - base) + 1;
            while (i1 <= n && clauseMap[i1][j2] != j2) {
                ++i1;
            }
            if (i1 > n) continue;

            while (1) {
                for (int i = i1; i < i2; ++i) {
                    assert(clauseMap[i][j2] == j2);
                    clauseMap[i][j2] = j1;
                    if (clauseMapCube[i][j1].isNull()) {
                        clauseMapCube[i][j1] = clauseVar[j1];
                        clauseMapFrom[i][j1] = clauseVar[j1];
                        clauseMapTo[i][j1] = clauseVar[j1];
                    }
                    clauseMapCube[i][j1] &= clauseVar[j2];
                    clauseMapFrom[i][j1] |= clauseVar[j2];
                    clauseMapTo[i][j1] &= ~clauseVar[j2];
                }

                ++q;
                if (q == to || (**q)[k] != t1) break;
                j2 = (*q - base) + 1;
            }

            makeClauseMap(base, p, q, k + 1);
        }
    }

    /**
     * Makes rich reachability information.
     * @param limit the maximum BDD size in top-down traversal.
     */
    void traverse(size_t limit) {
        MessageHandler mh;
        mh.begin("symbolic state traversal");
        if (limit > 1) traverseTD(limit);
        traverseBU();
        mh.end(CuddBdd::peakLiveNodeCount());
    }

    /**
     * Makes rich reachability information from top to bottom.
     * @param limit the maximum BDD size.
     */
    void traverseTD(size_t limit) {
        MessageHandler mh;
        mh.begin("top-down traversal");

        for (int i = n - 1; i >= 0; --i) {
            MessageHandler mh;
            mh.begin("down") << " " << i << " ";
            CuddBdd f = frontierSet[i + 1];
            if (i + 2 <= n) f = f.cofactor(leaveConstraint[i + 2]); // faster than abstract()
            f &= enterConstraint[i + 1];
            mh << ".";
            CuddBdd p = f.abstract(posiCube[i + 1])
                    & (posiMask[i + 1] & leaveConstraint[i + 1]);
            mh << ".";
            CuddBdd q = f.abstract(negaCube[i + 1])
                    & (negaMask[i + 1] & leaveConstraint[i + 1]);
            mh << ".";
            CuddBdd g = p | q;
            if (g.size() > limit) {
                mh << " " << g.countMinterm(frontierClauses[i].size());
                mh << " <" << g.size() << "> ";
                do {
//                    CuddBdd v = CuddBdd(g.level(), 0, 1);
                    CuddBdd v = g.support();
                    if (v.isConstant()) break;
                    while (!v.child(1).isConstant()) {
                        v = v.child(1);
                    }
                    g = g.abstract(v);
                    mh << "#";
                } while (g.size() > limit);
            }
            frontierSet[i] = g;
            mh << " " // << std::fixed << std::setprecision(0)
                    << frontierSet[i].countMinterm(frontierClauses[i].size());
            mh.end(frontierSet[i].size());
        }

        mh.end(CuddBdd::sharingSize(frontierSet));
    }

    /**
     * Makes rich reachability information from bottom to top.
     */
    void traverseBU() {
        MessageHandler mh;
        mh.begin("bottom-up traversal");
        size_t totalStates = 0;
        frontierSet[0] = leaveConstraint[1];
        packedFrontierSet[0] = leaveConstraint[1];

        for (int i = 1; i <= n; ++i) {
            MessageHandler mh;
            mh.begin("up") << " " << i << " ";
            CuddBdd f = frontierSet[i - 1];
            mh << ".";
            //CuddBdd p = f.abstract(posiCube[i]).cofactor(enterConstraint[i]);
            CuddBdd p = f.cofactor(posiMask[i]).cofactor(enterConstraint[i]);
            if (i < n) p &= leaveConstraint[i + 1];
            mh << ".";
            //CuddBdd q = f.abstract(negaCube[i]).cofactor(enterConstraint[i]);
            CuddBdd q = f.cofactor(negaMask[i]).cofactor(enterConstraint[i]);
            if (i < n) q &= leaveConstraint[i + 1];
            mh << ".";
            frontierSet[i] &= p | q;

            f = frontierSet[i];
            //bool first = true;
            //for (int j = 1; j <= m; ++j) {
            for (int j = m; j >= 1; --j) {
                CuddBdd const& cube = clauseMapCube[i][j];
                if (cube.isNull()) continue;
                //if (first) {
                //    double states = f.countMinterm(frontierClauses[i].size());
                //    mh << " " // << std::fixed << std::setprecision(0)
                //            << states << " <" << f.size() << "> ";
                //    first = false;
                //}
                CuddBdd const& from = clauseMapFrom[i][j];
                CuddBdd const& to = clauseMapTo[i][j];
                f = (f & ~from) | ((f & from).abstract(cube) & to);
                //mh << ".";
            }
            packedFrontierSet[i] = f;

            double states = f.countMinterm(frontierClauses[i].size());
            totalStates += states;
            mh << " " // << std::fixed << std::setprecision(0)
                    << states;
            mh.end(f.size());
        }

        mh.end(CuddBdd::sharingSize(frontierSet));
        mh << "#state = " << totalStates << "\n";
    }

public:
    /**
     * Dumps the CNF in Graphviz (dot) format.
     * @param os the output stream.
     */
    void dumpCnf(std::ostream& os = std::cout, std::string title = "") const {
        os << "digraph \"" << title << "\" {\n";

        os << "  0 [shape=none,label=\"\"];\n";
        for (int v = 1; v <= n; ++v) {
            os << "  " << v << " [label=\"" << levelOfVar(v)
                    << "\",shape=none];\n";
        }

        os << "  0";
        for (int v = 1; v <= n; ++v) {
            os << " -> " << v;
        }
        os << " [style=invis];\n";

        for (int j = 1; j <= m; ++j) {
            os << "  c" << j << "_0 [label=\"" << j << "\",shape=none];\n";

            Clause const& c = cnf[j - 1];
            int from = std::abs(c[0]);
            int to = std::abs(c.back());
            MyVector<int> x(n + 1);
            for (typeof(c.begin()) t = c.begin(); t != c.end(); ++t) {
                x[std::abs(*t)] = *t;
            }

            for (int v = 1; v <= n; ++v) {
                os << "  c" << j << "_" << v;
                if (v < from || to < v) {
                    os << " [label=\"\",shape=none];\n";
                }
                else if (x[v] > 0) {
                    os << " [label=\"+" << v
                            << "\",style=filled,fillcolor=gray,fontcolor=black];\n";
                }
                else if (x[v] < 0) {
                    os << " [label=\"-" << v
                            << "\",style=filled,fillcolor=white,fontcolor=black];\n";
                }
                else {
                    os << " [label=\"\",shape=point];\n";
                }
            }

            os << "  c" << j << "_0";
            for (int v = 1; v <= from; ++v) {
                os << " -> c" << j << "_" << v;
            }
            os << " [style=invis];\n";

            if (from < to) {
                os << "  c" << j << "_" << from;
                for (int v = from + 1; v <= to; ++v) {
                    os << " -> c" << j << "_" << v;
                }
                os << " [style=bold,dir=none];\n";
            }

            if (to < n) {
                os << "  c" << j << "_" << to;
                for (int v = to + 1; v <= n; ++v) {
                    os << " -> c" << j << "_" << v;
                }
                os << " [style=invis];\n";
            }
        }

        for (int v = 0; v <= n; ++v) {
            os << "  {rank=same; " << v;
            for (int j = 1; j <= m; ++j) {
                os << "; c" << j << "_" << v;
            }
            os << "};\n";
        }

        for (int i = n; i >= 1; --i) {
            for (int j = 1; j <= m; ++j) {
                if (clauseMap[i][j] != j) {
                    os << "  c" << j << "_" << varAtLevel(i) << " -> c"
                            << clauseMap[i][j] << "_" << varAtLevel(i)
                            << " [color=navy];\n";
                }
            }
        }

        os << "}\n";
    }

private:
    MyVector<DdStructure> frontierZdd;
    MyVector<DataTable<int> > branchThreshold;
    MyVector<DdStructure> packedFrontierZdd;
    MyVector<DataTable<int> > packedBranchThreshold;
    MyVector<MyVector<int> > posiTrans;
    MyVector<MyVector<int> > negaTrans;

    void makeFrontierZdd(int level) {
        bool flag = MessageHandler::showMessages(false);
        frontierZdd[level] = DdStructure(
                bdd2zdd(frontierSet[level], frontierClauses[level]));
        frontierSet[level] = 0; //FIXME
        packedFrontierZdd[level] = DdStructure(
                bdd2zdd(packedFrontierSet[level], frontierClauses[level]));
        packedFrontierSet[level] = 0; //FIXME
        MessageHandler::showMessages(flag);

        computeThresholds(branchThreshold[level],
                *frontierZdd[level].getDiagram());
        computeThresholds(packedBranchThreshold[level],
                *packedFrontierZdd[level].getDiagram());

        MyVector<int>& pt = posiTrans[level];
        MyVector<int>& nt = negaTrans[level];
        ClauseList const& ec = enterClauses[level];
        ClauseList const& pc = posiClauses[level];
        ClauseList const& nc = negaClauses[level];
        int const en = ec.size();
        int const pn = pc.size();
        int const nn = nc.size();
        pt.reserve(ec.size() + pc.size() + 1);
        nt.reserve(ec.size() + nc.size() + 1);

        pt.push_back(0);
        for (int ei = 0, pi = 0; ei < en || pi < pn;) {
            if (ei < en && (pi >= pn || ec[ei] < pc[pi])) {
                pt.push_back(ec[ei]);
                ++ei;
            }
            else {
                assert(pi < pn);
                if (ei < en && ec[ei] == pc[pi]) ++ei;
                pt.push_back(-pc[pi]);
                ++pi;
            }
        }

        nt.push_back(0);
        for (int ei = 0, ni = 0; ei < en || ni < nn;) {
            if (ei < en && (ni >= nn || ec[ei] < nc[ni])) {
                nt.push_back(ec[ei]);
                ++ei;
            }
            else {
                assert(ni < nn);
                if (ei < en && ec[ei] == nc[ni]) ++ei;
                nt.push_back(-nc[ni]);
                ++ni;
            }
        }
    }

    void computeThresholds(DataTable<int>& bt,
            NodeTableEntity<2> const& diagram) {
        int n = diagram.numVars();
        bt.init(n + 1);
        bt.initRow(0, 2);
        bt[0][0] = 0;
        bt[0][1] = 1;
        for (int i = 1; i <= n; ++i) {
            const MyVector<Node<2> >& node = diagram[i];
            size_t m = node.size();
            bt.initRow(i, m);
            for (size_t j = 0; j < m; ++j) {
                NodeId f0 = node[j].branch[0];
                NodeId f1 = node[j].branch[1];
                bt[i][j] = bt[f0.row()][f0.col()] + bt[f1.row()][f1.col()];
            }
        }
        for (int i = n; i >= 1; --i) {
            const MyVector<Node<2> >& node = diagram[i];
            size_t m = node.size();
            for (size_t j = 0; j < m; ++j) {
                NodeId f0 = node[j].branch[0];
                bt[i][j] = bt[f0.row()][f0.col()];
            }
        }
        bt[0][1] = 0;
    }

public:
    int getRoot(State& s) {
        if (n == 0) return 0;
        s.packedId = 0;
        s.actualId = 0;
        frontierZdd.resize(n + 1);
        branchThreshold.resize(n + 1);
        packedFrontierZdd.resize(n + 1);
        packedBranchThreshold.resize(n + 1);
        posiTrans.resize(n + 1);
        negaTrans.resize(n + 1);
        makeFrontierZdd(n);
        makeFrontierZdd(n - 1);
        return n;
    }

    int getChild(State& s, int level, int take) {
        assert(level > 0);

        int id = s.actualId;
        int nextId = 0;
        int nextPackedId = 0;
        int nextLevel = level - 1;
        NodeId f = frontierZdd[level].root();
        NodeId nextF = frontierZdd[nextLevel].root();
        NodeId nextPackedF = packedFrontierZdd[nextLevel].root();
        NodeTableEntity<2> const& diagram = *frontierZdd[level].getDiagram();
        NodeTableEntity<2> const& nextDiagram =
                *frontierZdd[nextLevel].getDiagram();
        NodeTableEntity<2> const& nextPackedDiagram =
                *packedFrontierZdd[nextLevel].getDiagram();
        DataTable<int> const& threshold = branchThreshold[level];
        DataTable<int> const& nextThreshold = branchThreshold[nextLevel];
        DataTable<int> const& nextPackedThreshold =
                packedBranchThreshold[nextLevel];
        MyVector<int> const& trans = take ? posiTrans[level] : negaTrans[level];
        ClauseList const& nextMap = clauseMap[nextLevel];
        int t = trans.size() - 1;
        packedClause->clear();
        packedClause->add(m);

        //std::cerr << "\nLevel " << level << ", id = " << id << ", take = " << take << ", trans = " << trans;
        for (;;) {
            int i1 = f.row();
            int i2 = std::abs(trans[t]);
            int i3 = nextF.row();
            int i = std::max(std::max(i1, i2), i3);
            if (i <= 0) break;

            bool b = false;
            if (i1 == i) {
                int tmp = id - threshold[i1][f.col()];
                if (tmp >= 0) {
                    b = true;
                    id = tmp;
                }
                f = diagram.child(f, b);
            }
            //std::cerr << "\n  v" << i << " = " << b;
            if (i2 == i) {
                b = (trans[t] > 0);
                --t;
                //std::cerr << " -> " << b;
            }

            if (i3 == i) {
                if (b) nextId += nextThreshold[i3][nextF.col()];
                nextF = nextDiagram.child(nextF, b);
                if (nextF == 0) return 0;
            }
            else {
                if (b) return 0;
            }

            if (b) packedClause->add(m - nextMap[i]);
        }
        //std::cerr << "\nnextId = " << nextId;

        typeof(packedClause->begin()) pc = packedClause->begin();
        for (;;) {
            int i1 = m - *pc;
            int i2 = nextPackedF.row();
            int i = std::max(i1, i2);
            if (i <= 0) break;

            bool b = (i1 == i);
            if (b) ++pc;

            if (i2 == i) {
                if (b) nextPackedId +=
                        nextPackedThreshold[i2][nextPackedF.col()];
                nextPackedF = nextPackedDiagram.child(nextPackedF, b);
                assert(nextPackedF != 0);
            }
            else {
                assert(!b);
            }
        }

        s.packedId = nextPackedId;
        s.actualId = nextId;
        return (nextLevel > 0) ? nextLevel : -1;
    }

    void destructLevel(int i) {
        frontierZdd[i] = DdStructure();
        branchThreshold[i].init();
        if (i - 2 >= 0) makeFrontierZdd(i - 2);
    }

    size_t hashCode(State const& s) const {
        return s.packedId * 314159257;
    }

    bool equalTo(State const& s1, State const& s2) const {
        return s1.packedId == s2.packedId;
    }

    std::ostream& printState(std::ostream& os, State const& s) const {
        return os << s.packedId;
    }
};

} // namespace tdzdd
