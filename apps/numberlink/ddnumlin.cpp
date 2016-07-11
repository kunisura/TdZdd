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
#include <set>
#include <stdexcept>

#include <tdzdd/DdStructure.hpp>
#include <tdzdd/DdSpecOp.hpp>
#include "Board.hpp"
#include "ConstraintZdd.hpp"
#include "DegreeZdd.hpp"
#include "NumlinZdd.hpp"

using namespace tdzdd;

std::string options[][2] = //
        {{"k <n>", "Allow at most <n> blank boxes (default=infinity)"}, //
         {"a", "Enumerate all solutions"}, //
         {"p", "Use parallel processing"}, //
         {"m <n>", "Output <n> solutions at most (default=10)"}};

std::map<std::string,bool> opt;
std::map<std::string,long> optNum;
std::string infile;

void usage(char const* cmd) {
    std::cerr << "usage: " << cmd << " [ <option>... ] <input_file>\n";
    std::cerr << "options\n";
    for (unsigned i = 0; i < sizeof(options) / sizeof(options[0]); ++i) {
        std::cerr << "  -" << options[i][0];
        for (unsigned j = options[i][0].length(); j < 6; ++j) {
            std::cerr << " ";
        }
        std::cerr << ": " << options[i][1] << "\n";
    }
    std::cerr << "\n";
}

void output(std::ostream& os, DdStructure<2> const& dd, Board const& quiz,
        bool transposed) {
    int top_level = quiz.rows * (quiz.cols - 1);
    int count = 0;

    for (DdStructure<2>::const_iterator edges = dd.begin();
            edges != dd.end() && count <= optNum["m"]; ++edges) {
        os << "#" << ++count << "\n";
        Board answer = quiz;

        for (int i = 0; i < quiz.rows; ++i) {
            for (int j = 0; j < quiz.cols - 1; ++j) {
                int level = top_level - i * (quiz.cols - 1) - j;
                answer.hlink[i][j] = edges->count(level);
            }
        }

        answer.makeVerticalLinks();

        if (transposed) answer.transpose();
        //answer.fillNumbers();
        //answer.writeNumbers(os);
        answer.printNumlin(os);
        os << "\n";

        if (count == optNum["m"]) break;
    }

    double n = dd.evaluate(ZddCardinality<double,2>()) - count;
    if (n >= 1) {
        os << "  .\n  .\n  .\n\n";
        os << n << " more solution" << (n == 1 ? "" : "s") << "\n\n";
    }
}

void run() {
    MessageHandler mh;
    Board quiz;

    mh << "\nINPUT: " << infile << "\n";
    if (infile == "-") {
        quiz.readNumbers(std::cin);
    }
    else {
        std::ifstream ifs(infile.c_str());
        if (!ifs) throw std::runtime_error(strerror(errno));
        quiz.readNumbers(ifs);
    }

    quiz.printNumlin(mh);

    bool transposed = false;
    if (quiz.rows < quiz.cols) {
        mh << "\nThe board is transposed because it has more columns than rows.";
        quiz.transpose();
        transposed = true;
    }

    DdStructure<2> dd;
    NumlinZdd numlin(quiz, optNum["k"]);

    if (opt["a"]) {
        DegreeZdd degree(quiz);
        dd = DdStructure<2>(zddLookahead(degree), opt["p"]);
        dd.zddReduce();
        dd.zddSubset(zddLookahead(numlin));
    }
    else {
        ConstraintZdd constraint(quiz);
        dd = DdStructure<2>(zddLookahead(zddIntersection(constraint, numlin)),
                opt["p"]);
    }

    dd.zddReduce();
    mh << "\n#solution = " << dd.zddCardinality();

    if (optNum["m"]) {
        mh.begin("writing") << " ...\n";
        output(std::cout, dd, quiz, transposed);
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
                    optNum[s] = std::strtol(argv[++i], 0, 0);
                }
                else {
                    usage(argv[0]);
                    return 1;
                }
            }
            else if (infile.empty()) {
                infile = argv[i];
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

    if (!opt["k"]) optNum["k"] = -1;
    if (!opt["m"]) optNum["m"] = 10;

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
