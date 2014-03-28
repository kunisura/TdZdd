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

#include <climits>
#include <iostream>
#include <map>
#include <string>

#include <tdzdd/DdStructure.hpp>

#include "DegreeConstraint.hpp"
#include "FrontierBasedSearch.hpp"
#include "Graph.hpp"
#include "GraphillionZdd.hpp"
#include "SapporoZdd.hpp"
#include "SizeConstraint.hpp"
#include "ToZBDD.hpp"

using namespace tdzdd;

std::string options[][2] = { //
        {"path", "Restrict to paths and cycles"}, //
        {"matching", "Restrict to matchings"}, //
        {"spanning", "Restrict to make no isolated vertices"}, //
        {"noloop", "Restrict to forests"}, //
        {"uec <n>", "Number of the uncolored edge components"}, //
        {"lb <n>", "Lower bound of the number of edges"}, //
        {"ub <n>", "Upper bound of the number of edges"}, //
        {"st", "Color the first vertex and the last vertex"}, //
        {"a", "Read <graph_file> as an adjacency list"}, //
        {"graph", "Dump input graph to STDOUT in DOT format"}, //
        {"solutions <n>", "Dump at most <n> solutions to STDOUT in DOT format"}, //
        {"zdd", "Dump result ZDD to STDOUT in DOT format"}, //
        {"sapporo", "Translate to Sapporo ZBDD"}, //
        {"import", "Read constraint ZDD from STDIN"}, //
        {"export", "Dump result ZDD to STDOUT"}}; //

std::map<std::string,bool> opt;
std::map<std::string,int> optNum;
std::map<std::string,std::string> optStr;

void usage(char const* cmd) {
    std::cerr << "usage: " << cmd
            << " [ <option>... ] [ <graph_file> [ <vertex_group_file> ]]\n";
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

int main(int argc, char *argv[]) {
    optNum["uec"] = -1;
    optNum["lb"] = 0;
    optNum["ub"] = INT_MAX;

    for (unsigned i = 0; i < sizeof(options) / sizeof(options[0]); ++i) {
        opt[options[i][0]] = false;
    }

    std::string graphFileName;
    std::string termFileName;

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

    MessageHandler::showMessages();
    MessageHandler mh;
    mh.begin("started");

    GraphillionZdd base;
    if (opt["import"]) {
        base.read();
    }

    Graph g;
    try {
        if (!graphFileName.empty()) {
            if (opt["a"]) {
                g.readAdjacencyList(graphFileName);
            }
            else {
                g.readEdges(graphFileName);
            }
        }
        else {
            g.addEdge("v1", "v2");
            g.addEdge("v1", "v3");
            g.addEdge("v1", "v4");
            g.addEdge("v2", "v4");
            g.addEdge("v2", "v5");
            g.addEdge("v3", "v4");
            g.addEdge("v3", "v6");
            g.addEdge("v4", "v5");
            g.addEdge("v4", "v6");
            g.addEdge("v4", "v7");
            g.addEdge("v5", "v7");
            g.addEdge("v6", "v7");
            g.setColor("v2", 0);
            g.setColor("v3", 0);
            g.update();
        }

        if (!termFileName.empty()) {
            g.readVertexGroups(termFileName);
        }

        int const m = g.vertexSize();
        int const n = g.edgeSize();

        if (opt["st"] && m >= 1) {
            g.setColor(g.vertexName(1), 0);
            g.setColor(g.vertexName(m), 0);
            g.update();
        }

        mh << "#vertex = " << m << ", #edge = " << n << ", #color = "
                << g.numColor() << "\n";

        if (g.edgeSize() == 0) throw std::runtime_error(
                "ERROR: The graph is empty!!!");

        if (opt["graph"]) {
            g.dump(std::cout);
            return 0;
        }

        DdStructure<2> dd(n);

        if (opt["import"]) {
            dd = DdStructure<2>(base);
//            dd.dumpDot(std::cout, g.edgeLabeler());
            mh << "#node = " << dd.size() << ", #solution = "
                    << dd.evaluate(ZddCardinality<>()) << "\n\n";
        }

        if (opt["path"]) {
            if (!opt["uec"]) optNum["uec"] = 0;

            IntRange zeroOrTwo(0, 2, 2);
            IntRange justOne(1, 1);
            DegreeConstraint dc(g);

            for (int v = 1; v <= g.vertexSize(); ++v) {
                if (g.colorNumber(v) == 0) {
                    dc.setConstraint(v, &zeroOrTwo);
                }
                else {
                    dc.setConstraint(v, &justOne);
                }
            }
            dd.zddSubset(dc);

            mh << "#node = " << dd.size() << ", #solution = "
                    << dd.evaluate(ZddCardinality<>()) << "\n\n";
        }

        if (opt["matching"]) {
            IntRange zeroOrOne(0, 1);
            DegreeConstraint dc(g, &zeroOrOne);
            dd.zddSubset(dc);

            mh << "#node = " << dd.size() << ", #solution = "
                    << dd.evaluate(ZddCardinality<>()) << "\n\n";
        }

        if (opt["spanning"]) {
            IntRange oneOrMore(1);
            DegreeConstraint dc(g);

            for (int v = 1; v <= g.vertexSize(); ++v) {
                if (g.colorNumber(v) == 0) {
                    dc.setConstraint(v, &oneOrMore);
                }
            }
            dd.zddSubset(dc);

            mh << "#node = " << dd.size() << ", #solution = "
                    << dd.evaluate(ZddCardinality<>()) << "\n\n";
        }

        if (opt["lb"] || opt["ub"]) {
            IntRange r(optNum["lb"], optNum["ub"]);
            SizeConstraint sc(g.edgeSize(), &r);
            dd.zddSubset(sc);

            mh << "#node = " << dd.size() << ", #solution = "
                    << dd.evaluate(ZddCardinality<>()) << "\n\n";
        }

        FrontierBasedSearch fbs(g, optNum["uec"], opt["noloop"]);
        dd.zddSubset(fbs);

        if (opt["zdd"]) dd.dumpDot(std::cout, "ZDD");
        if (opt["export"]) dd.dumpSapporo(std::cout);

        mh << "#node = " << dd.size() << ", #solution = "
                << dd.evaluate(ZddCardinality<>()) << "\n\n";

        if (opt["sapporo"]) {
            BDD_Init(1024, 1024 * 1024 * 1024);
            MessageHandler mh;
            mh.begin("ToZBDD") << " ...";
            ZBDD f = dd.evaluate(ToZBDD());
            mh.end(f.Size());
        }

        if (opt["solutions"]) {
            int const n = g.edgeSize();
            int count = optNum["solutions"];

            for (typeof(dd.begin()) t = dd.begin(); t != dd.end(); ++t) {
                EdgeDecorator edges;

                edges.selected.resize(n);
                for (size_t i = 0; i < t->size(); ++i) {
                    edges.selected[n - (*t)[i]] = true;
                }

                g.dump(std::cout, edges);
                if (--count == 0) break;
            }
        }
    }
    catch (std::exception& e) {
        std::cerr << e.what() << "\n";
        return 1;
    }

    mh.end();
    return 0;
}
