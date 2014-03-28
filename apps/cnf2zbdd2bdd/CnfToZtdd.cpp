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

#include "CnfToZtdd.hpp"
#include <algorithm>

typedef CnfToZtddState::Clause Clause;

namespace{

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

struct ClauseCmp {
    bool operator()(Clause const& a, Clause const& b) const {
        int n = std::min(a.size(), b.size());
        for (int i = 0; i < n; ++i) {
            if (a[i] == -b[i]) return a[i] < b[i];
            if (std::abs(a[i]) != std::abs(b[i])) return std::abs(a[i])
                    > std::abs(b[i]);
        }
        return a.size() < b.size();
    }
};

struct ClauseEq {
    bool operator()(Clause const& a, Clause const& b) const {
        return a == b;
    }
};

class ClauseSearcher {
    size_t pos;

public:
    ClauseSearcher(int pos)
            : pos(pos) {
    }

    bool operator()(Clause const& a, int b) const {
        if (a.size() <= size_t(pos)) return true;
        if (a[pos] == -b) return a[pos] < b;
        return std::abs(a[pos]) > std::abs(b);
    }
};

} // namespace

void CnfToZtdd::readDimacs(std::istream& is) {
    tdzdd::MessageHandler mh;
    mh.begin("reading CNF") << " ...";

    std::string line;
    nv = 0;
    int nc = 0;

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

void CnfToZtdd::sortClauses() {
    tdzdd::MessageHandler mh;
    mh.begin("sorting clauses") << " ...";
    std::sort(cnf.begin(), cnf.end(), ClauseCmp());
    int nc = std::unique(cnf.begin(), cnf.end(), ClauseEq()) - cnf.begin();
    cnf.resize(nc);
    mh.end();
}

int CnfToZtdd::getRoot(State& s) {
    if (cnf.empty()) return 0;
    if (cnf.size() == 1 && cnf.front().empty()) return -1;

    s.beg = cnf.data();
    s.end = cnf.data() + cnf.size();
    s.pos = 0;

    assert(!(s.end - 1)->empty());
    int nextVar = std::abs((s.end - 1)->front());
    int nextLevel = levelOfVar(nextVar);
    assert(nextLevel <= nv);
    return nextLevel;
}

int CnfToZtdd::getChild(State& s, int level, int value) {
    assert(s.beg < s.end);
    ClauseSearcher searcher(s.pos);
    int v = varAtLevel(level);

    switch (value) {
    case 0:
        s.end = std::lower_bound(s.beg, s.end, -v, searcher);
        break;
    case 1:
        s.beg = std::lower_bound(s.beg, s.end, -v, searcher);
        s.end = std::lower_bound(s.beg, s.end, v, searcher);
        ++s.pos;
        break;
    default:
        s.beg = std::lower_bound(s.beg, s.end, v, searcher);
        ++s.pos;
        break;
    }

    if (s.beg == s.end) return 0;
    if (s.beg == s.end - 1 && s.beg->size() <= s.pos) return -1;

    assert(s.pos < (s.end - 1)->size());
    int nextVar = std::abs((s.end - 1)->at(s.pos));
    int nextLevel = levelOfVar(nextVar);
    assert(nextLevel < level);
    return nextLevel;
}
