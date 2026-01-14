// #include "fft.hpp"
// #include "ofdm.hpp"
// #include <format>
// #include <complex>
// #include <vector>
// #include <print>
// #include <ranges>

// template <typename T, typename CharT>
// struct std::formatter<std::complex<T>, CharT>
// {
//     // Parse the format specifiers (e.g., precision, etc.)
//     constexpr auto parse(auto& ctx)
//     {
//         return ctx.begin();
//     }

//     // Format the complex number
//     auto format(const std::complex<T>& c, auto& ctx) const
//     {
//         return std::format_to(ctx.out(), "({}+{}i)", c.real(), c.imag());
//     }
// };

// template <typename T>
// struct std::formatter<std::vector<T>> {
//     constexpr auto parse(format_parse_context& ctx) { return ctx.begin(); }

//     auto format(const std::vector<T>& v, format_context& ctx) const {
//         auto out = ctx.out();
//         out = std::format_to(out, "[");
//         for (size_t i = 0; i < v.size(); ++i) {
//             out = std::format_to(out, "{}{}", v[i], (i == v.size() - 1 ? "" : ", "));
//         }
//         return std::format_to(out, "]");
//     }
// };


// #include <iostream>
// #include <cstdio> // Required for popen
// #include <iomanip>

// void plot_constellation(const std::vector<std::complex<double>>& points) 
// {
//     FILE *gp = popen("gnuplot -persist", "w"); // Open a pipe to gnuplot

//     if (gp == nullptr) {
//         std::cerr << "Error opening pipe to Gnuplot. Make sure Gnuplot is installed and in PATH." << std::endl;
//         return;
//     }

//     // Gnuplot commands to set up the plot
//     fprintf(gp, "reset session\n");
//     fprintf(gp, "set term qt size 600,600\n");
//     fprintf(gp, "unset key\n");
//     fprintf(gp, "unset multiplot\n");
//     fprintf(gp, "unset logscale\n");
//     fprintf(gp, "set size square\n");

//     fprintf(gp, "set title 'OFDM 16-QAM Constellation'\n");
//     fprintf(gp, "set xlabel 'In-phase (I)'\n");
//     fprintf(gp, "set ylabel 'Quadrature (Q)'\n");
//     fprintf(gp, "set grid\n");
//     fprintf(gp, "set xrange [-3.5:3.5]\n");
//     fprintf(gp, "set yrange [-3.5:3.5]\n");
//     fprintf(gp, "plot '-' with points pt 6 ps 1 lc rgb \"blue\"\n");

//     for (const auto& p : points)
//         fprintf(gp, "%.6f %.6f\n", p.real(), p.imag());

//     fprintf(gp, "e\n");
//     fflush(gp);
//     pclose(gp);
// }

// template <std::floating_point T>
// void plot_fft(const std::vector<std::complex<T>>& fft)
// {
//     FILE* pipe = popen("gnuplot -persist", "w"); // Use popen on Linux/macOS

//     if (!pipe) {
//         std::cerr << "Error opening Gnuplot pipe." << std::endl;
//         return;
//     }

//     // 4. Send Gnuplot commands and data
//     fprintf(pipe, "reset session\n");
//     fprintf(pipe, "set title 'FFT Spectrum'\\n");
//     fprintf(pipe, "set xlabel 'Frequency Bin'\\n");
//     fprintf(pipe, "set ylabel 'Magnitude'\\n");
//     fprintf(pipe, "plot '-' with impulses title 'FFT Magnitude'\\n"); // '-' tells gnuplot to read data from stdin

//     for (int i = 0; i < fft.size(); ++i) {
//         auto magnitude = std::abs(fft[i]); // Calculate magnitude of complex number
//         fprintf(pipe, "%d %lf\\n", i, magnitude); // Send data points (bin, magnitude)
//     }

//     fprintf(pipe, "e\\n"); // 'e' signals the end of data input for the current plot command
//     fflush(pipe); // Flush the pipe to ensure data is sent immediately
//     pclose(pipe);
// }

// template <std::floating_point T>
// void plot_signal(const std::vector<std::complex<T>>& sig)
// {
//     // 2. Open a pipe to gnuplot
//     FILE* gnuplotPipe = popen("gnuplot -persist", "w"); // Use popen on Linux/macOS
//     if (!gnuplotPipe) {
//         std::cerr << "Error opening gnuplot pipe." << std::endl;
//         return;
//     }

