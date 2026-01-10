#include "fft.hpp"
#include "ofdm.hpp"
#include <format>
#include <complex>
#include <vector>
#include <print>
#include <ranges>

template <typename T, typename CharT>
struct std::formatter<std::complex<T>, CharT>
{
    // Parse the format specifiers (e.g., precision, etc.)
    constexpr auto parse(auto& ctx)
    {
        return ctx.begin();
    }

    // Format the complex number
    auto format(const std::complex<T>& c, auto& ctx) const
    {
        return std::format_to(ctx.out(), "({}+{}i)", c.real(), c.imag());
    }
};

template <typename T>
struct std::formatter<std::vector<T>> {
    constexpr auto parse(format_parse_context& ctx) { return ctx.begin(); }

    auto format(const std::vector<T>& v, format_context& ctx) const {
        auto out = ctx.out();
        out = std::format_to(out, "[");
        for (size_t i = 0; i < v.size(); ++i) {
            out = std::format_to(out, "{}{}", v[i], (i == v.size() - 1 ? "" : ", "));
        }
        return std::format_to(out, "]");
    }
};


#include <iostream>
#include <cstdio> // Required for popen
#include <iomanip>

void plot_constellation(const std::vector<std::complex<double>>& points) 
{
    FILE *gp = popen("gnuplot -persist", "w"); // Open a pipe to gnuplot

    if (gp == nullptr) {
        std::cerr << "Error opening pipe to Gnuplot. Make sure Gnuplot is installed and in PATH." << std::endl;
        return;
    }

    // Gnuplot commands to set up the plot
    fprintf(gp, "reset session\n");
    fprintf(gp, "set term qt size 600,600\n");
    fprintf(gp, "unset key\n");
    fprintf(gp, "unset multiplot\n");
    fprintf(gp, "unset logscale\n");
    fprintf(gp, "set size square\n");

    fprintf(gp, "set title 'OFDM 16-QAM Constellation'\n");
    fprintf(gp, "set xlabel 'In-phase (I)'\n");
    fprintf(gp, "set ylabel 'Quadrature (Q)'\n");
    fprintf(gp, "set grid\n");
    fprintf(gp, "set xrange [-3.5:3.5]\n");
    fprintf(gp, "set yrange [-3.5:3.5]\n");
    fprintf(gp, "plot '-' with points pt 6 ps 1 lc rgb \"blue\"\n");

    for (const auto& p : points)
        fprintf(gp, "%.6f %.6f\n", p.real(), p.imag());

    fprintf(gp, "e\n");
    fflush(gp);
    pclose(gp);
}

int main()
{
    std::vector<std::complex<double>> d{0,1,2,3,4,5,6,7};
    std::vector<uint8_t> in{ 0b00000001
                           , 0b00100011
                           , 0b01000101
                           , 0b01100111
                           , 0b10001001
                           , 0b10101011
                           , 0b11001101
                           , 0b11101111 };
    auto v = ofdm::to_constellations<ofdm::e16QAM>(in);
    plot_constellation(v);

    // std::println("{}\n", d);

    // fft::fft2(d.begin(), d.end());

    // std::println("{}\n", d);

    // fft::ifft2(d.begin(), d.end());

    // std::println("{}", d);

    auto r = ofdm::tx(d);
    if (r.has_value())
//        for (auto v: r.value()) std::print("{}", v);
        std::println("{}\n", r.value());
    else
        std::println("{}", r.error());
    return 0;
}
