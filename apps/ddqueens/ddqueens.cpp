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

#include <cerrno>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <map>
#include <string>

#include <tdzdd/DdStructure.hpp>

#include "ColoredZdd.hpp"
#include "NAryZdd.hpp"
#include "NQueenZdd.hpp"
#include "ZddInterleave.hpp"

using namespace tdzdd;

std::string options[][2] = { //
        {"colored", "Solve the coloed N-queen problem"}, //
        {"colored1", "Solve the coloed N-queen problem by a single ZDD spec"}, //
        {"sat", "Try to find a single instance"}, //
        {"rook", "Solve the rook constraint at first"}, //
        {"p", "Use parallel algorithms"}, //
        {"dump", "Dump result ZDD to STDOUT in DOT format"}, //
        {"noreport", "Do not print final report"}};

std::map<std::string,bool> opt;
int n = 0;

void usage(char const* cmd) {
    std::cerr << "usage: " << cmd << " <option>... <size>\n";
    std::cerr << "options\n";
    for (unsigned i = 0; i < sizeof(options) / sizeof(options[0]); ++i) {
        std::cerr << "  -" << options[i][0];
        for (unsigned j = options[i][0].length(); j < 10; ++j) {
            std::cerr << " ";
        }
        std::cerr << ": " << options[i][1] << "\n";
    }
}

std::string to_string(int i) {
    std::ostringstream oss;
    oss << i;
    return oss.str();
}

struct NodeNamer {
    std::string operator()(NodeId f) {
        if (opt["colored"] || opt["colored1"] || opt["coloredN"]) {
            int m = n * n;
            int i = m * n - f.row();
            int j = i % m;
            return to_string(i / m) + "," + to_string(j / n) + ","
                    + char('A' + (j % n));
        }
        else {
            int i = n * n - f.row();
            return to_string(i / n) + "," + to_string(i % n);
        }
    }
};

template<typename DD>
void dump(std::ostream& os, DD const& dd) {
    //dd.dumpDot(os, "", NodeNamer());
    dd.dumpDot(os, "");
}

int main(int argc, char *argv[]) {
    for (unsigned i = 0; i < sizeof(options) / sizeof(options[0]); ++i) {
        opt[options[i][0]] = false;
    }

    for (int i = 1; i < argc; ++i) {
        std::string s = argv[i];
        if (s[0] == '-') {
            s = s.substr(1);
            if (opt.count(s)) {
                opt[s] = true;
            }
            else {
                usage(argv[0]);
                return 1;
            }
        }
        else if (n == 0) {
            n = std::atoi(argv[i]);
        }
        else {
            usage(argv[0]);
            return 1;
        }
    }

    if (n < 1) {
        usage(argv[0]);
        return 1;
    }

    MessageHandler::showMessages();
    MessageHandler mh;
    mh.begin("started") << " (" << n << "x" << n << "=" << n * n << ")\n";

    try {
        DdStructure<2> dd;

        if (opt["colored1"]) {
            ColoredNQueenZdd cnq(n);
            dd = DdStructure<2>(cnq, opt["p"]);
            dd.zddReduce();
        }
        else if (opt["colored"]) {
            std::vector<DdStructure<2> > nqs(n);
            for (int k = 0; k < n; ++k) {
                NQueenZdd nq(n, k);
                nqs[k] = DdStructure<2>(nq, opt["p"]);
                nqs[k].zddReduce();
            }

            OneHotNAryZdd ohna(n, n * n);
            DdStructure<2> oh(ohna);
            DdStructure<2> dc(n * n);

            for (int k = n - 1; k >= 0; --k) {
                std::vector<DdStructure<2> > nqk(n);

                for (int kk = 0; kk < n; ++kk) {
                    nqk[kk] = (kk == k) ? nqs[kk] : dc;
                }

                DdStructure<2> ddk(oh);
                //ddk.zddSubset(zddLookahead(ColoredZdd(nqk)));
                ColoredZdd c(nqk);
                ddk.zddSubset(c);
                ddk.zddReduce();

                if (k == n - 1) {
                    dd = ddk;
                }
                else {
                    dd.zddSubset(ddk);
                    dd.zddReduce();
                }
            }
        }
        else {
            NQueenZdd nq(n);

            if (opt["rook"]) {
                NRookZdd nr(n);
                dd = DdStructure<2>(nr, opt["p"]);
                dd.zddReduce();
                dd.zddSubset(nq);
                dd.zddReduce();
            }
            else {
                dd = DdStructure<2>(nq, opt["p"]);
                dd.zddReduce();
            }
        }

        if (opt["dump"]) dump(std::cout, dd);

        if (!opt["noreport"]) {
            mh << "#node = " << dd.size() << ", #solution = "
                    << std::setprecision(10)
                    << dd.evaluate(ZddCardinality<double>()) << "\n";
        }
    }
    catch (std::exception& e) {
        mh << e.what() << "\n";
        return 1;
    }

    mh.end("finished");
    return 0;
}
