/*
 * Top-Down ZDD Construction Library for Frontier-Based Search
 * by Hiroaki Iwashita <iwashita@erato.ist.hokudai.ac.jp>
 * Copyright (c) 2013 Japan Science and Technology Agency
 * $Id: ddcnf.cpp 518 2014-03-18 05:29:23Z iwashita $
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
#include "eval/Density.hpp"
#include "op/BinaryOperation.hpp"
#include "op/TddHitting.hpp"
#include "spec/ClauseBdd.hpp"
#include "spec/ClauseZdd.hpp"
#include "util/MessageHandler.hpp"
#include "util/MySet.hpp"

using namespace tdzdd;

std::string options[][2] = { //
        {"apply", "Use the basic algorithm based on BDD-apply operations"}, //
        {"zddapply", "Use the basic algorithm based on ZDD-apply operations"}, //
        {"c <n>", "Compute rich reachability information limited by size <n>"}, //
        {"p", "Use parallel algorithms"}, //
        {"cnf", "Dump the input CNF to STDOUT in DIMACS format"}, //
        {"tdd", "Dump the input CNF-TDD to STDOUT in DOT format"}, //
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

void output(std::ostream& os, DdStructure const& f, CnfTdd const& cnf) {
    for (typeof(f.begin()) t = f.begin(); t != f.end(); ++t) {
        for (size_t i = 0; i < t->size(); ++i) {
            if (i != 0) std::cout << " ";
            cnf.printLevel(os, (*t)[i]);
        }
        os << "\n";
    }
}

void run() {
    MessageHandler mh;
    CnfTdd cnf;

    mh << "\nINPUT: " << infile;
    if (infile == "-") {
        cnf.load(std::cin);
    }
    else {
        std::ifstream ifs(infile.c_str());
        if (!ifs) throw std::runtime_error(strerror(errno));
        cnf.load(ifs);
    }

    if (opt["cnf"]) cnf.dumpDimacs(std::cout);
    if (opt["tdd"]) cnf.dumpDotCut(std::cout, "tdd");

    DdStructure dd;

    if (opt["apply"]) {
        MessageHandler mh;
        mh.begin("BDD-apply");
        mh.setSteps(cnf.numClauses());
        dd = DdStructure(0);
        for (typeof(cnf.begin()) c = cnf.begin(); c != cnf.end(); ++c) {
            mh.step();
            bool flag = MessageHandler::showMessages(false);
            dd = DdStructure(bddAnd(dd, ClauseBdd(*c)));
            dd.bddReduce(opt["p"]);
            MessageHandler::showMessages(flag);
        }
        mh.end(dd.size());
        if (opt["dd2"]) dd.dumpDot(std::cout, "dd2");
        mh << "\n#solution = "
                << size_t(
                        dd.evaluate(Density()) * std::pow(2.0, cnf.numVars()));
        return;
    }

    if (opt["zddapply"]) {
        MessageHandler mh;
        mh.begin("ZDD-apply");
        mh.setSteps(cnf.numClauses());
        dd = DdStructure(cnf.numVars());
        for (typeof(cnf.begin()) c = cnf.begin(); c != cnf.end(); ++c) {
            mh.step();
            bool flag = MessageHandler::showMessages(false);
            dd.zddSubset(ClauseZdd(cnf.numVars(), *c), opt["p"]);
            dd.zddReduce(opt["p"]);
            MessageHandler::showMessages(flag);
        }
        mh.end(dd.size());
    }
    else if (opt["c"]) {
        cnf.compile(optNum["c"]);
        TddHitting<false,true> spec(cnf);
        if (opt["dd0"]) spec.dumpDot(std::cout, "dd0");
        dd = DdStructure(spec, opt["p"]);
        if (opt["dd1"]) dd.dumpDot(std::cout, "dd1");
        dd.zddReduce(opt["p"]);
    }
    else {
        TddHitting<true,false> spec(cnf);
        if (opt["dd0"]) spec.dumpDot(std::cout, "dd0");
        dd = DdStructure(spec, opt["p"]);
        if (opt["dd1"]) dd.dumpDot(std::cout, "dd1");
        dd.zddReduce(opt["p"]);
    }

    if (opt["dd2"]) dd.dumpDot(std::cout, "dd2");
    mh << "\n#solution = " << dd.evaluate(Cardinality<>());

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
