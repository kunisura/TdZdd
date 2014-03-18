/*
 * Top-Down ZDD Construction Library for Frontier-Based Search
 * by Hiroaki Iwashita <iwashita@erato.ist.hokudai.ac.jp>
 * Copyright (c) 2013 Japan Science and Technology Agency
 * $Id: cnf2bdd.cpp 518 2014-03-18 05:29:23Z iwashita $
 */

#include <cerrno>
#include <climits>
#include <cstdlib>
#include <iostream>
#include <map>
#include <stdexcept>
#include <vector>

#include "dd/DdStructure.hpp"
#include "eval/Cardinality.hpp"
#include "spec/CnfBdd140311.hpp"
#include "util/MessageHandler.hpp"

using namespace tdzdd;

std::string options[][2] = { //
        {"b", "Only bottom-up reachability analysis; synonym of \"limit 0\""}, //
        {"s", "Sort input clauses"}, //
        {"limit <n>",
         "Abstract top-down reachability analysis when BDD size is more than <n>"}, //
        {"p", "Use parallel algorithms"}, //
        {"cnf", "Dump the input CNF to STDOUT"}, //
        {"dd0", "Dump a state transition graph to STDOUT in DOT format"}, //
        {"dd1", "Dump a ZDD before reduction to STDOUT in DOT format"}, //
        {"dd2", "Dump the final ZDD to STDOUT in DOT format"}}; //

std::map<std::string,bool> opt;
std::map<std::string,int> optNum;
std::string infile;
std::string outfile;

void usage(char const* cmd) {
    std::cerr << "usage: " << cmd
            << " [<option>...] [<input_file> [<output_file>]]\n";
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

void output(std::ostream& os, DdStructure const& f, CnfBdd140311 const& cnf) {
    for (typeof(f.begin()) t = f.begin(); t != f.end(); ++t) {
        for (size_t i = 0; i < t->size(); ++i) {
            if (i != 0) std::cout << " ";
            os << cnf.varAtLevel((*t)[i]);
        }
        os << "\n";
    }
}

void run() {
    MessageHandler mh;
    CnfBdd140311 cnf;

    mh << "\nINPUT: " << infile;
    if (!opt["limit"]) optNum["limit"] = INT_MAX;
    if (opt["b"]) optNum["limit"] = 0;
    if (infile == "-") {
        cnf.load(std::cin, opt["s"], optNum["limit"]);
    }
    else {
        std::ifstream ifs(infile.c_str());
        if (!ifs) throw std::runtime_error(strerror(errno));
        cnf.load(ifs, opt["s"], optNum["limit"]);
    }

    if (opt["cnf"]) cnf.dumpCnf(std::cout);
    if (opt["dd0"]) cnf.dumpDot(std::cout, "dd0");

    DdStructure dd(cnf);
    if (opt["dd1"]) dd.dumpDot(std::cout, "dd1");
    dd.bddReduce(opt["p"]);
    if (opt["dd2"]) dd.dumpDot(std::cout, "dd2");
    mh << "\n#solution = " << dd.evaluate(Cardinality<>(cnf.numVars()));

    if (!outfile.empty()) {
        mh << "\nOUTPUT: " << outfile;
        mh.begin("writing") << " ...";

        if (outfile == "-") {
            output(std::cout, dd, cnf);
        }
        else {
            std::ofstream ofs(outfile.c_str());
            if (!ofs) throw std::runtime_error(strerror(errno));
            output(ofs, dd, cnf);
        }

        mh.end();
    }
}

int main(int argc, char *argv[]) {
    for (unsigned i = 0; i < sizeof(options) / sizeof(options[0]); ++i) {
        opt[options[i][0]] = false;
    }

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
            usage(argv[0]);
            return 1;
        }
    }

    if (infile.empty()) infile = "-";

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
