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

#include "CnfToBdd.hpp"

typedef std::vector<int> Clause;

void CnfToBdd::readDimacs(std::istream& is) {
    tdzdd::MessageHandler mh;
    mh.begin("reading CNF") << " ...";

    std::string line;
    nv = nc = 0;

    while (is) {
        std::getline(is, line);
        if (sscanf(line.c_str(), "p cnf %d %d", &nv, &nc) == 2) break;
    }
    if (nv < 1 || nc < 1) throw std::runtime_error("CNF header line not found");

    cnf.reserve(cnf.size() + nc);
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
            if (std::abs(x) > nv) throw std::runtime_error(
                    "Variable ID is out of range");
            if (mm >= nc) throw std::runtime_error("Too many clauses");

            c.push_back(x);
        }
    }

    if (mm < nc) throw std::runtime_error("Too few clauses");
    mm = cnf.size();
    if (mm != nc) {
        mh << "\n" << (nc - mm) << " redundant clause"
                << (nc - mm == 1 ? "" : "s") << " found.";
        nc = mm;
    }

    mh.end();
    mh << "#var = " << nv << ", #clause = " << nc << "\n";
}

void CnfToBdd::sortClauses() {
    tdzdd::MessageHandler mh;
    mh.begin("sorting clauses") << " ...";
    std::sort(cnf.begin(), cnf.end(), ClauseCmpForOrdering());
    nc = std::unique(cnf.begin(), cnf.end(), ClauseEq()) - cnf.begin();
    cnf.resize(nc);
    mh.end();
}

