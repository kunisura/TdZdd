/*
 * Top-Down ZDD Construction Library for Frontier-Based Search
 * by Hiroaki Iwashita <iwashita@erato.ist.hokudai.ac.jp>
 * Copyright (c) 2014 Japan Science and Technology Agency
 * $Id: CnfBdd140311.hpp 518 2014-03-18 05:29:23Z iwashita $
 */

#pragma once

#include <cassert>
#include <iostream>

#include "../dd/DdSpec.hpp"
#include "../spec/CuddBdd.hpp"
#include "../util/MessageHandler.hpp"
#include "../util/MySet.hpp"
#include "../util/MyVector.hpp"

namespace tdzdd {

struct CnfBdd140311State {
    typedef int ClauseNumber;
    typedef MySmallSetOnPool<ClauseNumber> ClauseSet;

    ClauseSet* set;
    ClauseSet* id;
};

class CnfBdd140311: public ScalarDdSpec<CnfBdd140311,CnfBdd140311State> {
    typedef CnfBdd140311State::ClauseNumber ClauseNumber;
    typedef CnfBdd140311State::ClauseSet ClauseSet;
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
    MyVector<MyVector<CuddBdd> > clauseMapCond; // union of source clauses for each level

    MyVector<CuddBdd> frontierSet; // reachable state set for each level

    MemoryPools pools;
    MyVector<uint16_t> work;

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
        pools.resize(n + 1);

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
        clauseMapCond.resize(n + 1);
        for (int i = 0; i <= n; ++i) {
            clauseMap[i].resize(m + 1);
            clauseMapCube[i].resize(m + 1);
            clauseMapCond[i].resize(m + 1);
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
                        clauseMapCond[i][j1] = clauseVar[j1];
                    }
                    clauseMapCube[i][j1] &= clauseVar[j2];
                    clauseMapCond[i][j1] |= clauseVar[j2];
                }

                ++q;
                if (q == to || (**q)[k] != t1) break;
                j2 = (*q - base) + 1;
            }

            makeClauseMap(base, p, q, k + 1);
        }
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
            double states = frontierSet[i].countMinterm(frontierClauses[i].size());
            totalStates += states;
            mh << " " // << std::fixed << std::setprecision(0)
                    << states;
            mh.end(frontierSet[i].size());
        }

        mh.end(CuddBdd::sharingSize(frontierSet));
        mh << "#state = " << totalStates << "\n";
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
     * Relaxes state restrictions by adding equivalent states.
     */
    void relax() {
        MessageHandler mh;
        mh.begin("relaxing");
        mh.setSteps(n);

        for (int i = n; i >= 0; --i) {
//            MessageHandler mh;
//            mh.begin("relax") << " " << i << " ...";
            CuddBdd& f = frontierSet[i];

            for (int j = 1; j <= m; ++j) {
                CuddBdd const& cube = clauseMapCube[i][j];
                CuddBdd const& cond = clauseMapCond[i][j];
                if (cube.isNull()) continue;
                f |= (f & cond).abstract(cube) & cond;
            }

//            mh << " " << std::fixed << std::setprecision(0)
//                    << f.countMinterm(frontierClauses[i].size());
//            mh.end(f.size());
            mh.step();
        }

        mh.end();
    }

//    void relaxStep(Clause const* base, Clause const* const * from,
//            Clause const* const * to, int k) {
//        for (Clause const* const * p = from; p < to - 1; ++p) {
//            Clause const* const * q = p + 1;
//            int t1 = (**p)[k];
//            if (t1 == 0) continue;
//            if ((**q)[k] != t1) continue;
//
//            int i1 = levelOfLiteral(t1);
//            int j1 = (*p - base) + 1;
//            while (clauseMap[i1][j1] != j1) {
//                ++i1;
//            }
//
//            int t2 = (**p)[k + 1];
//            int i2 = levelOfLiteral(t2);
//            CuddBdd cube = clauseVar[j1];
//            CuddBdd cond = clauseVar[j1];
//
//            do {
//                int j2 = (*q - base) + 1;
//                cube &= clauseVar[j2];
//                cond |= clauseVar[j2];
//                for (int i = i1; i < i2; ++i) {
//                    assert(clauseMap[i][j2] == j2);
//                    clauseMap[i][j2] = j1;
//                }
//            } while ((**++q)[k] == t1);
//
//            for (int i = i1; i < i2; ++i) {
//                doRelax(i, cube, cond);
//            }
//
//            relaxStep(base, p, q, k + 1);
//        }
//    }

//    void doRelax(int i, CuddBdd const& cube, CuddBdd const& cond) {
//        CuddBdd& f = frontierSet[i];
////        MessageHandler mh;
////        mh.begin("relax") << " " << i << ": "
////                << size_t(f.countMinterm(frontierClauses[i].size())) << " <"
////                << f.size() << "> ...";
//        f |= (f & cond).abstract(cube) & cond;
////        mh << " " << size_t(f.countMinterm(frontierClauses[i].size()));
////        mh.end(f.size());
//    }

public:
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
        //relax();
    }

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
    template<typename C>
    bool badState(C const& clauses, int level) const {
        CuddBdd g = frontierSet[level];
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

public:
    int getRoot(State& s) {
        if (n == 0) return 0;
        s.set = ClauseSet::newInstance(pools[n], 0);
        s.id = s.set;
        return n;
    }

    int getChild(State& s, int level, int take) {
        assert(level > 0);

        ClauseSet const& set = *s.set;
        ClauseList const& ent = enterClauses[level];
        ClauseList const& sat = take ? posiClauses[level] : negaClauses[level];
        typeof(set.begin()) a = set.begin();
        typeof(ent.begin()) b = ent.begin();
        typeof(sat.begin()) c = sat.begin();
        typeof(set.end()) az = set.end();
        typeof(ent.end()) bz = ent.end();
        typeof(sat.end()) cz = sat.end();
        work.resize(0);
        //std::cerr << "\n" << *s << "+" << ent << "-" << sat;//TODO

        // compute {a} + {b} - {c}
        while (a != az || b != bz) {
            if (a != az && (b == bz || *a < *b)) {
                while (c != cz && *c < *a) {
                    ++c;
                }
                if (c != cz && *c == *a) {
                    ++c;
                }
                else {
                    work.push_back(*a);
                }
                ++a;
            }
            else if (b != bz) {
                while (c != cz && *c < *b) {
                    ++c;
                }
                if (c != cz && *c == *b) {
                    ++c;
                }
                else {
                    work.push_back(*b);
                }
                if (a != az && *a == *b) ++a;
                ++b;
            }
        }
        //std::cerr << " -> " << work;//TODO

        --level;
        if (badState(work, level)) return 0;
        if (level <= completingLevel && work.empty()) return -1;

        s.set = ClauseSet::newInstance(pools[level], work);
        for (size_t k = 0; k < work.size(); ++k) {
            work[k] = clauseMap[level][work[k]];
        }
        s.id = ClauseSet::newInstance(pools[level], work);

        return level;
    }

    void destructLevel(int i) {
        pools[i].clear();
    }

    size_t hashCode(State const& s) const {
        return s.id->hash();
    }

    bool equalTo(State const& s1, State const& s2) const {
        return *s1.id == *s2.id;
    }

    std::ostream& printState(std::ostream& os, State const& s) const {
        return os << *s.id;
    }
};

} // namespace tdzdd
