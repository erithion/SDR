#include "fft.hpp"
#include <gtest/gtest.h>
#include <vector>

#include <gtest/gtest.h>
#include <complex>
#include <vector>
#include <cmath>
#include <iomanip>
#include <numeric>

// Helper function to check approximate equality of std::complex<double>
::testing::AssertionResult ComplexNear(const std::complex<double>& expected,
                                       const std::complex<double>& actual,
                                       double abs_error) {
    if (std::abs(expected.real() - actual.real()) <= abs_error &&
        std::abs(expected.imag() - actual.imag()) <= abs_error) {
        return ::testing::AssertionSuccess();
    } else {
        return ::testing::AssertionFailure()
               << "Complex numbers are not near enough."
               << "\n  Expected: " << expected
               << "\n  Actual:   " << actual
               << "\n  Tolerance: " << abs_error
               << "\n  Delta real: " << std::abs(expected.real() - actual.real())
               << "\n  Delta imag: " << std::abs(expected.imag() - actual.imag());
    }
}

// Helper function to check if two vectors of complex numbers are approximately equal
::testing::AssertionResult VectorsComplexNear(const std::vector<std::complex<double>>& expected,
                                              const std::vector<std::complex<double>>& actual,
                                              double abs_error) {
    if (expected.size() != actual.size()) {
        return ::testing::AssertionFailure()
               << "Vector sizes differ. Expected size " << expected.size()
               << ", Actual size " << actual.size();
    }

    for (size_t i = 0; i < expected.size(); ++i) {
        // Use the ComplexNear helper for element-wise comparison
        ::testing::AssertionResult res = ComplexNear(expected[i], actual[i], abs_error);
        if (!res) {
            // Propagate the failure message with index information
            return ::testing::AssertionFailure()
                   << "At index " << i << ": " << res.message();
        }
    }

    return ::testing::AssertionSuccess();
}

TEST(fft_test, simple_array)
{
    std::vector<std::complex<double>> truth{0,1,2,3,4,5,6,7};
    std::vector<std::complex<double>> sequence{0,1,2,3,4,5,6,7};

    fft::fft2(sequence.begin(), sequence.end());

    fft::ifft2(sequence.begin(), sequence.end());

    const double tolerance = 1e-9;

    // Use a custom assertion wrapper
    ASSERT_TRUE(VectorsComplexNear(sequence, truth, tolerance));
}