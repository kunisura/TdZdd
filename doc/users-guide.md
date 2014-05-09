TdZdd
===========================================================================

### A top-down/breadth-first decision diagram manipulation framework

TdZdd is an efficient C++ library for manipulating ordered decision diagrams
(DDs).
Unlike the ordinary DD libraries that are optimized for efficient operations
between multiple DDs, TdZdd focuses on direct construction of a non-trivial
DD structure from scratch.
It has three basic functions:

* Top-down/breadth-first DD construction
* Reduction as BDDs/ZDDs
* Bottom-up/breadth-first DD evaluation

The DD construction function takes user's class object as an argument,
which is a specification of the DD structure to be constructed.
An argument of the DD evaluation function represents return data type and
the procedure to be executed at each DD node.
The construction and evaluation functions can also be used to implement
import and export functions of DD structures from/to other DD libraries.

Features of TdZdd include:

* Support of *N*-ary (binary, ternary, quaternary, ...) DDs
* Parallel processing with OpenMP
* Distributed with sample applications
* Header-only C++ library; no need for installation

This software is released under the MIT License, see [LICENSE](LICENSE).

### Contents

* [Overview](#overview)
* [DD specification](#dd-specification)
* [DD structure](#dd-structure)
* [See also](#see-also)


Overview
---------------------------------------------------------------------------

A DD has one root node and two terminal nodes (⊤ and ⊥).
Every non-terminal node of an *N*-ary DD has *N* outgoing edges.

![An example of binary DD](doc/fig/example1.png)

The above picture shows an example of binary DD structure,
where the ⊥ terminal node and all edges to it are omitted for visibility;
dashed and solid lines are 0- and 1-edges respectively.
The DD represents a set of all 3-combinations out of 5 items.
Note that levels of DD nodes are defined in descending order;
the root node has the highest level and the terminal nodes have the lowest.

The following code from [apps/test/example1.cpp](apps/test/example1.cpp)
is a *DD specification* of a binary DD structure representing a set of all
*k*-combinations out of *n* items.

```cpp
class Combination: public tdzdd::DdSpec<Combination,int,2> {
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

Class `Combination` inherits from `tdzdd::DdSpec<Combination,int,2>`,
of which the first template parameter is the derived class itself,
the second one is the type of its *state*,
and the third one declares the out-degree of non-terminal DD nodes (*N*).
The class contains public member functions `int getRoot(int& state)`
and `int getChild(int& state, int level, int value)`.

`getRoot` initializes the `state` argument by the state of the root node
and returns its level.
`getChild` receives `state` and `level` of a non-terminal node and integer
`value` in the range [0, *N*-1] selecting one of the *N* branches.
It computes the state and the level of the child node.
If the child node is not a terminal, it updates `state` and returns the level.
If the child node is ⊥ or ⊤, it returns 0 or -1 respectively;
`state` is not used in those cases.

A DD represented by a DD specification can be dumped in "dot" format
for [Graphviz](http://www.graphviz.org/) visualization tools:

```cpp
Combination spec(5, 2);
spec.dumpDot(std::cout);
```

`tdzdd::DdStructure<2>` is a class of explicit binary DD structures.
Its object can be constructed from a DD specification object:

```cpp
tdzdd::DdStructure<2> dd(spec);
```

A DD structrue can be reduced as a BDD or ZDD using its `void bddReduce()` or
`void zddReduce()` member function, and also can be dumped in "dot" format:

```cpp
dd.zddReduce();
dd.dumpDot(std::cout);
```


DD specification
---------------------------------------------------------------------------

A DD specification can be defined by deriving it from an appropriate one of
base classes defined in <[tdzdd/DdSpec.hpp](include/tdzdd/DdSpec.hpp)>.

Every DD specification must have `getRoot` and `getChild` member functions.
`getRoot` defines the the root node configuration. `getChild` defines a
mapping from parent node configurations to child node configurations.
If the node is not a terminal, the functions update state information and
return a positive integer representing the node level.
If the node is ⊥ or ⊤, they simply return 0 or -1 respectively
for convenience, even though both nodes are level 0.
We refer to such a return value as a *level code*.

### DdSpec

```cpp
#include <tdzdd/DdSpec.hpp>
class S: public tdzdd::DdSpec<S,T,N>
```
is a *N*-ary DD specification using type `T` to store state information.
Class `S` must implement the following member functions:

* `int getRoot(T& state)` stores state information of the root node into
  `state` and returns a level code.

* `int getChild(T& state, int level, int value)` receives state information
  of a non-terminal node via `state`, the level of it via `level`, and a
  branch label in the range [0, *N*-1] via `value`;
  updates `state` by state information of the child node and returns a level
  code.

In addition, `S` may have to override the following member functions:

* `bool equalTo(T const& state1, T const& state2)` returns if `state1` and
  `state2` are equivalent.
  The default implementation is `state1 == state2`.

* `size_t hashCode(T const& state)` computes a hash value for `state`.
  The default implementation is `static_cast<size_t>(state)`.

Hash values for states *a* and *b* must be the same whenever *a* and *b* are
equivalent.
As shown in [Overview](#overview), we can use the default implementations and
do not need to override them when `T` is a fundamental data type such as `int`.


### PodArrayDdSpec

```cpp
#include <tdzdd/DdSpec.hpp>
class S: public tdzdd::PodArrayDdSpec<S,T,N>
```
is a *N*-ary DD specification using an array of type `T` to store state
information.
The size of the array must be fixed to some positive integer *m* by calling
predefined function `setArraySize(m)` in the constructor of `S`.
`T` must be a plain old data (POD) type, which does not use object-oriented
features such as user-defined constructor, destructor, and copy-assignment;
because the library handles the state information just as
(`sizeof(T)` × *m*)-byte raw data for efficiency.
Class `S` must implement the following member functions:

* `int getRoot(T* array)` stores state information of the root node into
  `array[0]`..`array[m-1]` and returns a level code.

* `int getChild(T* array, int level, int value)` receives state information
  of a non-terminal node via `array[0]`..`array[m-1]`, the level of it via
  `level`, and a branch label in the range [0, *N*-1] via `value`;
  updates `array[0]`..`array[m-1]` by state information of the child node
  and returns a level code.

You can find an example in [apps/test/example2.cpp](apps/test/example2.cpp).


### PodHybridDdSpec

```cpp
#include <tdzdd/DdSpec.hpp>
class S: public tdzdd::PodHybridDdSpec<S,TS,TA,N>
```
is a *N*-ary DD specification using a combination of type `TS` and an array
of type `TA` to store state information.
The size of the array must be fixed to some positive integer *m* by calling
predefined function `setArraySize(m)` in the constructor of `S`.
Both `TS` and `TA` must be plain old data (POD) types, which do not use
object-oriented features such as user-defined constructor, destructor, and
copy-assignment; because the library handles the state information just as
(`sizeof(TS)` + `sizeof(TA)` × *m*)-byte raw data for efficiency.
Class `S` must implement the following member functions:

* `int getRoot(TS& scalar, TA* array)` stores state information of the root
  node into `scalar` as well as `array[0]`..`array[m-1]` and returns a level
  code.

* `int getChild(TS& scalar, TA* array, int level, int value)` receives
  state information of a non-terminal node via `scalar` and
  `array[0]`..`array[m-1]`, the level of it via `level`, and a branch label
  in the range [0, *N*-1] via `value`;
  updates `scalar` and `array[0]`..`array[m-1]` by state information of the
  child node and returns a level code.


### StatelessDdSpec

```cpp
#include <tdzdd/DdSpec.hpp>
class S: public tdzdd::StatelessDdSpec<S,N>
```
is a *N*-ary DD specification using no state information, which means that
the DD does not have more than one nodes at any non-terminal level.
Class `S` must implement the following member functions:

* `int getRoot()` returns a level code of the root node.

* `int getChild(int level, int value)` receives level of a non-terminal
  node via `level` and a branch label in the range [0, *N*-1] via `value`;
  returns a level code of the child node.

The next example is a specification of a ZDD for a family of all
singletons out of *n* items.

```cpp
#include <tdzdd/DdSpec.hpp>

class SingletonZdd: public tdzdd::StatelessDdSpec<SingletonZdd,2> {
    int const n;

public:
    SingletonZdd(int n)
            : n(n) {
    }

    int getRoot() const {
        return n;
    }

    int getChild(int level, int value) const {
        if (value) return -1;
        return level - 1;
    }
};
```


### Visualizing a DD

Every DD specification inherits from `tdzdd::DdSpecBase<S,N>`,
which defines a member function to generate a "dot" file for
[Graphviz](http://www.graphviz.org/).

```cpp
void dumpDot(std::ostream& os = std::cout, std::string title =
tdzdd::typenameof<S>()) const;
```

Each DD specification may have to implement its own `printState` member
function that makes text labels to be drawn on the DD nodes.

```cpp
void printState(std::ostream& os, T const& state) const; // for DdSpec
void printState(std::ostream& os, T const* array) const; // for PodArrayDdSpec
void printState(std::ostream& os, TS const& scalar, TA const* array) const; // for PodHybridDdSpec
```

Text label for each level can be also customized by overriding `printLevel`
member function.

```cpp
void printLevel(std::ostream& os, int level) const;
```

DD structure
---------------------------------------------------------------------------

The template class of explicit DD structures is defined in
<[tdzdd/DdStructure.hpp](include/tdzdd/DdStructure.hpp)>.

```cpp
template<int N> class tdzdd::DdStructure;
```


### Constructors

```cpp
DdStructure();
```
constructs a new DD representing ⊥.

```cpp
DdStructure(int n, bool useMP = false);
```
constructs a new DD representing a power set of a set of *n* items.

```cpp
template<typename S>
DdStructure(tdzdd::DdSpecBase<S,N> const& spec, bool useMP = false);
```
constructs a new DD from a DD specification.

The optional argument `useMP` sets the attribute value of the object that
enables OpenMP parallel processing on it.
Note that the parallel algorithms are tuned for fairly large DDs and may
even decrease the performance for small DDs.


### Member functions

#### Subsetting

```cpp
template<typename S>
void zddSubset(tdzdd::DdSpecBase<S,N> const& spec);
```
constructs a new DD from a DD specification under the constraint represented
by the current DD and overwrites itself.
This function assumes the zero-suppress node deletion rule for input and
output DDs.

#### Reduction

```cpp
void qddReduce();
```
applies the node sharing rule only.

```cpp
void bddReduce();
```
applies the node sharing rule and the BDD node deletion rule,
which deletes a node when all its outgoing edges point to the same child.

```cpp
void zddReduce();
```
applies the node sharing rule and the ZDD node deletion rule,
which deletes a node when all its outgoing edges labeled with non-zero value
point to ⊥.

#### Comparison

```cpp
bool operator==(tdzdd::DdStructure<N> const& o) const
```
checks structural equivalence with another DD.
Compare after reduction if you want to know logical equivalence.

```cpp
bool operator!=(tdzdd::DdStructure<N> const& o) const
```
checks structural inequivalence with another DD.
Compare after reduction if you want to know logical inequivalence.

#### 

See also
---------------------------------------------------------------------------

* [Graphillion](http://graphillion.org): a Python library using TdZdd.
