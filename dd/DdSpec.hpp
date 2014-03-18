/*
 * Top-Down ZDD Construction Library for Frontier-Based Search
 * by Hiroaki Iwashita <iwashita@erato.ist.hokudai.ac.jp>
 * Copyright (c) 2012 Japan Science and Technology Agency
 * $Id: DdSpec.hpp 518 2014-03-18 05:29:23Z iwashita $
 */

#pragma once

#include <cassert>
#include <functional>
#include <iostream>

#include "DdBuilder.hpp"
#include "../util/demangle.hpp"

namespace tdzdd {

/**
 * Base class of DD specs.
 * @param S the class implementing this class.
 * @param AR arity of the nodes.
 *
 * Every implementation must have the following functions:
 * - int datasize() const
 * - int get_root(void* p)
 * - int get_child(void* p, int level, int value)
 * - void get_copy(void* to, void const* from)
 * - void destruct(void* p)
 * - void destructLevel(int level)
 * - bool needWipedown(int level)
 * - void set_wipedown_root(void* p, int level)
 * - void doWipedown(int level)
 * - size_t hash_code(void const* p, int level) const
 * - bool equal_to(void const* p, void const* q, int level) const
 * - void print_state(std::ostream& os, void const* p) const
 * Optionally, the following functions can be overloaded:
 * - void printLevel(std::ostream& os, int level) const
 * - void print_level(std::ostream& os, int level) const
 *
 * A return code of get_root(void*) or get_child(void*, int, bool)
 * is 0 when the node is forwarded to the 0-terminal
 * and is -1 when the node is forwarded to other nodes including 1-terminal.
 * WARNING: Subsetting methods support forwarding only to a terminal.
 */
template<typename S, int AR>
class DdSpec {
public:
    static int const ARITY = AR;

    S& entity() {
        return *static_cast<S*>(this);
    }

    S const& entity() const {
        return *static_cast<S const*>(this);
    }

    void printLevel(std::ostream& os, int level) const {
        os << level;
    }

    void print_level(std::ostream& os, int level) const {
        entity().printLevel(os, level);
    }

private:
    /**
     * Dumps the node table in Graphviz (dot) format.
     * @param os the output stream.
     * @param cut flag to cut 0-paths form the root and to duplicate the 1-terminal.
     * @param title title label.
     */
    void dumpDot_(std::ostream& os, bool cut, std::string title) const {
        NodeTableHandler<S::ARITY> diagram;
        InstantDdBuilder<S,true> idb(entity(), diagram, os, cut);
        NodeId root;
        idb.initialize(root);

        os << "digraph \"" << title << "\" {\n";
        for (int i = root.row(); i >= 1; --i) {
            os << "  " << i << " [shape=none,label=\"";
            entity().print_level(os, i);
            os << "\"];\n";
        }
        for (int i = root.row() - 1; i >= 1; --i) {
            os << "  " << (i + 1) << " -> " << i << " [style=invis];\n";
        }

        if (root == 1) {
            os << "  \"^\" [shape=none,label=\"" << title << "\"];\n";
            os << "  \"^\" -> \"" << root << "\" [style=dashed";
            if (root.getAttr()) os << ",arrowtail=dot";
            os << "];\n";
            if (cut) os << "  \"" << root
                    << "\" [shape=square,fixedsize=true,width=0.2,label=\"\"];\n";
        }
        else if (!cut && root != 0) {
            os << "  \"^\" [shape=none,label=\"" << title << "\"];\n";
            os << "  \"^\" -> \"" << root << "\" [style=dashed";
            if (root.getAttr()) os << ",arrowtail=dot";
            os << "];\n";
        }
        else if (!title.empty()) {
            os << "  labelloc=\"t\";\n";
            os << "  label=\"" << title << "\";\n";
        }

        for (int i = root.row(); i >= 1; --i) {
            idb.construct(i);
            diagram.derefLevel(i);
        }

        if (!cut && root != 0) {
            os << "  \"" << NodeId(1) << "\" ";
            if (cut) {
                os << "[shape=square,fixedsize=true,width=0.2,label=\"\"];\n";
            }
            else {
                os << "[shape=square,label=\"⊤\"];\n";
            }
        }

        os << "}\n";
        os.flush();
    }

public:
    /**
     * Dumps the node table in Graphviz (dot) format.
     * @param os the output stream.
     * @param title title label.
     */
    void dumpDot(std::ostream& os = std::cout, std::string title =
            typenameof<S>()) const {
        dumpDot_(os, false, title);
    }

    /**
     * Dumps the node table in Graphviz (dot) format.
     * Cuts 0-paths form the root and duplicates the 1-terminal.
     * @param os the output stream.
     * @param title title label.
     */
    void dumpDotCut(std::ostream& os = std::cout, std::string title =
            typenameof<S>()) const {
        dumpDot_(os, true, title);
    }

