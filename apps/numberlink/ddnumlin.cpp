/*
 * Copyright (c) 2014 Hiroaki Iwashita
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
#include <iostream>
#include <map>
#include <stdexcept>

#include <tdzdd/DdStructure.hpp>
#include <tdzdd/DdSpecOp.hpp>
#include "Board.hpp"
#include "DegreeZdd.hpp"
#include "NumlinZdd.hpp"

using namespace tdzdd;

std::string options[][2] = { //
        {"k", "Allow unused area (\"Kansai\" solutions)"}, //
        //{"rot <n>", "Rotate <n> x 90 degrees counterclockwise"}, //
        {"p", "Use parallel algorithms"}, //
        {"dd0", "Dump a state transition graph to STDOUT in DOT format"}, //
        {"dd1", "Dump a BDD before reduction to STDOUT in DOT format"}, //
        {"dump", "Dump the final ZDD to STDOUT in DOT format"}}; //

std::map<std::string,bool> opt;
std::map<std::string,int> optNum;
std::string infile;
std::string outfile;
int rot;

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

void output(std::ostream& os, DdStructure<2> const& dd,
        NumlinZdd const& numlin) {
    Board const& quiz = numlin.quiz();
    int rows = quiz.getRows();
    int cols = quiz.getCols();
    Board answer = quiz;

    for (typeof(dd.begin()) ans = dd.begin(); ans != dd.end(); ++ans) {
        for (int i = 0; i < rows; ++i) {
            for (int j = 0; j < cols - 1; ++j) {
                answer.hlink[i][j] = false;
            }
        }

        for (typeof(ans->begin()) lev = ans->begin(); lev != ans->end();
                ++lev) {
            std::div_t pos = numlin.level2pos(*lev);
            answer.hlink[pos.quot][pos.rem] = true;
        }

        answer.makeVerticalLinks();

        //answer.fillNumbers();
        //answer.writeNumbers(os);
        answer.printNumlin(os);
        os << "\n";
    }
}

void run() {
    MessageHandler mh;
    Board quiz;

    mh << "\nINPUT: " << infile << "\n";
    if (opt["rot"]) mh << "\nROTATION: " << optNum["rot"];
    if (infile == "-") {
        quiz.readNumbers(std::cin);
    }
    else {
        std::ifstream ifs(infile.c_str());
        if (!ifs) throw std::runtime_error(strerror(errno));
        quiz.readNumbers(ifs);
    }

    quiz.printNumlin(mh);

    DegreeZdd degree(quiz, opt["k"]);
    NumlinZdd numlin(quiz);

    if (opt["dd0"]) numlin.dumpDot(std::cout, "dd0");

    //DdStructure<2> dd(numlin, opt["p"]);
    //DdStructure<2> dd(zddLookahead(numlin), opt["p"]);

    DdStructure<2> dd(degree, opt["p"]);
    dd.zddReduce();
    dd.zddSubset(zddLookahead(numlin));

    if (opt["dd1"]) dd.dumpDot(std::cout, "dd1");
    dd.zddReduce();
    if (opt["dump"]) dd.dumpDot(std::cout, "ZDD");
    mh << "\n#solution = " << dd.zddCardinality();

    if (!outfile.empty()) {
        mh << "\nOUTPUT: " << outfile;
        mh.begin("writing") << " ...\n";

        if (outfile == "-") {
            output(std::cout, dd, numlin);
        }
        else {
            std::ofstream ofs(outfile.c_str());
            if (!ofs) throw std::runtime_error(strerror(errno));
            output(ofs, dd, numlin);
        }

        mh.end();
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

    rot = optNum["rot"] % 4;
    if (rot < 0) rot += 4;
    assert(0 <= rot && rot < 4);

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
