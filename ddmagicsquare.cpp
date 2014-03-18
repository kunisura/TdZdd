#include <cstdlib>
#include <iostream>
#include <map>
#include <stdexcept>

#include "dd/DdStructure.hpp"
#include "dd/PathCounter.hpp"
#include "eval/Cardinality.hpp"
#include "op/BinaryOperation.hpp"
#include "spec/PermutationZdd.hpp"
#include "util/MessageHandler.hpp"

using namespace tdzdd;

std::string options[][2] = { //
        {"dump", "Dump a state transition diagram to STDOUT in DOT format"}}; //

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

class MagicSquareZdd: public PodArrayDdSpec<MagicSquareZdd,int16_t> {
    int const size;
    int const values;
    int const ddvars;
    int const sum;

    int16_t& row(int16_t* array) {
        return array[0];
    }

    int16_t& udg(int16_t* array) {
        return array[1];
    }

    int16_t& ddg(int16_t* array) {
        return array[2];
    }

    int16_t& col(int16_t* array, int j) {
        assert(0 <= j && j < size);
        return array[3 + j];
    }

    bool update(int16_t& s, int k, int v) {
        s -= v;
        if (k < size - 1) {
            int r = size - k - 1; // remaining cells
            if (s < r * (r + 1) / 2) return true;
            if (s > r * (2 * values - r + 1) / 2) return true;
        }
        else {
            if (s != 0) return true;
            s = sum;
        }
        return false;
    }

public:
    MagicSquareZdd(int n)
            : size(n), values(size * size), ddvars(values * size * size),
              sum(n * (values + 1) / 2) {
        setArraySize(3 + size);
    }

    int getRoot(int16_t* a) {
        row(a) = sum;
        udg(a) = sum;
        ddg(a) = sum;
        for (int j = 0; j < size; ++j)
            col(a, j) = sum;
        return ddvars;
    }

    int getChild(int16_t* a, int level, int take) {
        if (take) {
            std::div_t d1 = std::div(ddvars - level, values);
            std::div_t d2 = std::div(d1.quot, size);
            int i = d2.quot;
            int j = d2.rem;
            int v = d1.rem + 1;
            assert(0 <= i && i < size);
            assert(0 <= j && j < size);
            assert(1 <= v && v <= values);

            if (update(row(a), j, v)) return 0;
            //if (i + j == size - 1 && update(udg(a), i, v)) return 0;
            //if (i == j && update(ddg(a), i, v)) return 0;
            if (update(col(a, j), i, v)) return 0;
        }
        return --level ? level : -1;
    }
};

class MagicSquareRowZdd: public ScalarDdSpec<MagicSquareRowZdd,int16_t> {
    int const size;
    int const values;
    int const ddvars;
    int const sum;

    bool update(int16_t& s, int k, int v) {
        s -= v;
        if (k < size - 1) {
            int r = size - k - 1; // remaining cells
            if (s < r * (r + 1) / 2) return true;
            if (s > r * (2 * values - r + 1) / 2) return true;
        }
        else {
            if (s != 0) return true;
            s = sum;
        }
        return false;
    }

public:
    MagicSquareRowZdd(int n)
            : size(n), values(size * size), ddvars(values * size * size),
              sum(n * (values + 1) / 2) {
    }

    int getRoot(int16_t& s) {
        s = sum;
        return ddvars;
    }

    int getChild(int16_t& s, int level, int take) {
        if (take) {
            std::div_t d1 = std::div(ddvars - level, values);
            std::div_t d2 = std::div(d1.quot, size);
            //int i = d2.quot;
            int j = d2.rem;
            int v = d1.rem + 1;
            //assert(0 <= i && i < size);
            assert(0 <= j && j < size);
            assert(1 <= v && v <= values);

            if (update(s, j, v)) return 0;
        }
        return --level ? level : -1;
    }
};

class MagicSquareColZdd: public ScalarDdSpec<MagicSquareColZdd,int16_t> {
    int const size;
    int const values;
    int const ddvars;
    int const sum;
    int const col;

    bool update(int16_t& s, int k, int v) {
        s -= v;
        if (k < size - 1) {
            int r = size - k - 1; // remaining cells
            if (s < r * (r + 1) / 2) return true;
            if (s > r * (2 * values - r + 1) / 2) return true;
        }
        else {
            if (s != 0) return true;
            s = sum;
        }
        return false;
    }

public:
    MagicSquareColZdd(int n, int col)
            : size(n), values(size * size), ddvars(values * size * size),
              sum(n * (values + 1) / 2), col(col) {
    }

    int getRoot(int16_t& s) {
        s = sum;
        return ddvars;
    }

    int getChild(int16_t& s, int level, int take) {
        if (take) {
            std::div_t d1 = std::div(ddvars - level, values);
            std::div_t d2 = std::div(d1.quot, size);
            int i = d2.quot;
            int j = d2.rem;
            int v = d1.rem + 1;
            assert(0 <= i && i < size);
            assert(0 <= j && j < size);
            assert(1 <= v && v <= values);

            if (j == col && update(s, i, v)) return 0;
        }
        return --level ? level : -1;
    }
};

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
    mh << "\nn = " << n << ", #var = " << (n * n * n * n);

    //PermutationZdd perm(n * n);
    //MagicSquareZdd mgsq(n);
    //dumpDot(mgsq, std::cout);
    //ZddIntersection<PermutationZdd,MagicSquareZdd> spec(perm, mgsq);
    //dumpDot(spec, std::cout);

    //DdStructure f(n*n*n*n);
    //DdStructure f(perm, true);
    DdStructure f(MagicSquareZdd(n), true);
    f.zddReduce(true);
    //f.zddSubset(mgsq, true);
    f.zddSubset(PermutationZdd(n*n), true);
    if (opt["dump"]) f.dumpDot(std::cout);
    f.zddReduce(true);

    mh << "\n#solution = " << f.evaluate(Cardinality<>());

    mh.end("finished");
    return 0;
}