//     // 3. Send commands and data
//     // Configure plot
//     fprintf(gnuplotPipe, "reset session\n");
//     fprintf(gnuplotPipe, "set title 'Complex Signal Components vs. Sample Index'\n");
//     fprintf(gnuplotPipe, "set xlabel 'Sample Index (n)'\n");
//     fprintf(gnuplotPipe, "set ylabel 'Value'\n");
//     // Plot using the pipe's stdin ('-') and two columns (1:2 and 1:3 for real and imag parts)
//     fprintf(gnuplotPipe, "plot '-' using 1:2 with lines title 'Real Part', "
//                          "'-' using 1:2 with lines title 'Imaginary Part'\n");

//     // Send data for the "Real Part" plot
//     for (int i = 0; i < sig.size(); ++i) {
//         fprintf(gnuplotPipe, "%d %lf\n", i, sig[i].real());
//     }
//     fprintf(gnuplotPipe, "e\n"); // 'e' ends the data block for the first plot

//     // Send data for the "Imaginary Part" plot
//     for (int i = 0; i < sig.size(); ++i) {
//         fprintf(gnuplotPipe, "%d %lf\n", i, sig[i].imag());
//     }
//     fprintf(gnuplotPipe, "e\n"); // 'e' ends the data block for the second plot

//     fprintf(gnuplotPipe, "quit\n"); // Exit gnuplot process (optional with -persist)
//     fflush(gnuplotPipe); // Flush the pipe buffer

//     // 4. Close the pipe
//     pclose(gnuplotPipe);
// }

// int main()
// {
//     std::vector<std::complex<double>> d{0,1,2,3,4,5,6,7};
//     std::vector<uint8_t> in{ 0b00000001
//                            , 0b00100011
//                            , 0b01000101
//                            , 0b01100111
//                            , 0b10001001
//                            , 0b10101011
//                            , 0b11001101
//                            , 0b11101111 };
//     auto constel = ofdm::to_constellations<ofdm::e16QAM>(in);
// //    plot_constellation(constel);

// //    std::vector<std::complex<double>> aa(128, 0+0j);
//     std::vector<std::complex<double>> aa(256, 0+0j);
//     aa[2] = aa[3] = aa[4] = aa[5] = 1+0j;
//     // for (auto i = 56; i < 74; ++i)
//     //     aa[i] = 1+0j;
//     ofdm::tx(aa, 32)
// //    ofdm::tx(constel)
//         .transform([](auto&& modul)
//         {
//             plot_signal(modul);
// //            fft::fft2(mod.begin(), mod.end());
//         });


//     // std::println("{}\n", d);

//     // fft::fft2(d.begin(), d.end());

//     // std::println("{}\n", d);

//     // fft::ifft2(d.begin(), d.end());

//     // std::println("{}", d);

//     auto r = ofdm::tx(d, 4);
//     if (r.has_value())
// //        for (auto v: r.value()) std::print("{}", v);
//         std::println("{}\n", r.value());
//     else
//         std::println("{}", r.error());
//     return 0;
// }

#include "ofdm.hpp"
#include <imgui.h>
#include <implot.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>
#include <GLFW/glfw3.h>
#include <vector>
#include <complex>
#include <mutex>
#include <cmath>
#include <expected>
#include <random>
#include <cstdint>

// -------------------- Scrolling buffer --------------------
struct ScrollingBuffer {
    std::vector<std::complex<float>> data;
    size_t max_size;
    std::mutex mtx;

    ScrollingBuffer(size_t size) : max_size(size) {
        data.resize(max_size, {0.0f,0.0f});
    }

    // Shift right, insert new samples at front → waveform moves left→right
    void push_vector(const std::vector<std::complex<double>>& vec) {
        std::lock_guard lock(mtx);
        size_t N = vec.size();
        if (N >= max_size) {
            for (size_t i = 0; i < max_size; ++i)
                data[i] = {static_cast<float>(vec[N - max_size + i].real()),
                           static_cast<float>(vec[N - max_size + i].imag())};
        } else {
            // Shift existing data right
            for (size_t i = max_size-1; i >= N; --i)
                data[i] = data[i-N];
            // Copy new samples to front
            for (size_t i = 0; i < N; ++i)
                data[i] = {static_cast<float>(vec[i].real()), static_cast<float>(vec[i].imag())};
        }
    }
};

