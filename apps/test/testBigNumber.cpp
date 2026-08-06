/*
 * TdZdd: a Top-down/Breadth-first Decision Diagram Manipulation Framework
 * by Hiroaki Iwashita <iwashita@erato.ist.hokudai.ac.jp>
 * Copyright (c) 2014 ERATO MINATO Project
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 */

#include <gtest/gtest.h>

#include <sstream>
#include <string>

#include <tdzdd/util/BigNumber.hpp>

using namespace tdzdd;

namespace {

std::string toString(BigNumber const& n) {
    std::ostringstream os;
    os << n;
    return os.str();
}

} // namespace

TEST(BigNumberTest, TranslateZeroOnDefaultConstructed) {
    BigNumber n; // array == 0 represents the value 0
    EXPECT_EQ(0ULL, n.translate<unsigned long long>());
}

TEST(BigNumberTest, TranslateNonZero) {
    uint64_t storage[16] = {0};
    BigNumber n(storage);

    n.store(0x123456789ABCDEFULL);
    EXPECT_EQ(0x123456789ABCDEFULL, n.translate<unsigned long long>());
}

TEST(BigNumberTest, ShiftLeftWordShift) {
    // Values cross-checked with Python integer arithmetic.
    {
        uint64_t storage[16] = {0};
        BigNumber n(storage);
        n.store(1);
        n.shiftLeft(200);
        EXPECT_EQ(
                "1606938044258990275541962092341162602522202993782792835301376",
                toString(n)); // 1 << 200
    }
    {
        uint64_t storage[16] = {0};
        BigNumber n(storage);
        n.store((uint64_t(1) << 63) - 1);
        n.shiftLeft(63);
        EXPECT_EQ("85070591730234615856620279821087277056",
                toString(n)); // (2**63 - 1) << 63
    }
    {
        uint64_t storage[16] = {0};
        BigNumber n(storage);
        n.store(0x5EADBEEFCAFEBABEULL);
        n.shiftLeft(100); // make it span multiple words first
        n.shiftLeft(130);
        EXPECT_EQ(
                "1177147871839296290137194946937083262666837731232894736550"
                "4683601285120896141021111386112",
                toString(n)); // 0x5EADBEEFCAFEBABE << 230
    }
}
