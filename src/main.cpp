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

template <std::floating_point T>
void plot_fft(const std::vector<std::complex<T>>& fft)
{
    FILE* pipe = popen("gnuplot -persist", "w"); // Use popen on Linux/macOS

    if (!pipe) {
        std::cerr << "Error opening Gnuplot pipe." << std::endl;
        return;
    }

    // 4. Send Gnuplot commands and data
    fprintf(pipe, "reset session\n");
    fprintf(pipe, "set title 'FFT Spectrum'\\n");
    fprintf(pipe, "set xlabel 'Frequency Bin'\\n");
    fprintf(pipe, "set ylabel 'Magnitude'\\n");
    fprintf(pipe, "plot '-' with impulses title 'FFT Magnitude'\\n"); // '-' tells gnuplot to read data from stdin

    for (int i = 0; i < fft.size(); ++i) {
        auto magnitude = std::abs(fft[i]); // Calculate magnitude of complex number
        fprintf(pipe, "%d %lf\\n", i, magnitude); // Send data points (bin, magnitude)
    }

    fprintf(pipe, "e\\n"); // 'e' signals the end of data input for the current plot command
    fflush(pipe); // Flush the pipe to ensure data is sent immediately
    pclose(pipe);
}

template <std::floating_point T>
void plot_signal(const std::vector<std::complex<T>>& sig)
{
    // 2. Open a pipe to gnuplot
    FILE* gnuplotPipe = popen("gnuplot -persist", "w"); // Use popen on Linux/macOS
    if (!gnuplotPipe) {
        std::cerr << "Error opening gnuplot pipe." << std::endl;
        return;
    }

    // 3. Send commands and data
    // Configure plot
    fprintf(gnuplotPipe, "reset session\n");
    fprintf(gnuplotPipe, "set title 'Complex Signal Components vs. Sample Index'\n");
    fprintf(gnuplotPipe, "set xlabel 'Sample Index (n)'\n");
    fprintf(gnuplotPipe, "set ylabel 'Value'\n");
    // Plot using the pipe's stdin ('-') and two columns (1:2 and 1:3 for real and imag parts)
    fprintf(gnuplotPipe, "plot '-' using 1:2 with lines title 'Real Part', "
                         "'-' using 1:2 with lines title 'Imaginary Part'\n");

    // Send data for the "Real Part" plot
    for (int i = 0; i < sig.size(); ++i) {
        fprintf(gnuplotPipe, "%d %lf\n", i, sig[i].real());
    }
    fprintf(gnuplotPipe, "e\n"); // 'e' ends the data block for the first plot

    // Send data for the "Imaginary Part" plot
    for (int i = 0; i < sig.size(); ++i) {
        fprintf(gnuplotPipe, "%d %lf\n", i, sig[i].imag());
    }
    fprintf(gnuplotPipe, "e\n"); // 'e' ends the data block for the second plot

    fprintf(gnuplotPipe, "quit\n"); // Exit gnuplot process (optional with -persist)
    fflush(gnuplotPipe); // Flush the pipe buffer

    // 4. Close the pipe
    pclose(gnuplotPipe);
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
    auto constel = ofdm::to_constellations<ofdm::e16QAM>(in);
//    plot_constellation(constel);
    ofdm::tx(constel)
        .transform([](auto&& modul)
        {
            plot_signal(modul);
//            fft::fft2(mod.begin(), mod.end());
        });


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
