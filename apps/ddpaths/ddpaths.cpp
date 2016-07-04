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

#include <cstdlib>
#include <iomanip>
#include <iostream>
#include <map>
#include <stdexcept>
#include <string>

#include <tdzdd/DdStructure.hpp>

#include "PathZdd.hpp"
#include "PathZddByStdMap.hpp"
#include "../graphillion/DegreeConstraint.hpp"
#include "../graphillion/FrontierBasedSearch.hpp"
#include "../graphillion/Graph.hpp"
#include "../graphillion/SizeConstraint.hpp"

using namespace tdzdd;

std::string options[][2] = { //
        {"t path", "Enumerate paths (default)"}, //
        {"t cycle", "Enumerate cycles"}, //
        {"t cc", "Enumerate connected components"}, //
        {"t forest", "Enumerate forests"}, //
        {"hamilton", "Enumerate Hamiltonian paths/cycles"}, //
        {"slow", "Use slower algorithm (only for paths/cycles)"}, //
        {"nola", "Do not use lookahead (only for paths/cycles)"}, //
        {"p", "Use parallel algorithms"}, //
        {"dc", "Use degree constraint filter"}, //
        {"nored", "Do not execute final reduction"}, //
        {"ub <n>", "Upper bound of the number of items"}, //
        {"lb <n>", "Lower bound of the number of items"}, //
        {"uec <n>", "Number of the uncolored edge components"}, //
        {"count", "Report the number of solutions"}, //
        {"graph", "Dump input graph to STDOUT in DOT format"}, //
        {"all", "Dump all solutions to STDOUT in DOT format"}, //
        {"zdd", "Dump result ZDD to STDOUT in DOT format"}, //
        {"zdd1", "Dump intermediate ZDD to STDOUT in DOT format"}, //
        {"export", "Dump result ZDD to STDOUT in Sapporo BDD format"}}; //

std::map<std::string,bool> opt;
std::map<std::string,int> optNum;
std::map<std::string,std::string> optStr;
std::string graphFileName;
std::string termFileName;

void usage(char const* cmd) {
    std::cerr << "usage: " << cmd
              << " [ <option>... ] <graph_file> [ <terminal_pair_file> ]\n";
    std::cerr << "options\n";
    for (unsigned i = 0; i < sizeof(options) / sizeof(options[0]); ++i) {
        std::cerr << "  -" << options[i][0];
        for (unsigned j = options[i][0].length(); j < 10; ++j) {
            std::cerr << " ";
        }
        std::cerr << ": " << options[i][1] << "\n";
    }
}

class EdgeDecorator {
    int const n;
    std::set<int> const& levels;

public:
    EdgeDecorator(int n, std::set<int> const& levels) :
            n(n), levels(levels) {
    }

    std::string operator()(Graph::EdgeNumber a) const {
        return levels.count(n - a) ?
                "[style=bold]" : "[style=dotted,color=gray]";
    }
};

//void exportZdd(ZDD const& dd) {
//    int n = dd.getLevel(dd.root());
//    while (n > BDD_VarUsed()) {
//        BDD_NewVar();
//    }
//
//    dd.convert<ZBDD>().Export();
//}

