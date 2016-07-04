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
#include <climits>
#include <cstdlib>
#include <iostream>
#include <map>
#include <stdexcept>
#include <vector>

#include <tdzdd/DdStructure.hpp>

#include "CnfToBdd.hpp"

using namespace tdzdd;

std::string options[][2] = { //
        {"s", "Sort input clauses"}, //
        {"n", "Do not perform top-down/bottom-up reachability analysis"}, //
        {"b", "Bottom-up reachability analysis only; synonym of \"limit 0\""}, //
        {"limit <n>", "Limit BDD size for top-down reachability analysis"}, //
        {"c", "Disable mapping to canonical clause IDs"}, //
        {"p", "Use parallel algorithms"}, //
        {"cnf", "Dump the input CNF to STDOUT"}, //
        {"dd0", "Dump a state transition graph to STDOUT in DOT format"}, //
        {"dd1", "Dump a BDD before reduction to STDOUT in DOT format"}, //
        {"dump", "Dump the final BDD to STDOUT in DOT format"}}; //

std::map<std::string,bool> opt;
std::map<std::string,int> optNum;
std::string infile;
std::string outfile;

void usage(char const* cmd) {
    std::cerr << "usage: " << cmd
              << " [ <option>... ] <input_file> [ <output_file> ]\n";
    std::cerr << "options\n";
    for (unsigned i = 0; i < sizeof(options) / sizeof(options[0]); ++i) {
        std::cerr << "  -" << options[i][0];
        for (unsigned j = options[i][0].length(); j < 10; ++j) {
            std::cerr << " ";
        }
        std::cerr << ": " << options[i][1] << "\n";
    }
    std::cerr << "\n";
}

void output(std::ostream& os, DdStructure<2> const& f, CnfToBdd const& cnf) {
    for (DdStructure<2>::const_iterator t = f.begin(); t != f.end(); ++t) {
        for (int i = 1; i <= cnf.numVars(); ++i) {
            if (i != 1) os << " ";
            os << (t->count(cnf.levelOfVar(i)) ? "+" : "-") << i;
        }
        os << "\n";
    }
}

void run() {
    MessageHandler mh;
    CnfToBdd cnf;

    mh << "\nINPUT: " << infile;
    if (!opt["limit"]) optNum["limit"] = INT_MAX;
    if (opt["b"]) optNum["limit"] = 0;
    if (infile == "-") {
        cnf.load(std::cin, opt["s"]);
    }
    else {
        std::ifstream ifs(infile.c_str());
        if (!ifs) throw std::runtime_error(strerror(errno));
        cnf.load(ifs, opt["s"]);
    }

    cnf.useClauseMap(!opt["c"]);
    if (!opt["n"]) cnf.traverse(optNum["limit"]);

    if (opt["cnf"]) cnf.dumpCnf(std::cout, "CNF");
    if (opt["dd0"]) cnf.dumpDot(std::cout, "dd0");

    DdStructure<2> dd(cnf, opt["p"]);
    if (opt["dd1"]) dd.dumpDot(std::cout, "dd1");
    dd.bddReduce();
    if (opt["dump"]) dd.dumpDot(std::cout, "BDD");
    mh << "\n#solution = " << dd.evaluate(BddCardinality<>(cnf.numVars()));

    if (!outfile.empty()) {
        DdStructure<2> zdd = dd.bdd2zdd(cnf.numVars());
        mh << "\nOUTPUT: " << outfile << "\n";
        if (outfile == "-") {
            output(std::cout, zdd, cnf);
        }
        else {
            std::ofstream ofs(outfile.c_str());
            if (!ofs) throw std::runtime_error(strerror(errno));
            output(ofs, zdd, cnf);
        }
    }
}

int main(int argc, char *argv[]) {
    for (unsigned i = 0; i < sizeof(options) / sizeof(options[0]); ++i) {
        opt[options[i][0]] = false;
    }

    try {
        for (int i = 1; i < argc; ++i) {
            std::string s = argv[i];
            if (s[0] == '-' && s.size() >= 2) {
                s = s.substr(1);
                if (opt.count(s)) {
                    opt[s] = true;
                }
                else if (i + 1 < argc && opt.count(s + " <n>")) {
                    opt[s] = true;
                    optNum[s] = std::atoi(argv[++i]);
                }
                else {
                    usage(argv[0]);
                    return 1;
                }
            }
            else if (infile.empty()) {
                infile = argv[i];
            }
            else if (outfile.empty()) {
                outfile = argv[i];
            }
            else {
                throw std::exception();
            }
        }

        if (infile.empty()) throw std::exception();
    }
    catch (std::exception& e) {
        usage(argv[0]);
        return 1;
    }

    std::ios::sync_with_stdio(false); // for speed-up
    MessageHandler::showMessages();
    MessageHandler mh;
    mh.begin("started");

    try {
        run();
    }
    catch (std::exception& e) {
        mh << e.what() << "\n";
        return 1;
    }

    mh.end("finished");
    return 0;
}
