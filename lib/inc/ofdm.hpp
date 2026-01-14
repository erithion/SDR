#pragma once
// 3.2 microsec symbol length
// I.e. each carrier is deltaF=1/3.2 microsec=312.5kHz apart
#include "fft.hpp"
#include <concepts>
#include <expected>
#include <vector>
#include <complex>
#include <stdint.h>
#include <array>

namespace ofdm
{
    struct eBPSK{};    // 1bit/symbol
    struct eQPSK{};    // 2bits/symbol
    struct e16QAM{};   // 4bits/symbol, WiFi/cable
    struct e64QAM{};   // 6bits/symbol, LTE/Cable TV
    struct e256QAM{};  // 8bits/symbol, 5G
    struct e1024QAM{}; //

    namespace detail
    {
        template <typename T>
        std::vector<std::complex<T>> gray_coded_mapper(const std::vector<uint8_t>& in, e16QAM)
        {
            static constexpr std::array<std::complex<T>, 16> tbl = 
            { -3+3j, -1+3j, +1+3j, +3+3j
            , -3+1j, -1+1j, +1+1j, +3+1j
            , -3-3j, -1-3j, +1-3j, +3-3j
            , -3-1j, -1-1j, +1-1j, +3-1j };
        
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
    std::vector<std::complex<T>> to_constellations(const std::vector<uint8_t>& in, Mod m = {})
    {
        return detail::gray_coded_mapper<T>(in, m);
    }

    template <std::floating_point T>
    std::expected<void, std::string> tx(const std::vector<std::complex<T>>& in, size_t cp_size, std::vector<std::complex<T>>& out)
    {
        out.resize(in.size() + cp_size);
        std::copy(in.begin(), in.end(), out.begin() + cp_size);
        return fft::ifft2(out.begin() + cp_size, out.end())
            .and_then([&out, cp_size]() -> std::expected<void, std::string>
            {
                std::copy(out.end() - cp_size, out.end(), out.begin());
                return {};
            });
    }

    template <std::floating_point T>
    std::expected<std::vector<std::complex<T>>, std::string> tx(const std::vector<std::complex<T>>& in, size_t cp_size)
    {
        std::vector<std::complex<T>> out;
        return tx(in, cp_size, out)
            .transform([&out]()
            {
                return out;
            });
    }
}