int main(int argc, char *argv[]) {
    optStr["t"] = "path";
    optNum["uec"] = -1;
    optNum["lb"] = 0;
    optNum["ub"] = INT_MAX;

    for (unsigned i = 0; i < sizeof(options) / sizeof(options[0]); ++i) {
        opt[options[i][0]] = false;
    }

    try {
        for (int i = 1; i < argc; ++i) {
            std::string s = argv[i];
            if (s.size() >= 2 && s[0] == '-') {
                s = s.substr(1);

                if (opt.count(s)) {
                    opt[s] = true;
                }
                else if (i + 1 < argc && opt.count(s + " <n>")) {
                    opt[s] = true;
                    optNum[s] = std::atoi(argv[++i]);
                }
                else if (i + 1 < argc && opt.count(s + " " + argv[i + 1])) {
                    opt[s] = true;
                    optStr[s] = argv[++i];
                }
                else {
                    throw std::exception();
                }
            }
            else if (graphFileName.empty()) {
                graphFileName = s;
            }
            else if (termFileName.empty()) {
                termFileName = s;
            }
            else {
                throw std::exception();
            }
        }

        if (graphFileName.empty()) throw std::exception();
        if (graphFileName == "-") graphFileName = "";
    }
    catch (std::exception& e) {
        usage(argv[0]);
        return 1;
    }

    MessageHandler::showMessages();
    MessageHandler m0;
    m0.begin("started");

    try {
        Graph graph;
        graph.readAdjacencyList(graphFileName);
        if (!termFileName.empty()) graph.readVertexGroups(termFileName);

        if (optStr["t"] == "path" && graph.numColor() == 0) {
            graph.setDefaultPathColor();
        }

        m0 << "\n#vertex = " << graph.vertexSize() << ", #edge = "
           << graph.edgeSize() << ", max_frontier_size = "
           << graph.maxFrontierSize();
        if (optStr["t"] == "cycle") {
            graph.clearColors();
        }
        else {
            m0 << ", #color = " << graph.numColor();
        }
        m0 << "\n";

        if (graph.edgeSize() == 0)
            throw std::runtime_error("ERROR: The graph is empty!!!");
//        if (optStr["t"] != "cycle" && graph.numColor() == 0) throw std::runtime_error(
//                "ERROR: No vertex is colored!!!");
        if (optStr["t"] == "path" && !graph.hasColorPairs())
            throw std::runtime_error(
                    "ERROR: Colored vertices are not paired!!!");

        if (opt["graph"]) {
            graph.dump(std::cout);
            return 0;
        }

        int const n = graph.edgeSize();
        DdStructure<2> f(n);

        MessageHandler m1;
        m1.begin("building") << " ...";

        if (opt["dc"] && (optStr["t"] == "path" || optStr["t"] == "cycle")) {
            IntRange zeroOrTwo(0, 2, 2);
            IntRange justOne(1, 1);
            DegreeConstraint dc(graph);

            for (int v = 1; v <= graph.vertexSize(); ++v) {
                if (graph.colorNumber(v) == 0) {
                    dc.setConstraint(v, &zeroOrTwo);
                }
                else {
                    dc.setConstraint(v, &justOne);
                }
            }

            //f.zddSubset(dc, opt["p"]);
            f = DdStructure<2>(dc, opt["p"]);
            f.zddReduce();
        }

        if (opt["lb"] || opt["ub"]) {
            IntRange r(optNum["lb"], optNum["ub"]);
            SizeConstraint sc(graph.edgeSize(), &r);
            f.zddSubset(sc);
            f.zddReduce();
        }

        if (opt["zdd1"]) f.dumpDot(std::cout, "Intermediate ZDD");

        if (optStr["t"] == "path") {
            if (opt["dc"]) {
                if (opt["hamilton"]) {
                    HamiltonPathZdd hp(graph, !opt["nola"]);
                    f.zddSubset(hp);
                }
                else if (opt["slow"]) {
                    PathZddByStdMap p(graph);
                    f.zddSubset(p);
                }
                else {
                    PathZdd p(graph, !opt["nola"]);
                    f.zddSubset(p);
                }
            }
            else {
                DdStructure<2> g = f;

                if (opt["hamilton"]) {
                    HamiltonPathZdd hp(graph, !opt["nola"]);
                    f = DdStructure<2>(hp, opt["p"]);
                }
                else if (opt["slow"]) {
                    PathZddByStdMap p(graph);
                    f = DdStructure<2>(p, opt["p"]);
                }
                else {
                    PathZdd p(graph, !opt["nola"]);
                    f = DdStructure<2>(p, opt["p"]);
                }

                if (opt["lb"] || opt["ub"]) {
                    if (!opt["nored"]) f.zddReduce();
                    f.zddSubset(g);
                }
            }

            if (!opt["nored"]) f.zddReduce();
        }
        else if (optStr["t"] == "cycle") {
            if (opt["dc"]) {
                if (opt["hamilton"]) {
                    HamiltonCycleZdd hc(graph, !opt["nola"]);
                    f.zddSubset(hc);
                }
                else if (opt["slow"]) {
                    PathZddByStdMap c(graph);
                    f.zddSubset(c);
                }
                else {
                    CycleZdd c(graph, !opt["nola"]);
                    f.zddSubset(c);
                }
            }
            else {
                DdStructure<2> g = f;

                if (opt["hamilton"]) {
                    HamiltonCycleZdd hc(graph, !opt["nola"]);
                    f = DdStructure<2>(hc, opt["p"]);
                }
                else if (opt["slow"]) {
                    PathZddByStdMap c(graph);
                    f = DdStructure<2>(c, opt["p"]);
                }
                else {
                    CycleZdd c(graph, !opt["nola"]);
                    f = DdStructure<2>(c, opt["p"]);
                }

                if (opt["lb"] || opt["ub"]) {
                    if (!opt["nored"]) f.zddReduce();
                    f.zddSubset(g);
                }
            }

            if (!opt["nored"]) f.zddReduce();
        }
        else if (optStr["t"] == "cc" || optStr["t"] == "forest") {
            FrontierBasedSearch fbs(graph, optNum["uec"],
                    optStr["t"] == "forest");
            if (opt["lb"] || opt["ub"]) {
//                f.zddSubset(fbs, opt["p"]);
                DdStructure<2> g = f;
                f = DdStructure<2>(fbs, opt["p"]);
                if (!opt["nored"]) f.zddReduce();
                f.zddSubset(g);
            }
            else {
                f = DdStructure<2>(fbs);
            }

            if (!opt["nored"]) f.zddReduce();
        }
        else {
            throw std::runtime_error(optStr["t"] + ": Unknown type (-t)");
        }

        m1.end();

        if (opt["zdd"]) f.dumpDot(std::cout, "Result ZDD");
        if (opt["export"]) f.dumpSapporo(std::cout);

        m0 << "\n#node = " << f.size() << ", #solution = "
                << std::setprecision(10) << f.evaluate(ZddCardinality<double>())
                << "\n";

        if (opt["count"]) {
            m1.begin("counting solutions") << " ...";
            m1 << "\n#solution = " << f.evaluate(ZddCardinality<>());
            m1.end();
        }

        if (opt["all"]) {
            for (DdStructure<2>::const_iterator t = f.begin(); t != f.end();
                    ++t) {
                EdgeDecorator edges(n, *t);
                graph.dump(std::cout, edges);
            }
        }
    }
    catch (std::exception& e) {
        m0 << e.what() << "\n";
        return 1;
    }

    m0.end("finished");
    return 0;
}
