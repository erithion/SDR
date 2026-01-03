#pragma once

#include <stdint.h>
#include <iterator>
#include <complex>

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
    void bit_reverse_permute(It& begin, It& end)
    {
        std::size_t size = std::distance(begin, end);
        std::size_t j = 0;

        for (std::size_t i = 1; i < size; ++i)
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

    template <std::random_access_iterator It>
    void cooley_tukey_iterative_fft(It&& begin, It&& end, int8_t sign = 1)
    {
        static const auto z = std::polar(1.0, 0.0);
        // TODO: check multiples of 2
        bit_reverse_permute(begin, end);
    //            const auto  unityRoot = std::polar(1.0, -2 * sign * std::numbers::pi / seq.size());

    }
}

namespace fft
{
    template <std::random_access_iterator It>
    void fft2(It&& begin, It&& end)
    {
        detail::cooley_tukey_iterative_fft(std::forward<It>(begin), std::forward<It>(end));
    }
}