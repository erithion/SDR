#pragma once

#include <concepts>
#include <expected>
#include <vector>
#include <complex>
#include <stdint.h>
#include <array>
#include <format>

namespace modulation
{
    struct eBPSK{};    // 1bit/symbol
    struct eQPSK{};    // 2bits/symbol
    struct e16QAM
    {
        static constexpr size_t bits_per_symbol = 4;

        template <typename T>
        static constexpr std::array<std::pair<std::complex<T>, uint8_t>, 16> table = 
        {{ {-3-3j, 0x0}, {-3-1j, 0x1}, {-3+1j, 0x2}, {-3+3j, 0x3}
        ,  {-1-3j, 0x4}, {-1-1j, 0x5}, {-1+1j, 0x6}, {-1+3j, 0x7}
        ,  {+1-3j, 0x8}, {+1-1j, 0x9}, {+1+1j, 0xA}, {+1+3j, 0xB}
        ,  {+3-3j, 0xC}, {+3-1j, 0xD}, {+3+1j, 0xE}, {+3+3j, 0xF} }};

        template <typename T>
        static std::expected<uint8_t, std::string> nearest(const std::complex<T>& pt)
        {
            uint8_t res = 0;
            T       best = std::numeric_limits<T>::max();
            for (const auto& [cpx, bits]: table<T>)
            {
                T dx = pt.real() - cpx.real();
                T dy = pt.imag() - cpx.imag();
                T d2 = dx*dx + dy*dy;
                if (d2 < best)
                {
                    best = d2;
                    res = bits;
                }
            }
            if (best == std::numeric_limits<T>::max()) 
                return std::unexpected(std::format( "Nearest not found for Re{{pt}}={}, Im{{pt}}={} in 16-QAM"
                                                  , pt.real(), pt.imag() )); // Too lazy to define complex formatter
            return res;
        }
    };

    struct e64QAM{};   // 6bits/symbol, LTE/Cable TV
    struct e256QAM{};  // 8bits/symbol, 5G
    struct e1024QAM{}; //

    namespace detail
    {
        template <typename T>
        std::vector<std::complex<T>> gray_coded_mapper(const std::vector<uint8_t>& in, e16QAM)
        {
           static constexpr std::array<std::complex<T>, 16> tbl =
           { -3-3j, -3-1j, -3+1j, -3+3j
           , -1-3j, -1-1j, -1+1j, -1+3j
           , 1-3j,  1-1j,  1+1j,  1+3j
           , 3-3j,  3-1j,  3+1j,  3+3j };
        
            std::vector<std::complex<T>> out;
            out.reserve(in.size() * 2);
        
            for (uint8_t v : in)
            {
                uint8_t msb = (v >> 4) & 0xF;
                uint8_t lsb = v & 0xF;
            
                out.push_back(tbl[msb]);
                out.push_back(tbl[lsb]);
            }
            return out;
        }
    }

    template <typename Mod, typename T = double>
    std::vector<std::complex<T>> to_constl(const std::vector<uint8_t>& in, Mod m = {})
    {
        return detail::gray_coded_mapper<T>(in, m);
    }

    template <typename Mod, typename T = double>
    std::vector<uint8_t> from_constl(const std::vector<std::complex<T>>& in, Mod m = {})
    {
        std::vector<uint8_t> out;
        out.reserve(in.size() / 2);

        for (size_t i = 0; i + 1 < in.size(); i += 2)
        {
            // TODO(artem): don't ignore errors
            uint8_t msb_sym = Mod::template nearest<T>(in[i]).value_or(0);
            uint8_t lsb_sym = Mod::template nearest<T>(in[i + 1]).value_or(0);
            // Pack two 4-bit symbols into one byte
            out.push_back((msb_sym << 4) | (lsb_sym & 0xF));
        }

        return out;
    }
}