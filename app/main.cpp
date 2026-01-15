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
#include <algorithm>

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
