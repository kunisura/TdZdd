TdZdd
===========================================================================

### A top-down/breadth-first decision diagram manipulation framework

TdZdd is a software for manipulating ordered decision diagrams (DDs)
with efficient basic functions.

* Top-down/breadth-first DD construction
* Bottom-up/breadth-first DD evaluation
* Reduction as BDDs/ZDDs
* Parallel processing with OpenMP
* Support of *n*-ary (binary, ternary, quaternary, ...) DDs

Other features include followings.

* Header-only C++ library; no need for installation
* Distributed with sample applications
* Open-source MIT license

Simple example
---------------------------------------------------------------------------

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

    int getChild(int& state, int level, int take) const {
        state += take;
        if (--level == 0) return (state == k) ? -1 : 0;
        if (state > k) return 0;
        if (state + level < k) return 0;
        return level;
    }
};
```
