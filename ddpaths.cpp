/*
 * Top-Down ZDD Construction Library for Frontier-Based Search
 * by Hiroaki Iwashita <iwashita@erato.ist.hokudai.ac.jp>
 * Copyright (c) 2012 Japan Science and Technology Agency
 * $Id: ddpaths.cpp 518 2014-03-18 05:29:23Z iwashita $
 */

#include <cstdlib>
#include <iomanip>
#include <iostream>
#include <map>
#include <stdexcept>
#include <string>

#include "dd/DdStructure.hpp"
#include "eval/Cardinality.hpp"
//#include "eval/DdExporter.hpp"
#include "spec/DdSpecExamples.hpp"
#include "spec/DegreeConstraint.hpp"
#include "spec/FrontierBasedSearch.hpp"
#include "spec/PathZdd.hpp"
#include "spec/SizeConstraint.hpp"
//#include "util/CuddZdd.hpp"
#include "util/Graph.hpp"

using namespace tdzdd;

//#if __SIZEOF_POINTER__ == 8
//#define B_64
//#endif
//#include <ZBDD.h>

std::string options[][2] = { //
        {"t path", "Enumerate paths (default)"}, //
        {"t cycle", "Enumerate cycles"}, //
        {"t cc", "Enumerate connected components"}, //
        {"t forest", "Enumerate forests"}, //
        {"hamilton", "Enumerate Hamiltonian paths/cycles"}, //
        //{"color", "Enumerate coloring patterns"}, //
        {"slow", "Use slower algorithm (only for paths/cycles)"}, //
        {"nola", "Do not use lookahead (only for paths/cycles)"}, //
        {"p", "Use parallel algorithms (implies -pr)"}, //
        {"sr", "Use sequential reduction (Algorithm R)"}, //
        {"pr", "Use parallel reduction"}, //
        {"dc", "Use degree constraint filter"}, //
        {"nored", "Do not execute final reduction"}, //
        {"ub <n>", "Upper bound of the number of items"}, //
        {"lb <n>", "Lower bound of the number of items"}, //
        {"uec <n>", "Number of the uncolored edge components"}, //
        {"count", "Report the number of solutions"}, //
        //{"reorder", "Reorder result ZDD"}, //
        //{"pathreport", "Report the number and length of paths"}, //
        //{"edgecount", "Report the number of live graph/tree edges"}, //
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
            << " [ <option>... ] [ <graph_file> [ <terminal_pair_file> ]]\n";
    std::cerr << "options\n";
    for (unsigned i = 0; i < sizeof(options) / sizeof(options[0]); ++i) {
        std::cerr << "  -" << options[i][0];
        for (unsigned j = options[i][0].length(); j < 10; ++j) {
            std::cerr << " ";
        }
        std::cerr << ": " << options[i][1] << "\n";
    }
}

struct EdgeDecorator {
    std::vector<bool> selected;

    std::string operator()(Graph::EdgeNumber a) const {
        return selected[a] ? "[style=bold]" : "[style=dotted,color=gray]";
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
            if (s[0] == '-') {
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
    }
    catch (std::exception& e) {
        usage(argv[0]);
        return 1;
    }

    if (opt["p"]) opt["pr"] = true;
    if (opt["sr"]) opt["pr"] = false;

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

        if (graph.edgeSize() == 0) throw std::runtime_error(
                "ERROR: The graph is empty!!!");
//        if (optStr["t"] != "cycle" && graph.numColor() == 0) throw std::runtime_error(
//                "ERROR: No vertex is colored!!!");
        if (optStr["t"] == "path" && !graph.hasColorPairs()) throw std::runtime_error(
                "ERROR: Colored vertices are not paired!!!");

        if (opt["graph"]) {
            graph.dump(std::cout);
            return 0;
        }

        int const n = graph.edgeSize();
        DdStructure f(n);

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
            f = DdStructure(dc, opt["p"]);
            f.zddReduce(opt["pr"]);
        }

        if (opt["lb"] || opt["ub"]) {
            IntRange r(optNum["lb"], optNum["ub"]);
            SizeConstraint sc(graph.edgeSize(), &r);
            f.zddSubset(sc, opt["p"]);
            f.zddReduce(opt["pr"]);
        }

        if (opt["zdd1"]) f.dumpDot(std::cout, "Intermediate ZDD");

