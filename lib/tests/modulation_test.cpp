#include "modulation.hpp"
#include <vector>
#include <complex>
#include <gtest/gtest.h>
#include <gmock/gmock.h>

using ::testing::Pointwise;
using ::testing::Truly;

TEST(ModulationTest, LLRCorrect)
{
    auto r = 0.6d - 1.4j;
    auto llr = modulation::e16QAM{}.likelihood<double>(r, 0.5);
    EXPECT_THAT(llr, Pointwise(testing::DoubleEq(), std::vector<double>{4.8, -11.2, 11.2, 4.8}));
}
