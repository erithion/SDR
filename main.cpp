#include "fft.hpp"
#include <format>
#include <complex>
#include <omp.h>
#include <vector>
#include <print>
#include <iostream>

template <typename T, typename CharT>
struct std::formatter<std::complex<T>, CharT> {
    // Parse the format specifiers (e.g., precision, etc.)
    constexpr auto parse(auto& ctx) {
        return ctx.begin();
    }

    // Format the complex number
    auto format(const std::complex<T>& c, auto& ctx) const {
        return std::format_to(ctx.out(), "({}+{}i)", c.real(), c.imag());
    }
};

int main()
{
    std::vector<std::complex<double>> d{0,1,2,3,4,5,6,7};

    std::println("{}\n", d);

    fft::fft2(d.begin(), d.end());

    std::println("{}\n", d);

    fft::ifft2(d.begin(), d.end());

    std::println("{}", d);

    #pragma omp parallel
    {
        printf("Hello World from thread = %d\n", omp_get_thread_num());
    }
    return 0;
}
