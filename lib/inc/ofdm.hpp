#pragma once
// 3.2 microsec symbol length
// I.e. each carrier is deltaF=1/3.2 microsec=312.5kHz apart
#include "fft.hpp"
#include <concepts>
#include <expected>
#include <vector>
#include <complex>
#include <stdint.h>

namespace ofdm
{
    template <std::floating_point T>
    std::expected<void, std::string> tx(const std::vector<std::complex<T>>& in, size_t cp_size, std::vector<std::complex<T>>& out)
    {
        out.resize(in.size() + cp_size);
        std::copy(in.begin(), in.end(), out.begin() + cp_size);
        return fft::ifft2(out.begin() + cp_size, out.end())
            .and_then([&out, cp_size]() -> std::expected<void, std::string>
            {
                std::copy(out.end() - cp_size, out.end(), out.begin()); // guarding the start with a cyclic prefix
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

    template <std::floating_point T>
    std::expected<void, std::string> rx(const std::vector<std::complex<T>>& in, size_t cp_size, std::vector<std::complex<T>>& out)
    {
        out.resize(in.size() - cp_size);
        std::copy(in.begin() + cp_size, in.end(), out.begin()); // throwing the cyclic prefix away
        return fft::fft2(out.begin(), out.end());
    }

    template <std::floating_point T>
    std::expected<std::vector<std::complex<T>>, std::string> rx(const std::vector<std::complex<T>>& in, size_t cp_size)
    {
        std::vector<std::complex<T>> out;
        return rx(in, cp_size, out)
            .transform([&out]()
            {
                return out;
            });
    }
}