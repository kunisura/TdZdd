#include <cstdlib>
#include <iostream>
#include <map>
#include <stdexcept>

#include "dd/DdStructure.hpp"
#include "dd/PathCounter.hpp"
#include "eval/Cardinality.hpp"
#include "spec/NAryZdd.hpp"
#include "spec/PsnZdd.hpp"
#include "util/MessageHandler.hpp"

using namespace tdzdd;

std::string options[][2] = { //
        {"grid", "Arrange multiple ladders in a row"}, //
        {"merge", "Merge the states that are equivalent in terms of counting"}, //
        {"dump", "Dump a state transition diagram to STDOUT in DOT format"}, //
        {"zdd", "Dump ZDDs before/after reduction to STDOUT in DOT format"}, //
        {"count", "Count the number of patterns without building a ZDD"}}; //

std::map<std::string,bool> opt;

void usage(char const* cmd) {
    std::cerr << "usage: " << cmd << " [ <option>... ] <size>\n";
    std::cerr << "options\n";
    for (unsigned i = 0; i < sizeof(options) / sizeof(options[0]); ++i) {
        std::cerr << "  -" << options[i][0];
        for (unsigned j = options[i][0].length(); j < 10; ++j) {
            std::cerr << " ";
        }
        std::cerr << ": " << options[i][1] << "\n";
    }
}

int main(int argc, char *argv[]) {
    int n = 0;

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
            try {
                n = std::atoi(argv[i]);
            }
            catch (std::exception& e) {
                std::cerr << e.what() << "\n";
            }

            if (n <= 0) break;
        }
        else {
            n = -1;
            break;
        }
    }

    if (n <= 0) {
        usage(argv[0]);
        return 1;
    }

    MessageHandler::showMessages();
    MessageHandler mh;
    mh.begin("started");
    mh << "\nn = " << n << ", #row = " << (n * (n - 1) / 2) << ", #var = "
            << (n * (n - 1) * (n - 1) / 2);

    PsnZdd spec(n, opt["grid"], opt["merge"]);

    if (opt["dump"]) {
        //dumpDot(spec);
        std::cout << spec;
    }

    if (opt["zdd"]) {
        DdStructure f(spec, false);
        f.dumpDot(std::cout);
        f.zddReduce();
        f.dumpDot(std::cout);
        mh << "\n#solution = " << f.evaluate(Cardinality<>());
    }
    else if (opt["count"]) {
        mh << "\n#solution = " << countPaths64(spec);
    }
    else {
        DdStructure f(spec);
        mh << "\n#solution = " << f.evaluate(Cardinality<>());
    }

    mh.end("finished");
    return 0;
}
