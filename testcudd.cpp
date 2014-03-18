/*
 * testcudd.cpp
 */

#include <iostream>
#include "spec/CuddBdd.hpp"

using namespace tdzdd;

int main() {
//    CuddBdd(0).dumpDot();
//    CuddBdd(1).dumpDot();
    CuddBdd x1(1, 0, 1);
    CuddBdd x2(2, 0, 1);
    CuddBdd x3(3, 0, 1);
    CuddBdd f = (x1 & x2) | ~x3;
    f.dumpDot();
}
