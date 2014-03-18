/*
 * Top-Down ZDD Construction Library for Frontier-Based Search
 * by Hiroaki Iwashita <iwashita@erato.ist.hokudai.ac.jp>
 * Copyright (c) 2012 Japan Science and Technology Agency
 * $Id: ddqueens.cpp 518 2014-03-18 05:29:23Z iwashita $
 */

#include <cerrno>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <map>
#include <string>

#include "dd/InstanceFinder.hpp"
#include "dd/DdStructure.hpp"
#include "eval/Cardinality.hpp"
#include "op/ZddLookahead.hpp"
#include "spec/ColoredZdd.hpp"
#include "spec/NAryZdd.hpp"
#include "spec/NQueenZdd.hpp"
#include "spec/ZddInterleave.hpp"

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
        DdStructure dd;

        if (opt["colored1"]) {
            ColoredNQueenZdd cnq(n);

            if (opt["sat"]) {
                InstanceFinder<ColoredNQueenZdd>(cnq).find();
            }
            else {
                dd = DdStructure(cnq, opt["p"]);
                dd.zddReduce(opt["p"]);
            }
        }
        else if (opt["colored"]) {
//            dd = OneHotNAryZdd(n, n * n);
//
//            std::vector<int> list;
//            list.reserve(n * n * n);
//            int level = 1;
//            for (int i = 0; i < n; ++i) {
//                for (int j = 0; j < n; ++j) {
//                    for (int k = 0; k < n; ++k) {
//                        if (i == n - 1) {
//                            if (j == k) {
//                                list.push_back(level);
//                            }
//                        }
//                        else {
//                            list.push_back(-level);
//                        }
//                        ++level;
//                    }
//                }
//            }
//            dd &= TableToZdd({list});
//
//            DdStructure nqs;
//            if (opt["rook"]) {
//                nqs = NRookZdd(n);
//                nqs &= NQueenZdd(n);
//            }
//            else {
//                nqs = NQueenZdd(n);
//            }
//
//            if (opt["sat"]) {
//            }
//            else if (false) {
//                for (int k = 0; k < n; ++k) {
//                    dd &= ZddInterleave(nqs, n, k);
//                }
//            }
//            else {
//                dd &= ZddInterleave(nqs, n);
//            }
            std::vector<DdStructure> nqs(n);
            for (int k = 0; k < n; ++k) {
                NQueenZdd nq(n, k);
                nqs[k] = DdStructure(nq, opt["p"]);
                nqs[k].zddReduce(opt["p"]);
            }

            if (opt["sat"]) {
                ColoredZdd c(nqs);
                InstanceFinder<ColoredZdd>(c).find();
//                DdStructure oh(OneHotNAryZdd(n, n * n));
//                DdStructure dc(n * n);
//                std::vector<DdStructure> ddk(n);
//                DdStructure::And dda;
//
//                for (int k = n - 1; k >= 0; --k) {
//                    std::vector<DdStructure> nqk(n);
//
//                    for (int kk = 0; kk < n; ++kk) {
//                        nqk[kk] = (kk == k) ? nqs[kk] : dc;
//                    }
//
//                    ddk[k] = oh;
//                    ddk[k] &= ColoredZdd(nqk).lookahead();
//                    dda &= ddk[k];
//                }
//
//                dda.findInstance();
            }
            else {
                //dd = ColoredZdd(nqs).lookahead();
//                while (dds.size() >= 2) {
//                    std::sort(dds.begin(), dds.end(),
//                            [](DdStructure const& a, DdStructure const& b) {
//                                return a.size() > b.size();
//                            });
//
//                    int k = dds.size() - 2;
//                    dds[k] &= dds[k + 1];
//                    dds.pop_back();
//                }
                OneHotNAryZdd ohna(n, n * n);
                DdStructure oh = DdStructure(ohna);
                DdStructure dc(n * n);

                for (int k = n - 1; k >= 0; --k) {
                    std::vector<DdStructure> nqk(n);

                    for (int kk = 0; kk < n; ++kk) {
                        nqk[kk] = (kk == k) ? nqs[kk] : dc;
                    }

                    DdStructure ddk(oh);
                    //ddk.zddSubset(zddLookahead(ColoredZdd(nqk)));
                    ColoredZdd c(nqk);
                    ddk.zddSubset(c, opt["p"]);
                    ddk.zddReduce(opt["p"]);

                    if (k == n - 1) {
                        dd = ddk;
                    }
                    else {
                        dd.zddSubset(ddk, opt["p"]);
                        dd.zddReduce(opt["p"]);
                    }
                }
            }
        }
        else {
            NQueenZdd nq(n);

            if (opt["rook"]) {
                NRookZdd nr(n);
                dd = DdStructure(nr, opt["p"]);
                dd.zddReduce(opt["p"]);
                dd.zddSubset(nq, opt["p"]);
                dd.zddReduce(opt["p"]);
            }
            else {
                dd = DdStructure(nq, opt["p"]);
                dd.zddReduce(opt["p"]);
            }
        }

        if (opt["dump"]) dump(std::cout, dd);

        if (!opt["noreport"] && !opt["sat"]) {
            mh << "#node = " << dd.size() << ", #solution = "
                    << std::setprecision(10)
                    << dd.evaluate(Cardinality<double>()) << "\n";
        }
    }
    catch (std::exception& e) {
        mh << e.what() << "\n";
        return 1;
    }

    mh.end("finished");
    return 0;
}