        if (optStr["t"] == "path") {
            if (opt["dc"]) {
                if (opt["hamilton"]) {
                    HamiltonPathZdd hp(graph, !opt["nola"]);
                    f.zddSubset(hp, opt["p"]);
                }
                else if (opt["slow"]) {
                    SimpathZddByMap p(graph);
                    f.zddSubset(p, opt["p"]);
                }
                else {
                    PathZdd p(graph, !opt["nola"]);
                    f.zddSubset(p, opt["p"]);
                }
            }
            else {
                DdStructure g = f;

                if (opt["hamilton"]) {
                    HamiltonPathZdd hp(graph, !opt["nola"]);
                    f = DdStructure(hp, opt["p"]);
                }
                else if (opt["slow"]) {
                    SimpathZddByMap p(graph);
                    f = DdStructure(p, opt["p"]);
                }
                else {
                    PathZdd p(graph, !opt["nola"]);
                    f = DdStructure(p, opt["p"]);
                }

                if (opt["lb"] || opt["ub"]) {
                    if (!opt["nored"]) f.zddReduce(opt["pr"]);
                    f.zddSubset(g, opt["p"]);
                }
            }

            if (!opt["nored"]) f.zddReduce(opt["pr"]);
        }
        else if (optStr["t"] == "cycle") {
            if (opt["dc"]) {
                if (opt["hamilton"]) {
                    HamiltonCycleZdd hc(graph, !opt["nola"]);
                    f.zddSubset(hc, opt["p"]);
                }
                else if (opt["slow"]) {
                    SimpathZddByMap c(graph);
                    f.zddSubset(c, opt["p"]);
                }
                else {
                    CycleZdd c(graph, !opt["nola"]);
                    f.zddSubset(c, opt["p"]);
                }
            }
            else {
                DdStructure g = f;

                if (opt["hamilton"]) {
                    HamiltonCycleZdd hc(graph, !opt["nola"]);
                    f = DdStructure(hc, opt["p"]);
                }
                else if (opt["slow"]) {
                    SimpathZddByMap c(graph);
                    f = DdStructure(c, opt["p"]);
                }
                else {
                    CycleZdd c(graph, !opt["nola"]);
                    f = DdStructure(c, opt["p"]);
                }

                if (opt["lb"] || opt["ub"]) {
                    if (!opt["nored"]) f.zddReduce(opt["pr"]);
                    f.zddSubset(g, opt["p"]);
                }
            }

            if (!opt["nored"]) f.zddReduce(opt["pr"]);
        }
        else if (optStr["t"] == "cc" || optStr["t"] == "forest") {
            FrontierBasedSearch fbs(graph, optNum["uec"],
                    optStr["t"] == "forest");
            if (opt["lb"] || opt["ub"]) {
//                f.zddSubset(fbs, opt["p"]);
                DdStructure g = f;
                f = DdStructure(fbs, opt["p"]);
                if (!opt["nored"]) f.zddReduce(opt["pr"]);
                f.zddSubset(g, opt["p"]);
            }
            else {
                f = DdStructure(fbs, opt["p"]);
            }

            if (!opt["nored"]) f.zddReduce(opt["pr"]);
        }
        else {
            throw std::runtime_error(optStr["t"] + ": Unknown type (-t)");
        }

        m1.end();
//        if (opt["reorder"]) {
//            m1.begin("exporting to CUDD") << " ...";
//            CuddZdd::init(n);
//            CuddZdd g = f.evaluate(DdExporter<CuddZdd>());
//            m1.end(g.size());
//            m1.begin("reordering") << " ...";
//            CuddZdd::reorder();
//            m1.end(g.size());
//            for (int i = n; i > 0; --i) {
//                std::cout << graph.edgeLabel(CuddZdd::level2index(i)) << "\n";
//            }
//            m1.begin("importing from CUDD") << " ...";
//            //TODO
//            m1.end();
//        }

        if (opt["zdd"]) f.dumpDot(std::cout, "Result ZDD");
        if (opt["export"]) f.dumpSapporo(std::cout);

        m0 << "\n#node = " << f.size() << ", #solution = "
                << std::setprecision(10)
                << f.evaluate(Cardinality<double>(), opt["p"]) << "\n";

//        if (opt["pathreport"]) {
//            MessageHandler mh;
//            mh.begin("counting paths") << " ...";
//
//            auto pair = f.pathReport();
//            mh << "\n#solution = " << pair.first;
//            mh << "\ntotal length = " << pair.second;
//            if (pair.second != 0) {
//                mh << "\nmean length = " << std::setprecision(10)
//                        << (pair.second.get_d() / pair.first.get_d());
//            }
//
//            mh.end();
//        }
        if (opt["count"]) {
            MessageHandler mh;
            mh.begin("counting solutions") << " ...";
            mh << "\n#solution = " << f.evaluate(Cardinality<>(), opt["p"]);
            mh.end();
        }

//        if (opt["edgecount"]) {
//            MessageHandler mh;
//            mh.begin("counting edges") << " ...";
//
//            mh << "\n#LUTE = " << f.liveUnreducedTreeEdgeCount();
//            mh << "\n#LRTE = " << f.liveTreeEdgeCount();
//            mh << "\n#LUDE = " << f.liveUnreducedEdgeCount();
//            mh << "\n#LRDE = " << f.liveEdgeCount();
//
//            mh.end();
//        }

        if (opt["all"]) {
            int const n = graph.edgeSize();

            for (DdStructure::const_iterator t = f.begin(); t != f.end(); ++t) {
                EdgeDecorator edges;

                edges.selected.resize(n);
                for (size_t i = 0; i < t->size(); ++i) {
                    edges.selected[n - (*t)[i]] = true;
                }

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
