#include "modulation.hpp"
#include <vector>
#include <complex>
#include <gtest/gtest.h>
#include <gmock/gmock.h>

using ::testing::Pointwise;
using ::testing::Truly;

TEST(ModulationTest, 16QAMSoftDecodeMatchesHardDecode)
{
    std::vector<uint8_t> in{'H', 'e', 'l', 'l', 'o'};

    auto buf = modulation::to_constl<modulation::e16QAM>(in);
    auto hard = modulation::from_constl<modulation::e16QAM>(buf);
    auto soft = modulation::from_constl_llr<modulation::e16QAM>(buf, 26.0d);

    EXPECT_EQ(hard, soft);
}

TEST(ModulationTest, Transforms16QAMForthAndBackCorrectly)
{
    std::vector<uint8_t> in{'H', 'e', 'l', 'l', 'o'};

    auto buf = modulation::to_constl<modulation::e16QAM>(in);
    auto res = modulation::from_constl<modulation::e16QAM>(buf);

    EXPECT_EQ(res, in);
}

TEST(ModulationTest, LLRCorrect)
{
    auto r = 0.6d - 1.4j;
    auto llr = modulation::e16QAM<double>{}.llr(r, 0.5);

    EXPECT_THAT(
        llr,
        Pointwise(
            testing::DoubleNear(1e-6),
            std::vector<double>{
                -19.41750979388585,
                -54.83501958777170,
                 0.82106723119178,
                15.17893276880822
            }
        )
    );
}