    /**
     * Dumps the node table in Graphviz (dot) format.
     * @param os the output stream.
     * @param o the ZDD.
     * @return os itself.
     */
    friend std::ostream& operator<<(std::ostream& os, DdSpec const& o) {
        o.dumpDot(os);
        return os;
    }
};

/**
 * Abstract class of DD specifications without states.
 * Every implementation must have the following functions:
 * - int getRoot()
 * - int getChild(int level, int value)
 * @param S the class implementing this class.
 * @param AR arity of the nodes.
 */
template<typename S, int AR = 2>
class StatelessDdSpec: public DdSpec<S,AR> {
public:
    int datasize() const {
        return 0;
    }

    int get_root(void* p) {
        return this->entity().getRoot();
    }

    int get_child(void* p, int level, int value) {
        assert(0 <= value && value < S::ARITY);
        return this->entity().getChild(level, value);
    }

    void get_copy(void* to, void const* from) {
    }

    void destruct(void* p) {
    }

    void destructLevel(int level) {
    }

    bool needWipedown(int level) {
        return false;
    }

    void set_wipedown_root(void* p, int level) {
    }

    void doWipedown(int level) {
    }

    size_t hash_code(void const* p, int level) const {
        return 0;
    }

    bool equal_to(void const* p, void const* q, int level) const {
        return true;
    }

    void print_state(std::ostream& os, void const* p) const {
        os << "＊";
    }
};

/**
 * Abstract class of DD specifications using scalar states.
 * Every implementation must have the following functions:
 * - int getRoot(T& state)
 * - int getChild(T& state, int level, int value)
 * Optionally, the following functions can be overloaded:
 * - void construct(void* p)
 * - void getCopy(void* p, T const& state)
 * - bool needWipedown(int level)
 * - void setWipedownRoot(T& state, int level)
 * - void doWipedown(int level)
 * - size_t hashCode(T const& state) const
 * - bool equalTo(T const& state1, T const& state2) const
 * - void printLevel(std::ostream& os, int level) const
 * - void printState(std::ostream& os, State const& s) const
 * @param S the class implementing this class.
 * @param T data type.
 * @param AR arity of the nodes.
 */
template<typename S, typename T, int AR = 2>
class ScalarDdSpec: public DdSpec<S,AR> {
public:
    typedef T State;

private:
    static State& state(void* p) {
        return *static_cast<State*>(p);
    }

    static State const& state(void const* p) {
        return *static_cast<State const*>(p);
    }

public:
    int datasize() const {
        return sizeof(State);
    }

    void construct(void* p) {
        new (p) State();
    }

    int get_root(void* p) {
        this->entity().construct(p);
        return this->entity().getRoot(state(p));
    }

    int get_child(void* p, int level, int value) {
        assert(0 <= value && value < S::ARITY);
        return this->entity().getChild(state(p), level, value);
    }

    void getCopy(void* p, State const& s) {
        new (p) State(s);
    }

    void get_copy(void* to, void const* from) {
        this->entity().getCopy(to, state(from));
    }

    void destruct(void* p) {
        state(p).~State();
    }

    void destructLevel(int level) {
    }

    bool needWipedown(int level) {
        return false;
    }

    void setWipedownRoot(State& s, int level) {
    }

    void set_wipedown_root(void* p, int level) {
        this->entity().setWipedownRoot(state(p), level);
    }

    void doWipedown(int level) {
    }

    size_t hashCode(State const& s) const {
        //return std::hash<State>()(s);
        return static_cast<size_t>(s);
    }

    size_t hashCodeAtLevel(State const& s, int level) const {
        return this->entity().hashCode(s);
    }

    size_t hash_code(void const* p, int level) const {
        return this->entity().hashCodeAtLevel(state(p), level);
    }

    bool equalTo(State const& s1, State const& s2) const {
        return std::equal_to<State>()(s1, s2);
    }

    bool equalToAtLevel(State const& s1, State const& s2, int level) const {
        return this->entity().equalTo(s1, s2);
    }

    bool equal_to(void const* p, void const* q, int level) const {
        return this->entity().equalToAtLevel(state(p), state(q), level);
    }

    void printState(std::ostream& os, State const& s) const {
        os << s;
    }

    void print_state(std::ostream& os, void const* p) const {
        this->entity().printState(os, state(p));
    }
};

/**
 * Abstract class of DD specifications using POD array states.
 * Every implementation must have the following functions:
 * - int getRoot(T* array)
 * - int getChild(T* array, int level, int value)
 * Optionally, the following functions can be overloaded:
 * - void printLevel(std::ostream& os, int level) const
 * - void printState(std::ostream& os, State const& s) const
 * @param S the class implementing this class.
 * @param T data type of array elements.
 * @param AR arity of the nodes.
 */
template<typename S, typename T, int AR = 2>
class PodArrayDdSpec: public DdSpec<S,AR> {
public:
    typedef T State;

private:
    typedef size_t Word;

