TdZdd User Guide						{#mainpage}
===========================================================================

* [Overview](#overview)
* [DD specification](#dd-specification)
* [Operations on DD specifications](#operations-on-dd-specifications)
* [DD structure](#dd-structure)


===========================================================================
<a name="overview"></a>
Overview
===========================================================================

A DD has one root node and two terminal nodes (⊤ and ⊥).
Every non-terminal node of an *N*-ary DD has *N* outgoing edges.

![An example of binary DD](images/example1.png)

The above picture shows an example of binary DD structure,
where the ⊥ terminal node and all edges to it are omitted for visibility;
dashed and solid lines are 0- and 1-edges respectively.
The DD represents a set of all 3-combinations out of 5 items.
Note that levels of DD nodes are defined in descending order;
the root node has the highest level and the terminal nodes have the lowest.

The following code from [apps/test/example1.cpp](../apps/test/example1.cpp)
is a *DD specification* of a binary DD structure representing a set of all
*k*-combinations out of *n* items.

~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~{.cpp}
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
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

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
for [Graphviz](http://www.graphviz.org/) visualization tools.

~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~{.cpp}
Combination spec(5, 2);
spec.dumpDot(std::cout);
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

`tdzdd::DdStructure<2>` is a class of explicit binary DD structures.
Its object can be constructed from a DD specification object.

~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~{.cpp}
tdzdd::DdStructure<2> dd(spec);
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

A DD structrue can be reduced as a BDD or ZDD using its `void bddReduce()` or
`void zddReduce()` member function, and also can be dumped in "dot" format.

~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~{.cpp}
dd.zddReduce();
dd.dumpDot(std::cout);
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

A DD structure can be evaluated from the bottom to the top.
The following code from
[apps/test/testSizeConstraint.cpp](../apps/test/testSizeConstraint.cpp) is a
*DD evaluator* to find the size of the largest itemset represented by a ZDD.

~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~{.cpp}
class MaxNumItems: public tdzdd::DdEval<MaxNumItems,int> {
public:
    void evalTerminal(int& n, bool one) const {
        n = one ? 0 : INT_MIN;
    }

    void evalNode(int& n, int, DdValues<int,2> const& values) const {
        n = std::max(values.get(0), values.get(1) + 1);
    }
};
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

It is used as follows.

~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~{.cpp}
int max = dd.evaluate(MaxNumItems());
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

The construction and evaluation functions can also be used easily to implement
import and export functions of DD structures from/to other DD libraries.


===========================================================================
<a name="dd-specification"></a>
DD specification
===========================================================================

A DD specification is defined by deriving it from an appropriate one of the
base classes defined in <tdzdd/DdSpec.hpp>.

Every DD specification must have `getRoot` and `getChild` member functions.
`getRoot` defines the root node configuration. `getChild` defines a mapping
from parent node configurations to child node configurations.
If the node is not a terminal, the functions update state information and
return a positive integer representing the node level.
If the node is ⊥ or ⊤, they simply return 0 or -1 respectively
for convenience, even though both nodes are level 0.
We refer to such a return value as a *level code*.

DdSpec
---------------------------------------------------------------------------

~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~{.cpp}
#include <tdzdd/DdSpec.hpp>
class S: public tdzdd::DdSpec<S,T,N> { ... };
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
is an *N*-ary DD specification using type `T` to store state information.
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

Hash values for any two states must be the same whenever they are equivalent.
As shown in [Overview](#overview), we can use the default implementations and
do not need to override them when `T` is a fundamental data type such as `int`.


PodArrayDdSpec
---------------------------------------------------------------------------

~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~{.cpp}
#include <tdzdd/DdSpec.hpp>
class S: public tdzdd::PodArrayDdSpec<S,T,N> { ... };
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
is an *N*-ary DD specification using an array of type `T` to store state
information.
The size of the array must be fixed to some positive integer *m* by calling
predefined function `setArraySize(m)` in the constructor of `S`.
The library handles the state information just as (`sizeof(T)` × *m*)-byte
raw data for efficiency;
therefore, `T` must be a data structure without object-oriented features
(user-defined constructor, destructor, copy-assignment, etc.).
Class `S` must implement the following member functions:

* `int getRoot(T* array)` stores state information of the root node into
  `array[0]`..`array[m-1]` and returns a level code.

* `int getChild(T* array, int level, int value)` receives state information
  of a non-terminal node via `array[0]`..`array[m-1]`, the level of it via
  `level`, and a branch label in the range [0, *N*-1] via `value`;
  updates `array[0]`..`array[m-1]` by state information of the child node
  and returns a level code.

You can find an example in [apps/test/example2.cpp](../apps/test/example2.cpp).


HybridDdSpec
---------------------------------------------------------------------------

~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~{.cpp}
#include <tdzdd/DdSpec.hpp>
class S: public tdzdd::HybridDdSpec<S,TS,TA,N> { ... };
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
is an *N*-ary DD specification using a combination of type `TS` and an array
of type `TA` to store state information.
The size of the array must be fixed to some positive integer *m* by calling
predefined function `setArraySize(m)` in the constructor of `S`.
The library handles the state information of the array just as
(`sizeof(TA)` × *m*)-byte raw data for efficiency;
therefore, `TA` must be a data structure without object-oriented
features (user-defined constructor, destructor, copy-assignment, etc.).
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


StatelessDdSpec
---------------------------------------------------------------------------

~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~{.cpp}
#include <tdzdd/DdSpec.hpp>
class S: public tdzdd::StatelessDdSpec<S,N> { ... };
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
is an *N*-ary DD specification using no state information, which means that
the DD does not have more than one node at any non-terminal level.
Class `S` must implement the following member functions:

* `int getRoot()` returns a level code of the root node.

* `int getChild(int level, int value)` receives level of a non-terminal
  node via `level` and a branch label in the range [0, *N*-1] via `value`;
  returns a level code of the child node.

The next example is a specification of a ZDD for a family of all
singletons out of *n* items.

~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~{.cpp}
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
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~


DdSpecBase
---------------------------------------------------------------------------

Every DD specification inherits from `tdzdd::DdSpecBase<S,N>`,
which defines common member functions including the one to generate a "dot"
file for [Graphviz](http://www.graphviz.org/).

~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~{.cpp}
void tdzdd::DdSpecBase<S,N>::dumpDot(std::ostream& os = std::cout, std::string title = tdzdd::typenameof<S>()) const;
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

Each DD specification may have to define its own `printState` member
function that prints text on each DD nodes.

~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~{.cpp}
void tdzdd::DdSpecBase<S,N>::printState(std::ostream& os, T const& state) const; // for DdSpec
void tdzdd::DdSpecBase<S,N>::printState(std::ostream& os, T const* array) const; // for PodArrayDdSpec
void tdzdd::DdSpecBase<S,N>::printState(std::ostream& os, TS const& scalar, TA const* array) const; // for HybridDdSpec
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

Text of each level can also be customized by overriding `printLevel`
member function.

~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~{.cpp}
void tdzdd::DdSpecBase<S,N>::printLevel(std::ostream& os, int level) const;
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

The default implementation prints the `level` itself.


===========================================================================
<a name="operations-on-dd-specifications"></a>
Operations on DD specifications
===========================================================================

Operations on DD specifications are defined as global functions in
<tdzdd/DdSpecOp.hpp>.
Note that they are also applicable to DD structures.

~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~{.cpp}
template<typename S1, typename S2>
tdzdd::BddAnd<S1,S2> tdzdd::bddAnd(S1 const& spec1, S2 const& spec2);
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
returns a BDD specification for logical AND of two BDD specifications.

~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~{.cpp}
template<typename S1, typename S2>
tdzdd::BddOr<S1,S2> tdzdd::bddOr(S1 const& spec1, S2 const& spec2);
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
returns a BDD specification for logical OR of two BDD specifications.

~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~{.cpp}
template<typename S1, typename S2>
tdzdd::ZddIntersection<S1,S2> tdzdd::zddIntersection(S1 const& spec1, S2 const& spec2);
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
returns a ZDD specification for set intersection of two ZDD specifications.

~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~{.cpp}
template<typename S1, typename S2>
tdzdd::ZddUnion<S1,S2> tdzdd::zddUnion(S1 const& spec1, S2 const& spec2);
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
returns a ZDD specification for set union of two ZDD specifications.

~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~{.cpp}
template<typename S>
tdzdd::BddLookahead<S> tdzdd::bddLookahead(S const& spec);
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
optimizes a BDD specification in terms of the BDD node deletion rule.

~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~{.cpp}
template<typename S>
tdzdd::ZddLookahead<S> tdzdd::zddLookahead(S const& spec);
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
optimizes a ZDD specification in terms of the ZDD node deletion rule.

~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~{.cpp}
template<typename S>
tdzdd::BddUnreduction<S> tdzdd::bddUnreduction(S const& spec, int numVars);
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
creates a QDD specification from a BDD specification by complementing
skipped nodes in terms of the BDD node deletion rule.

~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~{.cpp}
template<typename S>
tdzdd::ZddUnreduction<S> tdzdd::zddUnreduction(S const& spec, int numVars);
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
creates a QDD specification from a ZDD specification by complementing
skipped nodes in terms of the ZDD node deletion rule.


===========================================================================
<a name="dd-structure"></a>
DD structure
===========================================================================

The template class of DD structures is defined in <tdzdd/DdStructure.hpp>.

~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~{.cpp}
template<int N>
class tdzdd::DdStructure;
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

Note that `DdStructure<N>` works also as the DD specification that represent
itself.


Construction
---------------------------------------------------------------------------

A DD structure can be constructed from a DD specification using the following
templated constructor.

~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~{.cpp}
template<typename S>
tdzdd::DdStructure<N>::DdStructure(tdzdd::DdSpecBase<S,N> const& spec, bool useMP = false);
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

The optional argument `useMP` sets the attribute value of the object that
enables OpenMP parallel processing.
Note that the parallel algorithms are tuned for fairly large DDs and may
not be very effective on small DDs.

The default constructor of `tdzdd::DdStructure<N>` creates a new DD
representing ⊥.
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~{.cpp}
tdzdd::DdStructure<N>::DdStructure();
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

The next one constructs a new DD with *n* non-terminal nodes at levels
*n* to 1, of which each one has *N* edges pointing to the same node at the
next lower level.
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~{.cpp}
tdzdd::DdStructure<N>::DdStructure(int n, bool useMP = false);
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
When *N* = 2, it is a ZDD for the power set of a set of *n* items.


Subsetting
---------------------------------------------------------------------------

TdZdd provides an efficient method, called *ZDD subsetting*, to refine an
existing ZDD by adding a constraint given as a DD specification.

~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~{.cpp}
template<typename S>
void tdzdd::DdStructure<N>::zddSubset(tdzdd::DdSpecBase<S,N> const& spec);
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
This templated member function of `tdzdd::DdStructure<N>` updates the current
DD by a new DD for ZDD intersection of itself and the given DD specification.
The original DD should be reduced as a ZDD in advance for this function to
work efficiently.


Reduction
---------------------------------------------------------------------------

The following member functions of `tdzdd::DdStructure<N>` apply reduction
rules to the current DD structure.

~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~{.cpp}
void tdzdd::DdStructure<N>::qddReduce();
void tdzdd::DdStructure<N>::bddReduce();
void tdzdd::DdStructure<N>::zddReduce();
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

`qddReduce()` applies the node sharing rule only.
`bddReduce()` applies the node sharing rule and the BDD node deletion rule,
which deletes a node when all its outgoing edges point to the same child.
`zddReduce()` applies the node sharing rule and the ZDD node deletion rule,
which deletes a node when all of its non-zero-labeled outgoing edges point
to ⊥.


Evaluation
---------------------------------------------------------------------------

A DD evaluator can be defined by deriving it from `tdzdd::DdEval<E,T>`
defined in <[tdzdd/DdSpec.hpp](../include/tdzdd/DdSpec.hpp)>.

~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~{.cpp}
#include <tdzdd/DdEval.hpp>
class E: public tdzdd::DdEval<E,T> { ... };
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
defines the DD evaluator *E* that computes a value of type *T*.

A `tdzdd::DDStructure<N>` object can be evaluated by *E* as follows.
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~{.cpp}
tdzdd::DdStructure<N> dd = ... ;
T value = dd.evaluate(E());
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

Every DD evaluator must define the following functions:

* `void evalTerminal(T& v, int id)` receives a terminal node identifier
  (0 or 1) via `id` and writes the value for it into `v`.

* `void evalNode(T& v, int level, tdzdd::DdValues<T,N> const& values)` receives
  the `level` of the node to be evaluated and evaluation results for its
  child nodes via `values`;
  writes the result value into `v`.

In `evalNode`, the value and level of the *b*-th child node (*b* = 0..*N*-1)
can be obtained through `values.get(b)` and `values.getLevel(b)` respectively.


Utility functions
---------------------------------------------------------------------------

The following member functions of `tdzdd::DdStructure<N>` might be also useful.

~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~{.cpp}
int tdzdd::DdStructure<N>::topLevel() const;
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
gets the level of the root node.

~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~{.cpp}
size_t tdzdd::DdStructure<N>::size() const;
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
gets the number of nonterminal nodes.

~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~{.cpp}
bool tdzdd::DdStructure<N>::operator==(tdzdd::DdStructure<N> const& o) const;
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
checks structural equivalence with another DD.
Compare after reduction if you want to check logical equivalence.

~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~{.cpp}
bool tdzdd::DdStructure<N>::operator!=(tdzdd::DdStructure<N> const& o) const;
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
checks structural inequivalence with another DD.
Compare after reduction if you want to check logical inequivalence.

~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~{.cpp}
tdzdd::DdStructure<N> tdzdd::DdStructure<N>::bdd2zdd(int numVars) const;
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
transforms a BDD into a ZDD.

~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~{.cpp}
tdzdd::DdStructure<N> tdzdd::DdStructure<N>::zdd2bdd(int numVars) const;
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
transforms a ZDD into a BDD.

~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~{.cpp}
std::string tdzdd::DdStructure<N>::bddCardinality(int numVars) const;
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
counts the number of minterms of the function represented by this BDD.

~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~{.cpp}
std::string tdzdd::DdStructure<N>::zddCardinality() const;
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
counts the number of itemsets in the family of itemsets represented
by this ZDD.

~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~{.cpp}
tdzdd::DdStructure<N>::const_iterator tdzdd::DdStructure<N>::begin() const;
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
returns an iterator to the first element of the family of itemsets
represented by this ZDD.
Each itemset is represented by `std::set<int>` of levels of selected items.

~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~{.cpp}
tdzdd::DdStructure<N>::const_iterator tdzdd::DdStructure<N>::end() const;
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
returns an iterator to the element following the last element of
the family of itemsets represented by this ZDD.
