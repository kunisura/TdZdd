TdZdd
===========================================================================

### A top-down/breadth-first decision diagram manipulation framework

TdZdd is a C++ library for manipulating ordered decision diagrams (DDs)
efficiently with following basic functions.

* Top-down/breadth-first DD construction
* Reduction as BDDs/ZDDs
* Dump in a Graphviz (dot) format
* Bottom-up/breadth-first DD evaluation

The DD construction function takes user's class object as an argument,
which is a specification of the DD structure to be constructed.
An argument of the DD evaluation function represents return data type and
the procedure to be executed at each DD node.
The construction and evaluation functions can also be used to implement
import and export functions of DD structures from/to standard BDD packages.

Other features include below.

* Support of *N*-ary (binary, ternary, quaternary, ...) DDs
* Parallel processing with OpenMP
* Header-only C++ library; no need for installation
* Distributed with sample applications

This software is released under the MIT License, see LICENSE.

Example of DD specifications
---------------------------------------------------------------------------

The following code from `apps/test/example1.cpp` is a *DD specification* of
a binary DD representing a set of all *k*-combinations out of *n* items.

```C++
#include <tdzdd/DdStructure.hpp>

class Combination: public tdzdd::ScalarDdSpec<Combination,int,2> {
    int const n;
    int const k;

public:
    Combination(int n, int k)
            : n(n), k(k) {
    }

    int getRoot(int& state) const {
        state = 0;
        return n;
    }

    int getChild(int& state, int level, int value) const {
        state += value;
        if (--level == 0) return (state == k) ? -1 : 0;
        if (state > k) return 0;
        if (state + level < k) return 0;
        return level;
    }
};
```

`tdzdd::DdStructure<2>` is a template class for binary DD objects.
We can construct a binary DD by giving a DD specification object
to its constructor.

```C++
tdzdd::DdStructure<2> dd(Combination(n, k));
```

An advanced example can be found in `apps/test/example2.cpp`.

See also
---------------------------------------------------------------------------

* [Graphillion](http://graphillion.org): a Python library using TdZdd.
