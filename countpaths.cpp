/*
 * Top-Down ZDD Construction Library for Frontier-Based Search
 * by Hiroaki Iwashita <iwashita@erato.ist.hokudai.ac.jp>
 * Copyright (c) 2012 Japan Science and Technology Agency
 * $Id: countpaths.cpp 516 2014-01-29 01:21:35Z iwashita $
 */

#include <iomanip>
#include <iostream>
#include <map>
#include <set>
#include <string>

#include "dd/PathCounter.hpp"
#include "spec/PathZdd.hpp"
#include "util/Graph.hpp"
#include "util/MessageHandler.hpp"

using namespace tdzdd;

std::string options[][2] = { //
        {"cycles", "Enumerate cycles instead of paths"}, //
        {"hamilton", "Enumerate Hamiltonian paths/cycles"}, //
        {"fast", "Count using more memory and less CPU time"}}; //

std::map<std::string,bool> opt;

void usage(char const* cmd) {
    std::cerr << "usage: " << cmd
            << " <option>... <graph_file> [<terminal_file>]\n";
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
    std::string graphFileName;
    std::string termFileName;

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
        else if (graphFileName.empty()) {
            graphFileName = s;
        }
        else if (termFileName.empty()) {
            termFileName = s;
        }
        else {
            usage(argv[0]);
            return 1;
        }
    }

    MessageHandler::showMessages();
    MessageHandler mh;
    mh.begin("started");

    try {
        Graph g;
        g.readAdjacencyList(graphFileName);
        if (termFileName.empty()) {
            g.setDefaultPathColor();
        }
        else {
            g.readVertexGroups(termFileName);
        }

        mh << "\n#vertex = " << g.vertexSize() << ", #edge = " << g.edgeSize()
                << ", max_frontier_size = " << g.maxFrontierSize() << "\n";
        if (g.edgeSize() == 0) {
            mh << "ERROR: The graph is empty!!!\n";
            return 1;
        }

        if (opt["cycles"]) {
            g.clearColors();
            if (opt["hamilton"]) {
                HamiltonCycleZdd dd(g);
                mh << "\n#path = " << countPaths(dd, opt["fast"]);
            }
            else {
                CycleZdd dd(g);
                mh << "\n#path = " << countPaths(dd, opt["fast"]);
            }
        }
        else {
            if (opt["hamilton"]) {
                HamiltonPathZdd dd(g);
                mh << "\n#path = " << countPaths(dd, opt["fast"]);
            }
            else {
                PathZdd dd(g);
                mh << "\n#path = " << countPaths(dd, opt["fast"]);
            }
        }
    }
    catch (std::exception& e) {
        mh << e.what() << "\n";
        return 1;
    }

    mh.end("finished");
    return 0;
}