    int arraySize;
    int dataWords;

    static State* state(void* p) {
        return static_cast<State*>(p);
    }

    static State const* state(void const* p) {
        return static_cast<State const*>(p);
    }

protected:
    void setArraySize(int n) {
        assert(0 <= n);
        arraySize = n;
        dataWords = (n * sizeof(State) + sizeof(Word) - 1) / sizeof(Word);
    }

public:
    PodArrayDdSpec()
            : arraySize(-1), dataWords(-1) {
    }

    int datasize() const {
        return dataWords * sizeof(Word);
    }

    int get_root(void* p) {
        return this->entity().getRoot(state(p));
    }

    int get_child(void* p, int level, int value) {
        assert(0 <= value && value < S::ARITY);
        return this->entity().getChild(state(p), level, value);
    }

    void get_copy(void* to, void const* from) {
        Word const* pa = static_cast<Word const*>(from);
        Word const* pz = pa + dataWords;
        Word* qa = static_cast<Word*>(to);
        while (pa != pz) {
            *qa++ = *pa++;
        }
    }

    void destruct(void* p) {
    }

    void destructLevel(int level) {
    }

    bool needWipedown(int level) {
        return false;
    }

    void setWipedownRoot(State* s, int level) {
    }

    void set_wipedown_root(void* p, int level) {
        this->entity().setWipedownRoot(state(p), level);
    }

    void doWipedown(int level) {
    }

    size_t hash_code(void const* p, int level) const {
        Word const* pa = static_cast<Word const*>(p);
        Word const* pz = pa + dataWords;
        size_t h = 0;
        while (pa != pz) {
            h += *pa++;
            h *= 314159257;
        }
        return h;
    }

    bool equal_to(void const* p, void const* q, int level) const {
        Word const* pa = static_cast<Word const*>(p);
        Word const* qa = static_cast<Word const*>(q);
        Word const* pz = pa + dataWords;
        while (pa != pz) {
            if (*pa++ != *qa++) return false;
        }
        return true;
    }

    void printState(std::ostream& os, State const* a) const {
        os << "[";
        for (int i = 0; i < arraySize; ++i) {
            if (i > 0) os << ",";
            os << a[i];
        }
        os << "]";
    }

    void print_state(std::ostream& os, void const* p) const {
        this->entity().printState(os, state(p));
    }
};

/**
 * Abstract class of DD specifications using non-POD array states.
 * Every implementation must have the following functions:
 * - int getRoot(T* array)
 * - int getChild(T* array, int level, int value)
 * Optionally, the following functions can be overloaded:
 * - void construct(void* p)
 * - void getCopy(void* p, T const& state)
 * - size_t hashCode(T const* p) const
 * - bool equalTo(T const* p, T const* q) const
 * - void printLevel(std::ostream& os, int level) const
 * - void printState(std::ostream& os, State const* a) const
 * @param S the class implementing this class.
 * @param T data type of array elements.
 * @param AR arity of the nodes.
 */
template<typename S, typename T, int AR>
class ArrayDdSpec: public DdSpec<S,AR> {
public:
    typedef T State;

private:
    int arraySize;

    static State* state(void* p) {
        return static_cast<State*>(p);
    }

    static State const* state(void const* p) {
        return static_cast<State const*>(p);
    }

protected:
    void setArraySize(int n) {
        assert(0 <= n);
        arraySize = n;
    }

public:
    ArrayDdSpec()
            : arraySize(-1) {
    }

    int datasize() const {
        return arraySize * sizeof(State);
    }

    void construct(void* p) {
        new (p) State();
    }

    int get_root(void* p) {
        for (int i = 0; i < arraySize; ++i) {
            this->entity().construct(state(p) + i);
        }

        return this->entity().getRoot(state(p));
    }

    int get_child(void* p, int level, int value) {
        assert(0 <= value && value < S::ARITY);
        return this->entity().getChild(state(p), level, value);
    }

    void getCopy(void* p, T const& s) {
        new (p) State(s);
    }

    void get_copy(void* to, void const* from) {
        for (int i = 0; i < arraySize; ++i) {
            this->entity().getCopy(state(to) + i, state(from)[i]);
        }
    }

    void destruct(void* p) {
        for (int i = 0; i < arraySize; ++i) {
            state(p)[i].~State();
        }
    }

    void destructLevel(int level) {
    }

    bool needWipedown(int level) {
        return false;
    }

    void setWipedownRoot(State* s, int level) {
    }

    void set_wipedown_root(void* p, int level) {
        this->entity().setWipedownRoot(state(p), level);
    }

    void doWipedown(int level) {
    }

    size_t hashCode(State const& state) const {
        //return static_cast<size_t>(state);
        return state.hash();
    }

