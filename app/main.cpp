#include "ofdm.hpp"
#include <imgui.h>
#include <implot.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>
#include <GLFW/glfw3.h>
#include <vector>
#include <complex>
#include <mutex>
#include <deque>
#include <random>
#include <string>
#include <algorithm>
#include <cmath>
#include <expected>
#include <chrono>

// -------------------- Scrolling buffer --------------------
struct ScrollingBuffer {
    std::vector<std::complex<float>> data;
    size_t max_size;
    std::mutex mtx;

    ScrollingBuffer(size_t size) : max_size(size) {
        data.resize(max_size, {0.0f, 0.0f});
    }

    void push_vector(const std::vector<std::complex<double>>& vec) {
        std::lock_guard lock(mtx);
        size_t N = vec.size();
        if (N >= max_size) {
            for (size_t i = 0; i < max_size; ++i)
                data[i] = {static_cast<float>(vec[N - max_size + i].real()),
                           static_cast<float>(vec[N - max_size + i].imag())};
        } else {
            for (size_t i = 0; i < max_size - N; ++i)
                data[i] = data[i + N];
            for (size_t i = 0; i < N; ++i)
                data[max_size - N + i] = {static_cast<float>(vec[i].real()),
                                           static_cast<float>(vec[i].imag())};
        }
    }
};

ScrollingBuffer g_time_plot(512);

// -------------------- Random generators --------------------
std::mt19937 rng{std::random_device{}()};
std::uniform_real_distribution<double> noise_dist(-0.02, 0.02);

// -------------------- Payload --------------------
const std::string payload = "Hello, world!       I am the Software-Defined Radio Stack";
size_t payload_pos = 0;

// -------------------- Decoded text --------------------
std::deque<unsigned char> g_decoded_text;

// -------------------- Helper --------------------
void plot_signal(const std::vector<std::complex<double>>& signal) {
    g_time_plot.push_vector(signal);
}

// -------------------- Main --------------------
int main() {
    if (!glfwInit()) return -1;
    GLFWwindow* window = glfwCreateWindow(1200, 600, "OFDM Demo", nullptr, nullptr);
    if (!window) return -1;
    glfwMakeContextCurrent(window);
    glfwSwapInterval(1); // Vsync

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImPlot::CreateContext();
    ImGui::StyleColorsDark();
    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init("#version 130");

    std::vector<uint8_t> input_bits;

    // -------------------- Timer for controlled update rate --------------------
    auto last_frame = std::chrono::steady_clock::now();
    double frame_interval = 0.02; // 25 FPS (~half original speed)

    while (!glfwWindowShouldClose(window)) {
        // -------------------- Wait for next frame --------------------
        auto now = std::chrono::steady_clock::now();
        double elapsed = std::chrono::duration<double>(now - last_frame).count();
        if (elapsed < frame_interval) {
            glfwWaitEventsTimeout(frame_interval - elapsed);
            continue;
        }
        last_frame = now;

        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        // -------------------- Prepare input --------------------
        input_bits.clear();
        for (size_t i = 0; i < 4; ++i) { // 4 bytes per frame
            char c = payload[payload_pos % payload.size()];
            payload_pos++;
            input_bits.push_back(static_cast<uint8_t>(c));
        }

        // -------------------- Constellation mapping --------------------
        auto constellation_syms = ofdm::to_constellations<ofdm::e16QAM>(input_bits);

        // -------------------- TX signal --------------------
        auto tx_sig_exp = ofdm::tx(constellation_syms, 8);
        if (!tx_sig_exp) continue;
        auto tx_sig = *tx_sig_exp;

        // -------------------- Add small noise --------------------
        for (auto &s : tx_sig) s += std::complex<double>(noise_dist(rng), noise_dist(rng));

        plot_signal(tx_sig);

        // -------------------- RX signal --------------------
        auto rx_sig_exp = ofdm::rx(tx_sig, 8);
        if (rx_sig_exp) {
            auto rx_const = *rx_sig_exp;
            auto rx_bytes = ofdm::from_constellations<ofdm::e16QAM>(rx_const);
            for (auto b : rx_bytes) g_decoded_text.push_back(b);
            if (g_decoded_text.size() > 200)
                g_decoded_text.erase(g_decoded_text.begin(),
                                     g_decoded_text.begin() + (g_decoded_text.size() - 200));
        }

        // -------------------- ImGui window --------------------
        ImGui::SetNextWindowSize(ImVec2(1100, 550), ImGuiCond_FirstUseEver);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(10,10));
        ImGui::Begin("OFDM TX Demo");

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
                ImPlot::SetupAxisLimits(ImAxis_X1, 0, static_cast<double>(N), ImGuiCond_Always);
                ImPlot::SetupAxisLimits(ImAxis_Y1, -4.0, 4.0, ImGuiCond_Always);
                ImPlot::PlotLine("Re{x[n]}", re.data(), N);
                ImPlot::PlotLine("Im{x[n]}", im.data(), N);
                ImPlot::EndPlot();
            }
        }

        // -------------------- Constellation plot --------------------
        {
            std::vector<float> re_c(constellation_syms.size()), im_c(constellation_syms.size());
            for (size_t i = 0; i < constellation_syms.size(); ++i) {
                re_c[i] = static_cast<float>(constellation_syms[i].real());
                im_c[i] = static_cast<float>(constellation_syms[i].imag());
            }

            if (ImPlot::BeginPlot("Constellation", ImVec2(-1, 200))) {
                ImPlot::SetupAxes("Re","Im");
                float margin = 0.3f;
                ImPlot::SetupAxisLimits(ImAxis_X1, -4.0f - margin, 4.0f + margin, ImGuiCond_Always);
                ImPlot::SetupAxisLimits(ImAxis_Y1, -4.0f - margin, 4.0f + margin, ImGuiCond_Always);
                ImPlot::PlotScatter("Symbols", re_c.data(), im_c.data(), re_c.size());
                ImPlot::EndPlot();
            }
        }

        // -------------------- Received text --------------------
        {
            static size_t offset = 0;
            std::string running_text(g_decoded_text.begin(), g_decoded_text.end());
            if (!running_text.empty()) {
                offset = (offset + 1) % running_text.size();
                std::string sub = running_text.substr(offset) + running_text.substr(0, offset);
                ImGui::TextWrapped("%s", sub.c_str());
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
