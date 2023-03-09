/*
 * https://github.com/kunisura/TdZdd/issues/7
 */

#include <gtest/gtest.h>

#include "tdzdd/DdSpec.hpp"
#include "tdzdd/DdStructure.hpp"

constexpr int ARITY = 2;  // or 3, 4, ...

struct Spec : tdzdd::StatelessDdSpec<Spec, ARITY> {
    int getRoot() const { return 1; }
    int getChild(int, int) const { return 0; }
};

TEST(Issue007, Main) {
    ASSERT_EQ(tdzdd::DdStructure<ARITY>{Spec{}},
              tdzdd::DdStructure<ARITY>{Spec{}});
}
