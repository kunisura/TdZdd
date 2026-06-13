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

#include <gtest/gtest.h>

#include <cstdio>
#include <fstream>
#include <stdexcept>
#include <string>

#include <tdzdd/util/MessageHandler.hpp>
#include <tdzdd/util/Graph.hpp>

using namespace tdzdd;

namespace {

class TempFile {
    std::string filename_;

public:
    explicit TempFile(char const* filename)
            : filename_(filename) {
        std::remove(filename_.c_str());
    }

    ~TempFile() {
        std::remove(filename_.c_str());
    }

    std::string const& filename() const {
        return filename_;
    }
};

bool writeFile(std::string const& filename, char const* content) {
    std::ofstream os(filename.c_str(), std::ios::out | std::ios::binary);
    if (!os) return false;
    os << content;
    return os.good();
}

}

TEST(GraphTest, ReadEdgesAcceptsFinalLineWithoutNewline) {
    TempFile file("tdzdd_test_graph_edges_no_newline.tmp");
    ASSERT_TRUE(writeFile(file.filename(), "a b\nb c"));

    Graph graph;
    ASSERT_NO_THROW(graph.readEdges(file.filename()));
    EXPECT_EQ(3, graph.vertexSize());
    EXPECT_EQ(2, graph.edgeSize());
    EXPECT_NO_THROW(graph.getVertex("a"));
    EXPECT_NO_THROW(graph.getVertex("b"));
    EXPECT_NO_THROW(graph.getVertex("c"));
}

TEST(GraphTest, ReadEdgesRejectsIncompleteFinalLineWithoutNewline) {
    TempFile file("tdzdd_test_graph_edges_incomplete.tmp");
    ASSERT_TRUE(writeFile(file.filename(), "a b\nc"));

    Graph graph;
    EXPECT_THROW(graph.readEdges(file.filename()), std::runtime_error);
}
