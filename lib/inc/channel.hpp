#pragma once

#include <random>
#include <complex>
#include <concepts>

/**
 * @brief The header models/sumulates a channel the signal is transmitted through
 * 
 */
namespace channel
{
    /**
     * @brief Additive White Gaussian Noise (with zero mean)
     * 
     */
    template <std::floating_point T>
    struct awgn
    {
        /**
         * @brief Construct a new awgn object
         * 
         * @param snr_db Signal-to-Noise Ratio in decibels
         * @param seed 
         */
        awgn(T snr_db, uint32_t seed = std::random_device{}())
            : snr_{std::pow(T{10}, snr_db / T{10})}
            , gen_{seed}
            , gauss_{0, 1} {}

        void snr(T snr_db)
        {
            snr_ = std::pow(T{10}, snr_db / T{10});
        }

        // TODO(artem): come up with a better design for out_noise. range by demand, maybe
        /**
         * @brief Adds AWGN to the signal.
         * 
         * @param s Signal 
         * @param is_normalized true if signal power is 1, i.e. the variance 
         * @return std::vector<std::complex<T>> of the noise added to s
         */
        std::vector<std::complex<T>> apply(std::vector<std::complex<T>>& s, bool is_normalized = false)
        {
            if (s.empty()) return {};

            T signal_power = 1;
            if (!is_normalized) // nothing to compute if power=1
            {
                signal_power = std::accumulate(s.begin(), s.end(), 0, [](T acc, const std::complex<T>& z)
                {
                    return acc + std::norm(z);
                });
                signal_power /= s.size();
            }

            T noise_power = signal_power / (2 * snr_); // it is also noise variance, or sigma^2

            // Gaussian White Noise is characterized by the standard deviation
            gauss_.param(typename std::normal_distribution<T>::param_type(0, std::sqrt(noise_power)));

            std::vector<std::complex<T>> out;
            out.reserve(s.size());

            for (auto& v: s)
            {
                std::complex<T> noise_w(gauss_(gen_), gauss_(gen_));
                out.push_back(noise_w);
                v += noise_w;
            }
            return out;
        }

    private:
        T                           snr_;
        std::mt19937                gen_;
        std::normal_distribution<T> gauss_;
    };

    // Deduction guide
    template <std::floating_point T>
    awgn(T) -> awgn<T>; 

    template <typename T>
    struct model
    {};
}