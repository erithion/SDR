#include "modulation.hpp"
#include "ofdm.hpp"
#include <vector>
#include <complex>
#include <gtest/gtest.h>
#include <gmock/gmock.h>

using ::testing::Pointwise;
using ::testing::Truly;

TEST(OFDMTest, TransformsSequenceForthAndBackCorrectly)
{
    std::vector<std::complex<double>> ref{0+0j,1+1j,2+2j,-1-1j,-2-2j,-3-3j,1-1j,-1+1j};
    std::vector<std::complex<double>> buf;

    ofdm::tx(ref, 8, buf);

    std::vector<std::complex<double>> res;
    ofdm::rx(buf, 8, res);

    EXPECT_THAT(res, Pointwise(Truly([](const auto& pair)
    {
        const auto& [a, b] = pair;
        return std::abs(a - b) < 1e-9;
    }), ref));
}