ScrollingBuffer g_time_plot(512);

// -------------------- Random noise --------------------
std::mt19937 rng{std::random_device{}()};
std::uniform_real_distribution<double> noise_dist(-0.05, 0.05);
std::uniform_int_distribution<uint8_t> bit_dist(0,15);

// -------------------- Plot helper --------------------
void plot_signal(const std::vector<std::complex<double>>& signal) {
    g_time_plot.push_vector(signal);
}

// -------------------- Main --------------------
int main() {
    if (!glfwInit()) return -1;
    GLFWwindow* window = glfwCreateWindow(1200, 600, "OFDM Time + Constellation", nullptr, nullptr);
    if (!window) return -1;
    glfwMakeContextCurrent(window);
    glfwSwapInterval(1);

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImPlot::CreateContext();
    ImGui::StyleColorsDark();
    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init("#version 130");

    size_t frame_counter = 0;

    while (!glfwWindowShouldClose(window)) {
        // -------------------- Poll events with small delay (~50 FPS) --------------------
        glfwWaitEventsTimeout(0.02); // ~50Hz
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        // -------------------- Generate new "incoming" bits --------------------
        std::vector<uint8_t> input_bits(32);
        for (auto &b : input_bits) b = bit_dist(rng);

        // -------------------- Map to constellation symbols --------------------
        auto constellation_syms = ofdm::to_constellations<ofdm::e16QAM>(input_bits);

        // -------------------- Add small random noise --------------------
        for (auto &s : constellation_syms)
            s += std::complex<double>(noise_dist(rng), noise_dist(rng));

        // -------------------- Generate time-domain waveform --------------------
        auto tx_sig = ofdm::tx(constellation_syms, 8);
        if (tx_sig) plot_signal(*tx_sig);

        frame_counter++;

        // -------------------- ImGui Window --------------------
        ImGui::SetNextWindowSize(ImVec2(1100, 550), ImGuiCond_FirstUseEver);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(10,10));
        ImGui::Begin("OFDM TX");

        // -------------------- Time-domain plot --------------------
        {
            std::vector<float> re, im;
            size_t N;
            {
                std::lock_guard lock(g_time_plot.mtx);
                N = g_time_plot.data.size();
                re.resize(N); im.resize(N);
                for (size_t i = 0; i < N; ++i) {
                    re[i] = g_time_plot.data[i].real();
                    im[i] = g_time_plot.data[i].imag();
                }
            }

            if (ImPlot::BeginPlot("x[n]", ImVec2(-1, 300))) {
                ImPlot::SetupAxes("n","Amplitude");
                ImPlot::SetupAxisLimits(ImAxis_X1, 0, static_cast<double>(N));
                ImPlot::SetupAxisLimits(ImAxis_Y1, -2.0, 2.0);
                ImPlot::PlotLine("Re{x[n]}", re.data(), N);
                ImPlot::PlotLine("Im{x[n]}", im.data(), N);
                ImPlot::EndPlot();
            }
        }

        // -------------------- Constellation plot --------------------
        {
            std::vector<float> re_c, im_c;
            re_c.resize(constellation_syms.size());
            im_c.resize(constellation_syms.size());
            for (size_t i = 0; i < constellation_syms.size(); ++i) {
                re_c[i] = static_cast<float>(constellation_syms[i].real());
                im_c[i] = static_cast<float>(constellation_syms[i].imag());
            }

            if (ImPlot::BeginPlot("Constellation", ImVec2(-1, 200))) {
                ImPlot::SetupAxes("Re","Im");
                ImPlot::SetupAxisLimits(ImAxis_X1, -2, 2);
                ImPlot::SetupAxisLimits(ImAxis_Y1, -2, 2);
                ImPlot::PlotScatter("Symbols", re_c.data(), im_c.data(), re_c.size());
                ImPlot::EndPlot();
            }
        }

        ImGui::End();
        ImGui::PopStyleVar();

        // -------------------- Render --------------------
        ImGui::Render();
        int w,h;
        glfwGetFramebufferSize(window,&w,&h);
        glViewport(0,0,w,h);
        glClearColor(0.1f,0.1f,0.1f,1.0f);
        glClear(GL_COLOR_BUFFER_BIT);
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
        glfwSwapBuffers(window);
    }

    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImPlot::DestroyContext();
    ImGui::DestroyContext();
    glfwDestroyWindow(window);
    glfwTerminate();
}
