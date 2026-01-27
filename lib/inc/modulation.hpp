#pragma once

#include <concepts>
#include <vector>
#include <complex>
#include <stdint.h>
#include <array>
#include <limits>
#include <cmath>

namespace modulation
{
    struct eBPSK{};    // 1bit/symbol
    struct eQPSK{};    // 2bits/symbol
    template <std::floating_point T>
    struct e16QAM
    {
        static constexpr size_t                             bits_per_symbol = 4;
        // Normalization factor for averaging symbol power to 1
        static constexpr T                                  norm = T{1} / std::sqrt(T{10});
        // Norm inverse, precomputed
        static constexpr T                                  inorm = std::sqrt(T{10});
        
        // Unnormalized Gray-coded 16-QAM where the adjacent symbols' binary indexes differ just by a single bit.
        // Thus, if noise falsely shifts the symbol closer to a neighbour, this flips just one bit.
        // Viterbi decoding is very efficient in recovering one error bit, hence the Bit Error Rate (BER) is better.
        static constexpr std::array<std::complex<T>, 16> table =
        {
            /* 0000 */ -3-3j,
            /* 0001 */ -3-1j,
            /* 0010 */ -3+3j,
            /* 0011 */ -3+1j,
            /* 0100 */ -1-3j,
            /* 0101 */ -1-1j,
            /* 0110 */ -1+3j,
            /* 0111 */ -1+1j,
            /* 1000 */ +3-3j,
            /* 1001 */ +3-1j,
            /* 1010 */ +3+3j,
            /* 1011 */ +3+1j,
            /* 1100 */ +1-3j,
            /* 1101 */ +1-1j,
            /* 1110 */ +1+3j,
            /* 1111 */ +1+1j
        };

        uint8_t nearest_bits(const std::complex<T>& constellation_point) noexcept
        {
            uint8_t res = 0;
            T       best = std::numeric_limits<T>::max();
            auto    unp = constellation_point * inorm;
            for (uint8_t i = 0; i != table.size(); ++i)
            {
                T d2 = std::norm(unp - table[i]);
                if (d2 < best)
                {
                    best = d2;
                    res = i;
                }
            }
            return res;
        }

        uint8_t likelihood_bits(const std::complex<T>& constellation_point, const T& s2) noexcept
        {
            auto llrs = llr(constellation_point, s2);
            uint8_t res = 0;
            for (uint8_t i = 0; i != bits_per_symbol; ++i)
            {
                auto v = llrs[i] > 0 ? 1 : 0;
                res |= (v << i);
            }
            return res;
        }

