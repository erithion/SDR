#include "fft.hpp"
#include <vector>
#include <complex>
#include <gtest/gtest.h>

using testing::AssertionResult;
using testing::AssertionSuccess;
using testing::AssertionFailure;

// Helper function to check approximate equality of std::complex<double>
AssertionResult ComplexNear(const std::complex<double>& exp, const std::complex<double>& act, double tolerance)
{
    return ( std::abs(exp.real() - act.real()) <= tolerance && std::abs(exp.imag() - act.imag()) <= tolerance )
           ? AssertionSuccess()
           : AssertionFailure()
               << "Complex numbers are not near enough."
               << "\n  Expected: " << exp
               << "\n  Actual:   " << act
               << "\n  Tolerance: " << tolerance
               << "\n  Delta real: " << std::abs(exp.real() - act.real())
               << "\n  Delta imag: " << std::abs(exp.imag() - act.imag());
}

// Helper function to check if two vectors of complex numbers are approximately equal
AssertionResult VectorsComplexNear( const std::vector<std::complex<double>>& exp
                                  , const std::vector<std::complex<double>>& seq
                                  , double tolerance ) 
{
    if (exp.size() != seq.size())
        return AssertionFailure()
               << "Vector sizes differ. Expected size " << exp.size()
               << ", Actual size " << seq.size();

    for (size_t i = 0; i < exp.size(); ++i)
    {
        // Use the ComplexNear helper for element-wise comparison
        AssertionResult res = ComplexNear(exp[i], seq[i], tolerance);
        if (!res)
            // Propagate the failure message with index information
            return AssertionFailure()
                   << "At index " << i << ": " << res.message();
    }

    return AssertionSuccess();
}

TEST(fft_test, ifft_restores_back_fft_transformed_sequence)
{
    std::vector<std::complex<double>> exp{0,1,2,3,4,5,6,7};
    std::vector<std::complex<double>> seq{0,1,2,3,4,5,6,7};

    fft::fft2(seq.begin(), seq.end());

    fft::ifft2(seq.begin(), seq.end());

    const double tolerance = 1e-9;

    // Use a custom assertion wrapper
    ASSERT_TRUE(VectorsComplexNear(seq, exp, tolerance));
}

TEST(fft_test, non_multiple_of_two_size_fails)
{
    std::vector<std::complex<double>> seq{0,1,2};

    ASSERT_FALSE((fft::fft2(seq.begin(), seq.end()).has_value()));
}