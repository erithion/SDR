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
    struct e16QAM
    {
        static constexpr size_t                             bits_per_symbol = 4;
        // Normalization factor for averaging symbol power to 1
        template <typename T>
        static constexpr T                                  norm = T{1} / std::sqrt(T{10});
        // Norm inverse, precomputed
        template <typename T>
        static constexpr T                                  inorm = std::sqrt(T{10});
        
        // Unnormalized Gray-coded 16-QAM where the adjacent symbols' binary indexes differ just by a single bit.
        // Thus, if noise falsely shifts the symbol closer to a neighbour, this flips just one bit.
        // Viterbi decoding is very efficient in recovering one error bit, hence the Bit Error Rate (BER) is better.
        template <typename T>
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

        template <typename T>
        static uint8_t nearest(const std::complex<T>& pt) noexcept
        {
            uint8_t res = 0;
            T       best = std::numeric_limits<T>::max();
            auto    unp = pt * inorm<T>;
            for (uint8_t i = 0; i != table<T>.size(); ++i)
            {
                T dx = unp.real() - table<T>[i].real();
                T dy = unp.imag() - table<T>[i].imag();
                T d2 = dx*dx + dy*dy;
                if (d2 < best)
                {
                    best = d2;
                    res = i;
                }
            }
            return res;
        }

        template <typename T = double>
        std::vector<T> likelihood(const std::complex<T>& constellation_point, T noise_variance)
        {
            std::array<T, table<T>.size()> ll = {};
            for (auto i = 0; i != table<T>.size(); ++i)
                ll[i] = std::norm(constellation_point - table<T>[i]); // distance |R-S[i]|^2
            
            std::vector<T>                 llr(bits_per_symbol, 0);
            for (int i = 1, k = 0; i != (1 << bits_per_symbol); i <<= 1, ++k)
            {
                T b0, b1;
                b0 = b1 = std::numeric_limits<T>::max();
                for (auto j = 0; j != ll.size(); ++j)
                    if (i & j) b1 = std::min(b1, ll[j]);
                    else b0 = std::min(b0, ll[j]);
                llr[k] = (b0 - b1) / noise_variance;
            }                    
            return llr;
        }
    };

    struct e64QAM{};   // 6bits/symbol, LTE/Cable TV
    struct e256QAM{};  // 8bits/symbol, 5G
    struct e1024QAM{}; //

    /**
     * 
     * @param in A sequence of 4-bit packed data
     */
    template <typename Mod, typename T = double>
    std::vector<std::complex<T>> to_constl(const std::vector<uint8_t>& in, Mod m = {})
        requires std::same_as<Mod, e16QAM> // Specialization/overload for 16-QAM
    {
        std::vector<std::complex<T>> out;
        out.reserve(in.size() * 2);
    
        for (uint8_t v: in)
        {
            uint8_t msb = (v >> 4) & 0xF;
            uint8_t lsb = v & 0xF;
        
            out.push_back(Mod::template table<T>[msb] * Mod::template norm<T>);
            out.push_back(Mod::template table<T>[lsb] * Mod::template norm<T>);
        }
        return out;
    }


    template <typename Mod, typename T = double>
    std::vector<uint8_t> from_constl(const std::vector<std::complex<T>>& in, Mod m = {})
        requires std::same_as<Mod, e16QAM> // Specialization/overload for 16-QAM
    {
        std::vector<uint8_t> out;
        out.reserve(in.size() / 2);

        for (size_t i = 0; i + 1 < in.size(); i += 2)
        {
            uint8_t msb_sym = Mod::template nearest<T>(in[i]);
            uint8_t lsb_sym = Mod::template nearest<T>(in[i + 1]);
            // Pack two 4-bit symbols into one byte
            out.push_back((msb_sym << 4) | (lsb_sym & 0xF));
        }

        return out;
    }
}