void CnfToBdd::prepare() {
    pools.resize(nv + 1);

    clauseVar.resize(nc + 1);
    for (int j = 1; j <= nc; ++j) {
        clauseVar[j] = Cudd(j, 0, 1);
    }

    posiClauses.resize(nv + 1);
    negaClauses.resize(nv + 1);
    enterClauses.resize(nv + 1);
    leaveClauses.resize(nv + 1);
    frontierClauses.resize(nv + 1);
    posiCube.resize(nv + 1);
    negaCube.resize(nv + 1);
    posiMask.resize(nv + 1);
    negaMask.resize(nv + 1);
    enterCube.resize(nv + 1);
    leaveCube.resize(nv + 1);
    enterConstraint.resize(nv + 1);
    leaveConstraint.resize(nv + 1);
    frontierCube.resize(nv + 1);
    completingLevel = nv;
    frontierSet.resize(nv + 1);
    for (int i = 0; i <= nv; ++i) {
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

    for (int j = 1; j <= nc; ++j) {
        Clause const& c = cnf[j - 1];
        int const enterLevel = nv - std::abs(c[0]) + 1;
        int const leaveLevel = nv - std::abs(c.back()) + 1;
        assert(1 <= leaveLevel && leaveLevel <= enterLevel && enterLevel <= nv);

        for (typeof(c.begin()) t = c.begin(); t != c.end(); ++t) {
            if (*t > 0) {
                posiClauses[nv - *t + 1].push_back(j);
                posiCube[nv - *t + 1] &= clauseVar[j];
                posiMask[nv - *t + 1] &= ~clauseVar[j];
            }
            else {
                negaClauses[nv + *t + 1].push_back(j);
                negaCube[nv + *t + 1] &= clauseVar[j];
                negaMask[nv + *t + 1] &= ~clauseVar[j];
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

    clauseMap.resize(nv + 1);
    clauseMapCube.resize(nv + 1);
    clauseMapCond.resize(nv + 1);
    for (int i = 0; i <= nv; ++i) {
        clauseMap[i].resize(nc + 1);
        clauseMapCube[i].resize(nc + 1);
        clauseMapCond[i].resize(nc + 1);
        for (int j = 1; j <= nc; ++j) {
            clauseMap[i][j] = j;
        }
    }
    useClauseMap_ = true;

    std::vector<Clause> rev(nc);
    std::vector<Clause const*> cp(nc);
    for (int j = 0; j < nc; ++j) {
        rev[j].reserve(cnf[j].size() + 1);
        rev[j] = cnf[j];
        std::reverse(rev[j].begin(), rev[j].end());
        rev[j].push_back(0); // add dummy terminator
        cp[j] = &rev[j];
    }
    std::sort(cp.begin(), cp.end(), ClauseCmpForRelaxing());

    makeClauseMap(rev.data(), cp.data(), cp.data() + nc, 0);
}

void CnfToBdd::makeClauseMap(Clause const* base, Clause const* const * from,
        Clause const* const * to, int k) {
    for (Clause const* const * p = from; p < to - 1; ++p) {
        Clause const* const * q = p + 1;
        int t1 = (**p)[k];
        if (t1 == 0) continue;
        if ((**q)[k] != t1) continue;

        int i1 = levelOfLiteral(t1);
        int t2 = (**p)[k + 1];
        int i2 = (t2 != 0) ? levelOfLiteral(t2) : nv + 1;

        int j1 = (*p - base) + 1;
        int j2 = (*q - base) + 1;
        while (i1 <= nv && clauseMap[i1][j2] != j2) {
            ++i1;
        }
        if (i1 > nv) continue;

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

void CnfToBdd::traverse(size_t limit) {
    tdzdd::MessageHandler mh;
    mh.begin("symbolic state traversal");

    if (limit > 1) traverseTD(limit);
    traverseBU();

    mh.end(Cudd::peakLiveNodeCount());
}

void CnfToBdd::traverseTD(size_t limit) {
    tdzdd::MessageHandler mh;
    mh.begin("top-down traversal");

    for (int i = nv - 1; i >= 0; --i) {
        tdzdd::MessageHandler mh;
        mh.begin("down") << " " << i << " ";
        Cudd f = frontierSet[i + 1];
        if (i + 2 <= nv) f = f.cofactor(leaveConstraint[i + 2]); // faster than abstract()
        f &= enterConstraint[i + 1];
        mh << ".";
        Cudd p = f.abstract(posiCube[i + 1])
                & (posiMask[i + 1] & leaveConstraint[i + 1]);
        mh << ".";
        Cudd q = f.abstract(negaCube[i + 1])
                & (negaMask[i + 1] & leaveConstraint[i + 1]);
        mh << ".";
        Cudd g = p | q;
        if (g.size() > limit) {
            mh << " " << g.countMinterm(frontierClauses[i].size());
            mh << " <" << g.size() << "> ";
            do {
//                    Cudd v = Cudd(g.level(), 0, 1);
                Cudd v = g.support();
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

    mh.end(Cudd::sharingSize(frontierSet));
}

void CnfToBdd::traverseBU() {
    tdzdd::MessageHandler mh;
    mh.begin("bottom-up traversal");
    size_t totalStates = 0;
    frontierSet[0] = leaveConstraint[1];

    for (int i = 1; i <= nv; ++i) {
        tdzdd::MessageHandler mh;
        mh.begin("up") << " " << i << " ";
        Cudd f = frontierSet[i - 1];
        mh << ".";
        //Cudd p = f.abstract(posiCube[i]).cofactor(enterConstraint[i]);
        Cudd p = f.cofactor(posiMask[i]).cofactor(enterConstraint[i]);
        if (i < nv) p &= leaveConstraint[i + 1];
        mh << ".";
        //Cudd q = f.abstract(negaCube[i]).cofactor(enterConstraint[i]);
        Cudd q = f.cofactor(negaMask[i]).cofactor(enterConstraint[i]);
        if (i < nv) q &= leaveConstraint[i + 1];
        mh << ".";
        frontierSet[i] &= p | q;
        double states = frontierSet[i].countMinterm(frontierClauses[i].size());
        totalStates += states;
        mh << " " // << std::fixed << std::setprecision(0)
                << states;
        mh.end(frontierSet[i].size());
    }

    mh.end(Cudd::sharingSize(frontierSet));
    mh << "#state = " << totalStates << "\n";
}

void CnfToBdd::dumpCnf(std::ostream& os, std::string title) const {
    os << "digraph \"" << title << "\" {\n";

    os << "  0 [shape=none,label=\"\"];\n";
    for (int v = 1; v <= nv; ++v) {
        os << "  " << v << " [label=\"" << levelOfVar(v) << "\",shape=none];\n";
    }

    os << "  0";
    for (int v = 1; v <= nv; ++v) {
        os << " -> " << v;
    }
    os << " [style=invis];\n";

    for (int j = 1; j <= nc; ++j) {
        os << "  c" << j << "_0 [label=\"" << j << "\",shape=none];\n";

        Clause const& c = cnf[j - 1];
        int from = std::abs(c[0]);
        int to = std::abs(c.back());
        std::vector<int> x(nv + 1);
        for (typeof(c.begin()) t = c.begin(); t != c.end(); ++t) {
            x[std::abs(*t)] = *t;
        }

        for (int v = 1; v <= nv; ++v) {
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

        if (to < nv) {
            os << "  c" << j << "_" << to;
            for (int v = to + 1; v <= nv; ++v) {
                os << " -> c" << j << "_" << v;
            }
            os << " [style=invis];\n";
        }
    }

    for (int v = 0; v <= nv; ++v) {
        os << "  {rank=same; " << v;
        for (int j = 1; j <= nc; ++j) {
            os << "; c" << j << "_" << v;
        }
        os << "};\n";
    }

    for (int i = nv; i >= 1; --i) {
        for (int j = 1; j <= nc; ++j) {
            if (clauseMap[i][j] != j) {
                os << "  c" << j << "_" << varAtLevel(i) << " -> c"
                        << clauseMap[i][j] << "_" << varAtLevel(i)
                        << " [color=navy];\n";
            }
        }
    }

    os << "}\n";
}

int CnfToBdd::getRoot(State& s) {
    if (nv == 0) return 0;
    s.set = ClauseSet::newInstance(pools[nv], 0);
    s.id = s.set;
    return nv;
}

int CnfToBdd::getChild(State& s, int level, int take) {
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

    --level;
    if (badState(work, level)) return 0;
    if (level <= completingLevel && work.empty()) return -1;

    s.set = ClauseSet::newInstance(pools[level], work);
    if (useClauseMap_) {
        for (size_t k = 0; k < work.size(); ++k) {
            work[k] = clauseMap[level][work[k]];
        }
        //std::sort(work.begin(), work.end());
        s.id = ClauseSet::newInstance(pools[level], work);
    }
    else {
        s.id = s.set;
    }

    return level;
}

bool CnfToBdd::badState(std::vector<uint16_t>& clauses, int level) const {
    DdNode* zero = Cudd(0).ddNode();
    DdNode* g = frontierSet[level].ddNode();
    if (Cudd_IsConstant(g)) return g == zero;
    typeof(clauses.rbegin()) t = clauses.rbegin();

    while (t != clauses.rend()) {
        if (Cudd_NodeReadIndex(g) == int(*t)) { // 1
            g = Cudd_NotCond(Cudd_T(g), Cudd_IsComplement(g));
            if (Cudd_IsConstant(g)) return g == zero;
            ++t;
        }
        else {
            while (Cudd_NodeReadIndex(g) > int(*t)) { // 0
                g = Cudd_NotCond(Cudd_E(g), Cudd_IsComplement(g));
                if (Cudd_IsConstant(g)) return g == zero;
            }
            while (int(*t) > Cudd_NodeReadIndex(g)) { // don't care
                ++t;
            }
        }
    }

    while (!Cudd_IsConstant(g)) {
        g = Cudd_NotCond(Cudd_E(g), Cudd_IsComplement(g));
    }
    return g == 0;
}

void CnfToBdd::destructLevel(int i) {
    pools[i].clear();
}