    size_t hashCodeAtLevel(State const& state, int level) const {
        return this->entity().hashCode(state);
    }

    size_t hash_code(void const* p, int level) const {
        size_t h = 0;
        for (int i = 0; i < arraySize; ++i) {
            h += this->entity().hashCodeAtLevel(state(p)[i]);
            h *= 314159257;
        }
        return h;
    }

    bool equalTo(State const& state1, State const& state2) const {
        return std::equal_to<State>()(state1, state2);
    }

    bool equalToAtLevel(State const& state1, State const& state2,
            int level) const {
        return this->entity().equalTo(state1, state2);
    }

    bool equal_to(void const* p, void const* q, int level) const {
        for (int i = 0; i < arraySize; ++i) {
            if (!this->entity().equalToAtLevel(state(p)[i], state(q)[i], level)) return false;
        }
        return true;
    }

    void printState(std::ostream& os, State const* a) const {
        os << "[";
        for (int i = 0; i < arraySize; ++i) {
            if (i > 0) os << ",";
            os << a[i];
        }
        os << "]";
    }

    void print_state(std::ostream& os, void const* a) const {
        this->entity().printState(os, state(a));
    }
};

/**
 * Abstract class of DD specifications using both POD scalar and POD array states.
 * Every implementation must have the following functions:
 * - int getRoot(TS& scalar, TA* array)
 * - int getChild(TS& scalar, TA* array, int level, int value)
 * Optionally, the following functions can be overloaded:
 * - void printLevel(std::ostream& os, int level) const
 * - void printState(std::ostream& os, TS const& s, TA const* a) const
 * @param S the class implementing this class.
 * @param TS data type of scalar.
 * @param TA data type of array elements.
 * @param AR arity of the nodes.
 */
template<typename S, typename TS, typename TA, int AR = 2>
class PodHybridDdSpec: public DdSpec<S,AR> {
public:
    typedef TS S_State;
    typedef TA A_State;

private:
    typedef size_t Word;

    struct States {
        S_State s;
        A_State a[1];
    };

    int arraySize;
    int dataWords;

    static S_State& s_state(void* p) {
        return static_cast<States*>(p)->s;
    }

    static S_State const& s_state(void const* p) {
        return static_cast<States const*>(p)->s;
    }

    static A_State* a_state(void* p) {
        return static_cast<States*>(p)->a;
    }

    static A_State const* a_state(void const* p) {
        return static_cast<States const*>(p)->a;
    }

protected:
    void setArraySize(int n) {
        assert(0 <= n);
        arraySize = n;
        dataWords = (sizeof(States) + (n - 1) * sizeof(A_State) + sizeof(Word)
                - 1) / sizeof(Word);
    }

public:
    PodHybridDdSpec()
            : arraySize(-1), dataWords(-1) {
    }

    int datasize() const {
        return dataWords * sizeof(Word);
    }

    int get_root(void* p) {
        return this->entity().getRoot(s_state(p), a_state(p));
    }

    int get_child(void* p, int level, int value) {
        assert(0 <= value && value < S::ARITY);
        return this->entity().getChild(s_state(p), a_state(p), level, value);
    }

    void get_copy(void* to, void const* from) {
        Word const* pa = static_cast<Word const*>(from);
        Word const* pz = pa + dataWords;
        Word* qa = static_cast<Word*>(to);
        while (pa != pz) {
            *qa++ = *pa++;
        }
    }

    void destruct(void* p) {
    }

    void destructLevel(int level) {
    }

    bool needWipedown(int level) {
        return false;
    }

    void setWipedownRoot(S_State& s, A_State* a, int level) {
    }

    void set_wipedown_root(void* p, int level) {
        this->entity().setWipedownRoot(s_state(p), a_state(p), level);
    }

    void doWipedown(int level) {
    }

    size_t hash_code(void const* p, int level) const {
        Word const* pa = static_cast<Word const*>(p);
        Word const* pz = pa + dataWords;
        size_t h = 0;
        while (pa != pz) {
            h += *pa++;
            h *= 314159257;
        }
        return h;
    }

    bool equal_to(void const* p, void const* q, int level) const {
        Word const* pa = static_cast<Word const*>(p);
        Word const* qa = static_cast<Word const*>(q);
        Word const* pz = pa + dataWords;
        while (pa != pz) {
            if (*pa++ != *qa++) return false;
        }
        return true;
    }

    void printState(std::ostream& os, S_State const& s,
            A_State const* a) const {
        os << "[";
        os << s << ":";
        for (int i = 0; i < arraySize; ++i) {
            if (i > 0) os << ",";
            os << a[i];
        }
        os << "]";
    }

    void print_state(std::ostream& os, void const* p) const {
        this->entity().printState(os, s_state(p), a_state(p));
    }
};

} // namespace tdzdd
