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

#include <cassert>
#include <vector>
#include <gtest/gtest.h>
#include <tdzdd/DdStructure.hpp>

using namespace tdzdd;

extern bool useMP;

class Simpath: public PodArrayDdSpec<Simpath,int,2> {
    typedef std::pair<int,int> Edge;

    int const rows;
    int const cols;
    int const numVertex; // V = {1..numVertex}
    int const numEdge;   // E = {0..numEdge-1}
    int const mateSize;
    std::vector<Edge> edges;

    class MateArray {
        int* const array;
        int const offset;
        int const size;

    public:
        MateArray(int* state, int offset, int size)
                : array(state - offset), offset(offset), size(size) {
        }

        int& operator[](int v) {
            assert(0 <= v - offset && v - offset < size);
            return array[v];
        }
    };

public:
    Simpath(int rows, int cols)
            : rows(rows), cols(cols), numVertex(rows * cols),
              numEdge(rows * cols * 2 - rows - cols), mateSize(cols + 1) {
        edges.reserve(numEdge);
        for (int i = 0; i < rows; ++i) {
            for (int j = 0; j < cols; ++j) {
                int v = i * cols + j + 1;
                if (j + 1 < cols) edges.push_back(std::make_pair(v, v + 1));
                if (i + 1 < rows) edges.push_back(std::make_pair(v, v + cols));
            }
        }
        assert(edges.size() == size_t(numEdge));

        setArraySize(mateSize);
    }

    int getRoot(int* state) const {
        MateArray mate(state, 1, mateSize);
        mate[1] = -1;
        for (int v = 2; v <= mateSize; ++v) {
            mate[v] = (v == numVertex) ? -1 : v;
        }
        return numEdge;
    }

    int getChild(int* state, int level, int take) const {
        int e = numEdge - level;
        assert(0 <= e && e < numEdge);
        int v1 = edges[e].first;
        int v2 = edges[e].second;
        MateArray mate(state, v1, mateSize);

        if (take) {
            int w1 = mate[v1];
            int w2 = mate[v2];

            if (w1 == 0 || w2 == 0) return 0; // already visited
            if (w1 == v2) return 0; // cycle made

            if (w1 < 0 && w2 < 0) { // s-t path completed
                for (int v = v1 + 1; v < v1 + mateSize; ++v) {
                    if (v == v2) continue;
                    if (mate[v] != 0 && mate[v] != v) return 0; // endpoint found
                }
                return -1; // OK
            }

            mate[v1] = 0;
            mate[v2] = 0;
            if (w1 > 0) mate[w1] = w2;
            if (w2 > 0) mate[w2] = w1;
        }

        if (e + 1 < numEdge) {
            int vv = edges[e + 1].first;
            int d = vv - v1;
            if (d > 0) {
                for (int v = v1; v < vv; ++v) { // check leaving elements
                    if (mate[v] != 0 && mate[v] != v) return 0; // endpoint found
                }
                for (int v = vv; v < v1 + mateSize; ++v) { // shift elements
                    mate[v - d] = mate[v];
                }
                for (int v = v1 + mateSize; v < vv + mateSize; ++v) { // init new elements
                    mate[v - d] = (v == numVertex) ? -1 : v;
                }
            }
        }

        return level - 1;
    }
};

TEST(Example2, Simpath) {
    std::string A007764[] =
            {"1", "2", "12", "184", "8512", "1262816", "575780564",
             "789360053252", "3266598486981642", "41044208702632496804",
             "1568758030464750013214100"};

    for (int n = 1; n <= 10; ++n) {
        DdStructure<2> dd(Simpath(n + 1, n + 1), useMP);

        ASSERT_EQ(A007764[n], dd.zddCardinality());
        dd.zddReduce();
        ASSERT_EQ(A007764[n], dd.zddCardinality());
    }
}
