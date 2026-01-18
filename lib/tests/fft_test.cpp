#include "fft.hpp"
#include <vector>
#include <complex>
#include <gtest/gtest.h>
#include <gmock/gmock.h>

using ::testing::Pointwise;
using ::testing::Truly;

TEST(FFTTest, TransformsSequenceForthAndBackCorrectly)
{
    std::vector<std::complex<double>> ref{0,1,2,3,4,5,6,7};
    std::vector<std::complex<double>> seq{0,1,2,3,4,5,6,7};

    fft::fft2(seq.begin(), seq.end());
    fft::ifft2(seq.begin(), seq.end());

    EXPECT_THAT(seq, Pointwise(Truly([](const auto& pair)
    {
        const auto& [a, b] = pair;
        return std::abs(a - b) < 1e-9;
    }), ref));
}

TEST(FFTTest, FftIfftForDouble)
{
    std::vector<std::complex<double>> ref{0,1,2,3,4,5,6,7};
    std::vector<std::complex<double>> seq{0,1,2,3,4,5,6,7};

    fft::fft2(seq.begin(), seq.end());
    fft::ifft2(seq.begin(), seq.end());

    EXPECT_THAT(seq, Pointwise(Truly([](const auto& pair)
    {
        const auto& [a, b] = pair;
        return std::abs(a - b) < 1e-9;
    }), ref));
}

TEST(FFTTest, FftIfftForFloat)
{
    std::vector<std::complex<float>> ref{0,1,2,3,4,5,6,7};
    std::vector<std::complex<float>> seq{0,1,2,3,4,5,6,7};

    fft::fft2(seq.begin(), seq.end());
    fft::ifft2(seq.begin(), seq.end());

    EXPECT_THAT(seq, Pointwise(Truly([](const std::tuple<std::complex<float>,std::complex<float>>& p)
    {
        constexpr float abs_eps = 1e-5f;
        constexpr float rel_eps = 1e-6f;

        const auto& [a, b] = p;

        auto close = [&](float x, float y)
        {
            float d = std::abs(x - y);
            float m = std::max(std::abs(x), std::abs(y));
            return d <= abs_eps || d <= rel_eps * m;
        };

        return close(a.real(), b.real()) &&
               close(a.imag(), b.imag());
    }), ref));
}

TEST(FFTTest, SizeNonMultipleOf2Fails)
{
    std::vector<std::complex<double>> seq{0,1,2};

    ASSERT_FALSE((fft::fft2(seq.begin(), seq.end()).has_value()));
}