        // TODO(artem): consider separable I/Q once again for loopless llr()
        /**
         * @brief Given a constellation symbol and noise variance, returns log-likelihood ratio of zero/one bits.
         * 
         * @details Assume a received symbol R=S+N, where
         * . S - original constellation symbol
         * . N - white Gaussian noise
         * and the noise which due to the Central Limit Theorem follows a normal distribution
         * with zero mean and variance S2.
         * 
         * First, for each of 16 QAM symbols Si we calculate p(R|Si), i.e. how probable to receive R
         * if Si was sent. This probability is in fact PDF(R-S) of the normal distribution
         *              p(R∣Si)=(1/2πσ^2)​exp(−∣R−Si∣^2/2σ^2).
         * 
         * Defining log-likelihood ll[i]=log p(R|Si) and ignoring constants yields ll[i]=−∣R−Si∣^2/2σ^2.
         * 
         * Probability of a k-th bit be 0 given R is p(Bk​=0∣R)=∑​p(R∣Si)p(Si). I.e., summing up probabilities
         * of each Si where k-th bit is 0. Yet, log ∑​e^(-xi)​ ≈ max​ -xi, as exp(-x) decays quickly.
         * 
         * The same is for p(Bk​=1∣R).
         * 
         * So, log-likelihood ratio LLR[k] ​≈ ​max​(−∣r−s∣^2​/2σ^2)−​max​(−∣r−s∣^2/2σ2​).
         * 
         * In implementation we can drop minus and switch to min, take out the scalar and drop 2.
         * 
         * Finally, we get a 4 doubles Dk. Each Dk is how plausible 0 is at k-th bit:
         * . Dk < 0 - bit 1 is more likely
         * . Dk > 0 - bit 0 is more likely
         * 
         * @tparam T 
         * @param constellation_point Received constellation point with white noise inside
         * @param noise_variance Variance (sigma squared) of the white Gaussian noise
         * @return std::vector<T> 4 floating LLR. If k-th element of LLR is
         * . < 0, then 1 is more likely at k-th bit
         * . > 0, then 0 is more likely at k-th bit
         */
        std::array<T, bits_per_symbol> llr(const std::complex<T>& r, const T& s2)
        {
            std::array<T, table.size()>     ll{};
            auto unp = r * inorm;

            for (size_t i = 0; i < table.size(); ++i)
                ll[i] = std::norm(unp - table[i]); // distance |R-S[i]|^2
            
            std::array<T, bits_per_symbol>  ret{};
            for (int i = 1, k = 0; i != (1 << bits_per_symbol); i <<= 1, ++k)
            {
                T b0 = std::numeric_limits<T>::max(), b1 = std::numeric_limits<T>::max();
                for (auto j = 0; j != ll.size(); ++j)
                    i & j ? b1 = std::min(b1, ll[j]) : b0 = std::min(b0, ll[j]);
                ret[k] = (b0 - b1) / s2;
            }                    
            return ret;
        }
    };

    struct e64QAM{};   // 6bits/symbol, LTE/Cable TV
    struct e256QAM{};  // 8bits/symbol, 5G
    struct e1024QAM{}; //

    /**
     * 
     * @param in A sequence of 4-bit packed data
     */
    template <template <typename> class Mod, typename T = double>
    std::vector<std::complex<T>> to_constl(const std::vector<uint8_t>& in, Mod<T> m = {})
        requires std::same_as<Mod<T>, e16QAM<T>> // Specialization/overload for 16-QAM
    {
        std::vector<std::complex<T>> out;
        out.reserve(in.size() * 2);
    
        for (uint8_t v: in)
        {
            uint8_t msb = (v >> 4) & 0xF;
            uint8_t lsb = v & 0xF;
        
            out.push_back(m.table[msb] * m.norm);
            out.push_back(m.table[lsb] * m.norm);
        }
        return out;
    }

    template <template <typename> class Mod, typename T = double>
    std::vector<uint8_t> from_constl(const std::vector<std::complex<T>>& in, Mod<T> m = {})
        requires std::same_as<Mod<T>, e16QAM<T>> // Specialization/overload for 16-QAM
    {
        std::vector<uint8_t> out;
        out.reserve(in.size() / 2);

        for (size_t i = 0; i + 1 < in.size(); i += 2)
        {
            uint8_t msb_sym = m.nearest_bits(in[i]);
            uint8_t lsb_sym = m.nearest_bits(in[i + 1]);
            // Pack two 4-bit symbols into one byte
            out.push_back((msb_sym << 4) | (lsb_sym & 0xF));
        }

        return out;
    }

    template <template <typename> class Mod, typename T = double>
    std::vector<uint8_t> from_constl_llr(const std::vector<std::complex<T>>& in, const T& sigma2, Mod<T> m = {})
        requires std::same_as<Mod<T>, e16QAM<T>> // Specialization/overload for 16-QAM
    {
        std::vector<uint8_t> out;
        out.reserve(in.size() / 2);

        for (size_t i = 0; i + 1 < in.size(); i += 2)
        {
            uint8_t msb_sym = m.likelihood_bits(in[i], sigma2);
            uint8_t lsb_sym = m.likelihood_bits(in[i + 1], sigma2);
            // Pack two 4-bit symbols into one byte
            out.push_back((msb_sym << 4) | (lsb_sym & 0xF));
        }

        return out;
    }
}