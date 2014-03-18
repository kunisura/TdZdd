/*
 * Top-Down ZDD Construction Library for Frontier-Based Search
 * by Hiroaki Iwashita <iwashita@erato.ist.hokudai.ac.jp>
 * Copyright (c) 2013 Japan Science and Technology Agency
 * $Id: DdBuilderDF.hpp 517 2014-01-29 05:15:16Z iwashita $
 */

#pragma once

#include "UniqueTable.hpp"
#include "../util/MyList.hpp"

namespace tdzdd {

template<typename S>
class DdBuilderDF {
    typedef S Spec;
    static int const AR = Spec::ARITY;

    Spec spec;
    NodeTableEntity<AR>& output;
    MyList<char> stack;
    UniqueTable<AR> uniq;

    void* push() {
        return stack.alloc_front(spec.datasize());
    }

    void pop() {
        stack.pop_front();
    }

    void* front() {
        return stack.front();
    }

public:
    DdBuilderDF(Spec const& s, NodeTableHandler<AR>& output)
            : spec(s), output(output.privateEntity()) {
    }

    NodeId construct() {
        int n = spec.get_root(push());

        if (n <= 0) {
            output.init(1);
            return (n == 0) ? 0 : 1;
        }

        output.init(n + 1);
        uniq.init(output);
        return constructRecursively(n);
    }

private:
    NodeId constructRecursively(int i) {
        if (i == 0) return 0;
        if (i < 0) return 1;

        Node<AR> node;
        void* s = front();
        void* ss = push();

        for (int b = 0; b < AR - 2; ++b) {
            spec.get_copy(ss, s);
            int ii = spec.get_child(ss, i, b);
            node.branch[b] = constructRecursively(ii);
            spec.destruct(ss);
        }
        spec.get_copy(ss, s);
        int i0 = spec.get_child(ss, i, AR - 2);
        node.branch[AR - 2] = constructRecursively(i0);
        pop();
        int i1 = spec.get_child(s, i, AR - 1);
        node.branch[AR - 1] = constructRecursively(i1);

        return uniq.getNode(i, node);
    }
};

} // namespace tdzdd
