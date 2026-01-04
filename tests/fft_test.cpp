#include "fft.hpp"
#include <gtest/gtest.h>
#include <vector>

#include <gtest/gtest.h>
#include <complex>
#include <vector>
#include <cmath>
#include <iomanip>
#include <numeric>

using testing::AssertionResult;
using testing::AssertionSuccess;
using testing::AssertionFailure;

// Helper function to check approximate equality of std::complex<double>
AssertionResult ComplexNear(const std::complex<double>& exp, const std::complex<double>& act, double abs_error)
{
    return ( std::abs(exp.real() - act.real()) <= abs_error && std::abs(exp.imag() - act.imag()) <= abs_error )
           ? AssertionSuccess()
           : AssertionFailure()
               << "Complex numbers are not near enough."
               << "\n  Expected: " << exp
               << "\n  Actual:   " << act
               << "\n  Tolerance: " << abs_error
               << "\n  Delta real: " << std::abs(exp.real() - act.real())
               << "\n  Delta imag: " << std::abs(exp.imag() - act.imag());
}

// Helper function to check if two vectors of complex numbers are approximately equal
AssertionResult VectorsComplexNear( const std::vector<std::complex<double>>& expected
                                  , const std::vector<std::complex<double>>& actual
                                  , double abs_error ) 
{
    if (expected.size() != actual.size())
        return AssertionFailure()
               << "Vector sizes differ. Expected size " << expected.size()
               << ", Actual size " << actual.size();

    for (size_t i = 0; i < expected.size(); ++i)
    {
        // Use the ComplexNear helper for element-wise comparison
        AssertionResult res = ComplexNear(expected[i], actual[i], abs_error);
        if (!res)
            // Propagate the failure message with index information
            return AssertionFailure()
                   << "At index " << i << ": " << res.message();
    }

    return AssertionSuccess();
}

TEST(fft_test, ifft_restores_back_fft_transformed_sequence)
{
    std::vector<std::complex<double>> truth{0,1,2,3,4,5,6,7};
    std::vector<std::complex<double>> sequence{0,1,2,3,4,5,6,7};

    fft::fft2(sequence.begin(), sequence.end());

    fft::ifft2(sequence.begin(), sequence.end());

    const double tolerance = 1e-9;

    // Use a custom assertion wrapper
    ASSERT_TRUE(VectorsComplexNear(sequence, truth, tolerance));
}

TEST(fft_test, non_multiple_of_two_size_fails)
{
    std::vector<std::complex<double>> sequence{0,1,2};

    ASSERT_FALSE((fft::fft2(sequence.begin(), sequence.end()).has_value()));
}