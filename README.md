TdZdd 1.1
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
An argument of the DD evaluation function represents procedures to be executed
at each DD node.
The construction and evaluation functions can also be used to implement
import and export functions of DD structures from/to other DD libraries.

Features of TdZdd include:

* Support of *N*-ary (binary, ternary, quaternary, ...) DDs
* Parallel processing with OpenMP
* Distributed with sample applications
* Header-only C++ library; no need for installation

This software is released under the MIT License, see [LICENSE](LICENSE).

Preparation
---------------------------------------------------------------------------
Add the absolute path of the "include" directory to your C++ include path,
or copy the "include/tdzdd" directory into an existing directory in your C++
include path (e.g. "/usr/local/include").

The "apps" directory contains several applications, which are included as sample code;
there is no need to compile them just to use TdZdd as a library.
If you are interested in compiling apps/graphillion, you will need [SAPPOROBDD](http://www.lab2.kuis.kyoto-u.ac.jp/minato/SAPPOROBDD/).
You need [CUDD](https://github.com/ivmai/cudd) to compile apps/cnfbdd and apps/cnf2ztdd2bdd.

Documents
---------------------------------------------------------------------------

* [User Guide and Reference](http://kunisura.github.io/TdZdd/doc/index.html)
* [Efficient Top-Down ZDD Construction Techniques Using Recursive Specifications](http://www-alg.ist.hokudai.ac.jp/~thomas/TCSTR/tcstr_13_69/tcstr_13_69.pdf)
<!-- * [TdZdd: フロンティア法のための効率的なトップダウンZDD構築を実現するC++ライブラリ](http://www-erato.ist.hokudai.ac.jp/docs/autumn2013/iwashita.pdf) -->
* [ZDDと列挙問題―最新の技法とプログラミングツール](http://doi.org/10.11309/jssst.34.3_97)

Related repositories
---------------------------------------------------------------------------

* [Graphillion - Fast, lightweight library for a huge number of graphs](https://github.com/takemaru/graphillion)
* [frontier_basic_tdzdd - An example implementation of the frontier-based search using TdZdd](https://github.com/junkawahara/frontier_basic_tdzdd)
* [reliability_tdzdd - Program that computes network reliability using TdZdd library](https://github.com/junkawahara/reliability_tdzdd)
* [SapporoTdZddApps - TdZdd と SAPPOROBDD の橋渡しライブラリ](https://github.com/hs-nazuna/SapporoTdZddApps)
