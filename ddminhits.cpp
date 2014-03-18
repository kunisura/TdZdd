/*
 * Top-Down ZDD Construction Library for Frontier-Based Search
 * by Hiroaki Iwashita <iwashita@erato.ist.hokudai.ac.jp>
 * Copyright (c) 2013 Japan Science and Technology Agency
 * $Id: ddminhits.cpp 518 2014-03-18 05:29:23Z iwashita $
 */

#include <cerrno>
#include <cstdlib>
#include <iostream>
#include <map>
#include <stdexcept>

#include "dd/DdStructure.hpp"
#include "eval/Cardinality.hpp"
#include "op/BddHitting.hpp"
#include "op/ZddMinimal.hpp"
#include "spec/ExplicitSubsetsZdd.hpp"
//#include "util/IntItemsets.hpp"
#include "util/MessageHandler.hpp"

using namespace tdzdd;

std::string options[][2] = { //
        {"s", "Find singletons first"}, //
        {"p", "Use parallel algorithms"}, //
        {"zdd0", "Dump input ZDD to STDOUT in DOT format"}, //
        {"zdd1", "Dump intermediate ZDD to STDOUT in DOT format"}, //
        {"zdd2", "Dump output ZDD to STDOUT in DOT format"}}; //

std::map<std::string,bool> opt;
std::string infile;
std::string outfile;

void usage(char const* cmd) {
    std::cerr << "usage: " << cmd
            << " [<option>...] <input_file> [<output_file>]\n";
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

class SingleHitting: public StatelessDdSpec<SingleHitting> {
    NodeTableEntity<2>& diagram;
    NodeId root;

    MyVector<bool> skippedLevel;

    bool isHitting(int i) const {
        if (skippedLevel[i]) return false;
        size_t m = diagram[i].size();
        if (m == 0) return false;
        for (size_t j = 0; j < m; ++j) {
            if (diagram.child(i, j, 0) != 0) return false;
        }
        return true;
    }

    int goDown(int i) {
        for (;;) {
            if (i <= 0) return 0;
            if (isHitting(i)) break;
            --i;
        }

        size_t m = diagram[i].size();
        for (size_t j = 0; j < m; ++j) {
            NodeId& f0 = diagram.child(i, j, 0);
            NodeId& f1 = diagram.child(i, j, 1);
            assert(f0 == 0);
            f0 = f1;
            f1 = 0;
        }

        return i;
    }

public:
    SingleHitting(DdStructure& dd)
            : diagram(dd.getDiagram().privateEntity()), root(dd.root()),
              skippedLevel(diagram.numRows()) {
        for (int i = diagram.numRows() - 1; i > 0; --i) {
            size_t m = diagram[i].size();
            int min = i - 1;

            for (size_t j = 0; j < m; ++j) {
                for (int b = 0; b <= 1; ++b) {
                    NodeId ff = diagram.child(i, j, b);
                    if (ff != 0 && ff.row() < min) min = ff.row();
                }
            }

            for (int ii = i - 1; ii > min; --ii) {
                skippedLevel[ii] = true;
            }
        }
    }

    int getRoot() {
        return goDown(diagram.numRows() - 1);
    }

    int getChild(int i, int take) {
        assert(i > 0);
        if (take) return -1;
        return goDown(i - 1);
    }
};

template<typename M>
void output(std::ostream& os, DdStructure const& f, M const& mapper) {
    for (typeof(f.begin()) t = f.begin(); t != f.end(); ++t) {
        for (size_t i = 0; i < t->size(); ++i) {
            if (i != 0) std::cout << " ";
            os << mapper((*t)[i]);
        }
        os << "\n";
    }
}

void run() {
    MessageHandler mh;
    DdStructure f;
    ExplicitSubsetsZdd::Mapper mapper;

    mh << "\nINPUT: " << infile;
    {
        ExplicitSubsets input;

        if (infile == "-") {
            input.read(std::cin);
        }
        else {
            std::ifstream ifs(infile.c_str());
            if (!ifs) throw std::runtime_error(strerror(errno));
            input.read(ifs);
        }
        ExplicitSubsetsZdd spec(input);
        f.constructDF(spec);
        mapper = spec.mapper();
    }
    mh << "\n#item = " << f.topLevel();
    mh << "\n#itemset = " << f.evaluate(Cardinality<size_t>());

    DdStructure f1;
    size_t ssols = 0;
    if (opt["s"]) {
        bool msgFlag = MessageHandler::showMessages(false);
        f1 = DdStructure(SingleHitting(f));
        f.zddReduce();
        MessageHandler::showMessages(msgFlag);
        ssols = f1.evaluate(Cardinality<size_t>());
        mh << "\n#singleton_solution = " << ssols;
        //mh << "\n#itemset = " << f.evaluate(Cardinality<size_t>());
    }

    if (opt["zdd0"]) f.dumpDot(std::cout, "zdd0");

    f = DdStructure(BddHitting(f, opt["p"]), opt["p"]);
    f.reduce<true,true>(opt["p"]);
    mh << "\n#solution = " << f.evaluate(Cardinality<size_t>());
    if (opt["zdd1"]) f.dumpDot(std::cout, "zdd1");

    f = DdStructure(ZddMinimal<>(f), opt["p"]);
    f.zddReduce(opt["p"]);
    if (opt["zdd2"]) f.dumpDot(std::cout, "zdd2");

    size_t sols = f.evaluate(Cardinality<size_t>());
    mh << "\n#solution = ";
    if (ssols != 0) {
        mh << ssols << " + " << sols << " = ";
        sols += ssols;
    }
    mh << sols;

    if (!outfile.empty()) {
        mh << "\nOUTPUT: " << outfile;
        mh.begin("writing") << " ...";

        if (outfile == "-") {
            if (ssols != 0) output(std::cout, f1, mapper);
            output(std::cout, f, mapper);
        }
        else {
            std::ofstream ofs(outfile.c_str());
            if (!ofs) throw std::runtime_error(strerror(errno));
            if (ssols != 0) output(ofs, f1, mapper);
            output(ofs, f, mapper);
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

    if (infile.empty()) {
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
