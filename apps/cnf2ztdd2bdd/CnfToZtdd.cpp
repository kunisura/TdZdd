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

size_t CnfToZtddState::hash() const {
    size_t h = 0;
    for (Clause const* p = beg; p != end; ++p) {
        for (size_t i = pos; i < p->size(); ++i) {
            h += (*p)[i];
            h *= 314159257;
        }
    }
    return h;
}

bool CnfToZtddState::operator==(CnfToZtddState const& o) const {
    if (end - beg != o.end - o.beg) return false;
    typedef Clause const* ClausePtr;
    for (ClausePtr p = beg, q = o.beg; p != end; ++p, ++q) {
        if (p->size() - pos != q->size() - o.pos) return false;
        for (size_t i = pos, j = o.pos; i < p->size(); ++i, ++j) {
            if ((*p)[i] != (*q)[j]) return false;
        }
    }
    return true;
}

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

void CnfToZtdd::printState(std::ostream& os, State const& s) const {
//        os << "[" << (s.beg - cnf.data()) << "," << (s.end - cnf.data()) << "]+"
//                << s.pos;
    for (Clause const* p = s.beg; p != s.end; ++p) {
        os << "(";
        for (size_t i = s.pos; i < p->size(); ++i) {
            if (i != s.pos) os << " ";
            os << (*p)[i];
        }
        os << ")";
    }
}
