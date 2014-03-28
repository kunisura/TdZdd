TdZdd
===========================================================================

### A top-down/breadth-first decision diagram manipulation framework

TdZdd is a software for manipulating ordered decision diagrams (DDs)
with efficient basic functions.

* Top-down/breadth-first DD construction
* Bottom-up/breadth-first DD evaluation
* Reduction as BDDs/ZDDs
* Parallel processing with OpenMP
* Support of *N*-ary (binary, ternary, quaternary, ...) DDs

Other features include followings.

* Header-only C++ library; no need for installation
* Distributed with sample applications
* Open-source MIT license

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

`tdzdd::DdStructure<`*N*`>` is a template class for *N*-ary DD objects.
We can construct a binary DD by giving a DD specification object
to the constructor of `tdzdd::DdStructure<2>`.

```C++
tdzdd::DdStructure<2> dd(Combination(n, k));
```

An advanced example can be found in `apps/test/example2.cpp`.

See also
---------------------------------------------------------------------------

* [Graphillion](http://graphillion.org): a Python library using TdZdd.
