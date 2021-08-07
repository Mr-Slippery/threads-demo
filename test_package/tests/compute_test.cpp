#include "gtest/gtest.h"

#include "compute.h"

TEST(BasicIncrementTests, IncrementOfZero) {
    int input{0};
    const auto res = increment(input);
    EXPECT_EQ(res, input + 1);
}

TEST(BasicDecrementTests, DecrementOfOne) {
    int input{0};
    const auto res = decrement(input);
    EXPECT_EQ(res, input - 1);
}

TEST(ComplexTests, IncrementOfDecrement) {
    int input{42};
    const auto res = increment(decrement(input));
    EXPECT_EQ(res, input);
}