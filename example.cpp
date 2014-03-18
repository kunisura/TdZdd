#include <iostream>
#include <string>

#include "dd/PathCounter.hpp"
#include "dd/DdStructure.hpp"
#include "eval/Cardinality.hpp"
#include "eval/MinNumItems.hpp"
#include "eval/MaxNumItems.hpp"
#include "eval/ToZBDD.hpp"
#include "spec/DdSpecExamples.hpp"
#include "spec/PathZdd.hpp"
#include "spec/SapporoZdd.hpp"
#include "util/Graph.hpp"

using namespace tdzdd;

int main(int argc, char *argv[]) {
    std::string graphFileName;
    std::string termFileName;

    for (int i = 1; i < argc; ++i) {
        if (graphFileName.empty()) {
            graphFileName = argv[i];
        }
        else if (termFileName.empty()) {
            termFileName = argv[i];
        }
        else {
            std::cerr << "usage: " << argv[0]
                    << " [ <graph_file> [ <terminal_pair_file> ]]\n";
            return 1;
        }
    }

    MessageHandler::showMessages();

    Graph g;
    try {
        g.readAdjacencyList(graphFileName);
        if (termFileName.empty()) {
            g.setDefaultPathColor();
        }
        else {
            g.readVertexGroups(termFileName);
        }
        std::cerr << "#vertex = " << g.vertexSize() << ", #edge = "
                << g.edgeSize() << "\n";
    }
    catch (std::exception& e) {
        std::cerr << e.what() << "\n";
        return 1;
    }

    BDD_Init(1024, 1024 * 1024 * 1024);

    ZBDD f0;
    {
        PathZdd p(g);
        DdStructure dd(p);
        std::cerr << "#node = " << dd.size() << ", #path = "
                << dd.evaluate(Cardinality<>()) << "\n";
        f0 = dd.evaluate(ToZBDD());
    }

    ZBDD f1;
    {
        SimpathZddByMap p(g);
        DdStructure dd(p);
        std::cerr << "#node = " << dd.size() << ", #path = "
                << dd.evaluate(Cardinality<>()) << "\n";
        f1 = dd.evaluate(ToZBDD());
    }
    std::cerr << (f1 == f0 ? "f1 == f0" : "f1 != f0") << "\n";

    {
        SapporoZdd f(f0);
        DdStructure dd(f);
        int min = dd.evaluate(MinNumItems());
        int max = dd.evaluate(MaxNumItems());
        std::cerr << "#node = " << dd.size() << ", #path = "
                << dd.evaluate(Cardinality<>()) << ", length = [" << min << ","
                << max << "]\n";
    }

    return 0;
}
