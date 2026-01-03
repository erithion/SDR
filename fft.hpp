#pragma once

#include <stdint.h>
#include <iterator>
#include <complex>
#include <type_traits>
#include <concepts>

namespace fft::detail
{
    /**
     * @brief Permutes elements of a random access container according to their indexes reversed.
     * I.e.
     * 
     * value    index (dec/bin)   ---becomes--->   reversed index (dec/bin)   value
     *  0           0/000                                   0/000               0
     *  1           1/001                                   4/100               4
     *  2           2/010                                   2/010               2
     *  3           3/011                                   6/110               6
     *  4           4/100                                   1/001               1
     *  5           5/101                                   5/101               5
     *  6           6/110                                   3/011               3
     *  7           7/111                                   7/111               7
     * 
     * Thus, an array {0,1,2,3,4,5,6,7} becomes {0,4,2,6,1,5,3,7}. Why does it matter?
     * Because (0,4) (2,6) (1,5) (3,7) are the pairs that FFT ends up processing in its leafs of sequences of size 2
     * "down the stack" before it starts building the whole sequence up. By reversing the order, one guarantees
     * sequential element access during FFT, hence a boost in speed, as this kind of access is highly cache-friendly.
     * @tparam It Any container possessing a random access iterator trait
     * @param begin A start iterator to the underlying data
     * @param end An end iterator of the underlying data
     */
    template <std::random_access_iterator It>
    void bit_reverse_permute(It& begin, size_t size)
    {
        for (std::size_t i = 1, j = 0; i < size; ++i)
        {
            // Always start from MSB in order to start shifting it towards LSB
            std::size_t bit = size >> 1;
            while (j & bit) // current bit is set ...
            {
                j ^= bit; // ... hence must be reset...
                bit >>= 1; // ... and off we go to check the next LSB.
            }
            j |= bit; // Residual in bit is LSB that can be left-appended to j

            if (i < j)
                std::swap(*(begin + i), *(begin + j));
        }
    }

    template <typename T>
    struct is_complex_t : public std::false_type {};

    template <typename T>
    struct is_complex_t<std::complex<T>> : public std::true_type {};

    template <typename T>
    concept is_complex = is_complex_t<T>::value;

    template <typename It>
    concept fft_compatible_iterator = std::random_access_iterator<It> && detail::is_complex<std::iter_value_t<It>>;

    template <fft_compatible_iterator It>
    void cooley_tukey_iterative_fft(It& begin, It& end, int8_t sign = 1)
    {
        // TODO: check multiples of 2
        const size_t size = std::distance(begin, end);
        bit_reverse_permute(begin, size);
        for (size_t N = 2; N <= size; N <<= 1) // to avoid std::log(size) we just move by powers of 2
        {
            const auto unity_root = std::polar(1.0, -2 * sign * std::numbers::pi / N);
            // Parallelize independent blocks
            #pragma omp parallel for
            for (size_t i = 0; i < size; i += N)
            {
                auto z = std::polar(1.0, 0.0);
                for (size_t j = 0; j < N / 2; ++j)
                {
                    auto even = *(begin + i + j);
                    auto odd = *(begin + i + j + N / 2) * z;
                    *(begin + i + j) = even + odd;
                    *(begin + i + j + N / 2) = even - odd;
                    z *= unity_root;
                }
            }
        }
    }
}

namespace fft
{
    template <typename It>
    concept fft_compatible_iterator = detail::fft_compatible_iterator<It>;

    template <fft_compatible_iterator It>
    void fft2(It&& begin, It&& end)
    {
        detail::cooley_tukey_iterative_fft(begin, end);
    }
    template <fft_compatible_iterator It>
    void ifft2(It&& begin, It&& end)
    {
        detail::cooley_tukey_iterative_fft(begin, end, -1);

        const auto N = std::distance(begin, end);
        for (auto& i = begin; i != end; ++i)
            *i /= N;
    }
}