#pragma once

#include <stdint.h>
#include <iterator>
#include <complex>
#include <numbers>
#include <type_traits>
#include <concepts>
#include <expected>

namespace fft::detail
{
    /**
     * @brief Permutes elements of a container according to their reversed indexes.
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
     * Because (0,4) (2,6) (1,5) (3,7) are the pairs that FFT ends up processing in its leafs "down the stack"
     * before it starts building the sequence up again. By reversing the order, one ensures a sequential element access,
     * hence a boost in speed, as this is cache-friendly for CPU.
     * @tparam It Any container possessing a random access iterator trait.
     * @param begin A start iterator to the underlying data.
     * @param size Size of the underlying data.
     */
    template <std::random_access_iterator It>
    void bit_reverse_permute(It begin, size_t size)
    {
        for (std::size_t i = 1, j = 0; i < size; ++i)
        {
            std::size_t zip = size >> 1; // Always start from the next to MSB: zip = ox*****
            while (j & zip) // Assume a number of iterations. The current bit is set: oooxooo
            {
                j ^= zip; // Hence the bit must be reset: j = ***o***
                zip >>= 1; // Off we go to the next LSB: zip = oooox**
            }
            j |= zip; // Ultimately as if +1 but bitwise reversed: j = ****x**

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

    /**
     * @brief Checks whether an iterator 
     * - belongs to a container possessing the random access trait
     * - its underlying type is std::complex
     * 
     * @tparam It An iterator type
     */
    template <typename It>
    concept fft_compatible_iterator = std::random_access_iterator<It>
                                   && detail::is_complex<std::iter_value_t<It>>;

    /**
     * @brief Iterative (I)FFT based on Cooley-Tukey Radix-2 Decimation-In-Time algorithm.
     * The principal difference from the FFT recursive is in re-arranging the sequence in contiguous
     * independent ranges that are CPU cache-friendly, and processing them in parallel using OpenMP.
     * 
     * @tparam ParallelThreshold Parallelize when the sequence size / 2 exceeds this number.
     * @tparam It An iterator type of a random access container with a std::complex underlying type.
     * @param begin A sequence start iterator.
     * @param end A sequence end iterator.
     * @param sign true for non-scaled IFFT of the sequence, otherwise false for FFT.
     * @return std::expected<void, std::string> 
     * - Nothing in case of success;
     * - A string of error in case of failure.
     */
    template <size_t ParallelThreshold, fft_compatible_iterator It>
    std::expected<void, std::string> cooley_tukey_iterative_fft(It begin, It end, bool inverse = false)
    {
        const size_t size = std::distance(begin, end);
        if (size > 0 && ((size & (size - 1)) != 0)) // Must be of powers of 2 size
            return std::unexpected("The sequence size must be of powers of 2");

        bit_reverse_permute(begin, size);
        for (size_t N = 2; N <= size; N <<= 1) // Avoiding std::log(size), just move by powers of 2
        {
/* Apparently, this version accumulates FP error
            const auto unity_root = std::polar(1.0, -2 * (inverse ? -1 : 1) * std::numbers::pi / N);
            #pragma omp parallel for if(size / N > ParallelThreshold) // Parallelize independent blocks
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
*/
            using floating = typename std::iter_value_t<It>::value_type; // double or float from std::complex
            const floating theta = -2.0 * (inverse ? -1.0 : 1.0) * std::numbers::pi / N;
            const floating cos = std::cos(theta);
            const floating sin = std::sin(theta);

            // TODO(artem): consider SIMD improvements
            #pragma omp parallel for if(size / N > ParallelWhenSizeIsLargerThan)
            for (size_t i = 0; i < size; i += N)
            {
                floating wr = 1.0;
                floating wi = 0.0;
            
                for (size_t j = 0; j < N / 2; ++j)
                {
                    const auto even = *(begin + i + j);
                    const auto odd  = *(begin + i + j + N / 2);
                
                    const std::complex<floating> t
                    {
                        odd.real() * wr - odd.imag() * wi,
                        odd.real() * wi + odd.imag() * wr
                    };
                
                    *(begin + i + j)         = even + t;
                    *(begin + i + j + N / 2) = even - t;
                
                    // Twiddle recurrence
                    const floating tmp = wr;
                    wr = tmp * cos - wi * sin;
                    wi = tmp * sin + wi * cos;
                
                    // Optional renormalization (cheap safety net)
                    if ((j & 31) == 0)
                    {
                        const floating mag = std::hypot(wr, wi);
                        wr /= mag;
                        wi /= mag;
                    }
                }
            }
        }
        return {};
    }
}

// TODO(artem): add FFTW lib as a seamless alternative
namespace fft
{
    template <typename It>
    concept fft_compatible_iterator = detail::fft_compatible_iterator<It>;

    /**
     * @brief Performs FFT of the sequence.
     * Attempts to be efficient employing parallelization in the iterative Cooley-Tukey FFT algorithm.
     * 
     * @tparam ParallelThreshold Parallelize when the sequence size / 2 exceeds this number.
     * @tparam It An iterator type of a random access container with a std::complex underlying type.
     * @param begin A sequence begin iterator.
     * @param end A sequence end iterator.
     * @return std::expected<void, std::string> 
     * - Nothing on success;
     * - Error string on failure.
     */
    template <size_t ParallelThreshold = 1024, fft_compatible_iterator It>
    std::expected<void, std::string> fft2(It begin, It end)
    {
        return detail::cooley_tukey_iterative_fft<ParallelThreshold>(begin, end);
    }

    /**
     * @brief Performs IFFT of the sequence.
     * Attempts to be efficient employing parallelization in the iterative Cooley-Tukey FFT algorithm.
     * 
     * @tparam ParallelThreshold Parallelize when the sequence size / 2 exceeds this number.
     * @tparam It An iterator type of a random access container with a std::complex underlying type.
     * @param begin A sequence begin iterator.
     * @param end A sequence end iterator.
     * @return std::expected<void, std::string> 
     * - Nothing on success;
     * - Error string on failure.
     */
    template <size_t ParallelThreshold = 1024, fft_compatible_iterator It>
    std::expected<void, std::string> ifft2(It begin, It end)
    {
        return detail::cooley_tukey_iterative_fft<ParallelThreshold>(begin, end, true)
            .and_then([begin, end]() -> std::expected<void, std::string>
            {
                const auto N = std::distance(begin, end);
                for (auto it = begin; it != end; ++it)
                    *it /= N;
                return {};
            }); // monadic action on success: scaling down and resetting return to void
    }
}