#include "debug_ui.h"

#include "debug/debug_settings.h"
#include "util/input.h"

#include <imgui.h>
#include <implot.h>

#include <algorithm>
#include <cmath>
#include <string>
#include <vector>

#include <iostream>

namespace
{
constexpr float GRAPH_HEIGHT = 150.0f;
constexpr float GRAPH_WIDTH = 450.0f;
// number of samples shown by a graph, i.e. seconds, since one sample is pushed per second
constexpr size_t GRAPH_WINDOW = 60;

// room left above the highest value, as a factor of it
constexpr double AXIS_MARGIN = 1.2;
// time constant of the fall of the y axis maximum, in seconds: it covers about two thirds of
// the way down to its target in that time
constexpr float AXIS_FALL_TIME = 2.0f;

/// Max of the `count` most recent values of `buffer` (0 if it is empty).
template <typename T, size_t N> T recentMax(const RingBuffer<T, N> &buffer, size_t count)
{
    T result{};
    size_t n = std::min(count, buffer.size());
    // chronological index i is at data()[(offset() + i) % N]
    for (size_t i = buffer.size() - n; i < buffer.size(); i++)
        result = std::max(result, buffer.data()[(buffer.offset() + i) % N]);
    return result;
}

/**
 * Moves the maximum of a y axis toward `target`: rises at once, so that a spike is never cut,
 * but falls back smoothly, so that the scale doesn't jump when the spike leaves the graph.
 *
 * @param dt duration of the last frame, in seconds: makes the fall independent of the FPS
 */
void updateAxisMax(float &axisMax, float target, float dt)
{
    if (target > axisMax)
        axisMax = target;
    else
        axisMax += (target - axisMax) * (1.0f - std::exp(-dt / AXIS_FALL_TIME));
}

/**
 * Begins a fixed, undecorated and semi-transparent window, anchored to a corner of the screen.
 * Must be closed with ImGui::End().
 *
 * @param corner corner of the screen: (0, 0) top-left, (1, 0) top-right, (0, 1) bottom-left...
 */
void beginOverlay(const char *name, ImVec2 corner)
{
    constexpr float OVERLAY_MARGIN = 10.0f;
    const ImGuiViewport *viewport = ImGui::GetMainViewport();

    // that corner of the screen, moved inward by the margin (+margin on a 0 side, -margin on a
    // 1 side)
    ImVec2 pos(viewport->WorkPos.x + corner.x * viewport->WorkSize.x
                   + (1.0f - 2.0f * corner.x) * OVERLAY_MARGIN,
               viewport->WorkPos.y + corner.y * viewport->WorkSize.y
                   + (1.0f - 2.0f * corner.y) * OVERLAY_MARGIN);
    // pivot = the same corner of the window, so that the window stays inside the screen
    ImGui::SetNextWindowPos(pos, ImGuiCond_Always, corner);
    ImGui::SetNextWindowBgAlpha(0.35f);

    ImGui::Begin(name,
                 nullptr,
                 ImGuiWindowFlags_NoDecoration // no title bar, resize border or scrollbars
                     | ImGuiWindowFlags_AlwaysAutoResize // window fitted to its content
                     | ImGuiWindowFlags_NoSavedSettings  // nothing stored in imgui.ini
                     | ImGuiWindowFlags_NoFocusOnAppearing
                     | ImGuiWindowFlags_NoInputs); // clicks go through it
}

bool beginGraph(const char *title,
                ImVec2 size,
                double xMax,
                double yMax,
                const char *xLabel = nullptr,
                const char *yLabel = nullptr)
{
    if (!ImPlot::BeginPlot(title, size, ImPlotFlags_NoFrame))
        return false;

    ImPlot::SetupAxes(
        xLabel, yLabel, ImPlotAxisFlags_NoTickLabels | ImPlotAxisFlags_NoTickMarks, 0);
    // the most recent value is at x = 0 (see plotSeries()): show the last xMax samples
    ImPlot::SetupAxisLimits(ImAxis_X1, -xMax, 0, ImPlotCond_Always);
    // from 0, so that the height of a curve is proportional to its value. Never an empty
    // range, even before the first value is recorded
    ImPlot::SetupAxisLimits(ImAxis_Y1, 0, std::max(yMax * AXIS_MARGIN, 1.0), ImPlotCond_Always);
    return true;
}

template <typename T, size_t N> void plotSeries(const char *label, const RingBuffer<T, N> &buffer)
{
    // x counts the samples back from the most recent one, which is at x = 0, so that the
    // newest values stay in view whatever the size of the history.
    // (not size() - 1: size_t would wrap around on an empty buffer)
    double xStart = 1.0 - static_cast<double>(buffer.size());

    ImPlot::PlotLine(label,
                     buffer.data(),
                     static_cast<int>(buffer.size()),
                     1.0,
                     xStart,
                     {ImPlotProp_Offset, static_cast<int>(buffer.offset())});
}
} // namespace

void DebugUI::handleInput(Input &input)
{
    bool f3held = input.isKeyPressed(Key::F3);
    auto &settings = debugSettings();

    // check combos
    for (auto &t : TOGGLES)
    {
        if (input.wasKeyPressed(t.key) && f3held)
        {
            settings.*t.flag = !(settings.*t.flag);
            _comboUsed = true;
        }
    }

    // main panel toggle
    if (input.wasKeyReleased(Key::F3))
    {
        if (!_comboUsed)
            debugSettings().showPanel = !debugSettings().showPanel;
        _comboUsed = false;
    }
}

void DebugUI::recordFrame(const DebugFrameInfo &info)
{
    _info = info;

    _frameCount++;
    _fpsTimer += info.frameTime;
    _cpuTimeSum += info.cpuTime;
    _gpuTimeSum += info.gpuTime;

    if (_fpsTimer >= 1.0f)
    {
        _fps = static_cast<float>(_frameCount) / _fpsTimer;
        // averages over the same window as _fps, not just the last frame of it
        _msPerFrame = 1000.0f / _fps;
        _cpuMs = 1000.0f * _cpuTimeSum / _frameCount;
        _gpuMs = 1000.0f * _gpuTimeSum / _frameCount;

        // update graph data buffers
        _fpsBuffer.push(_fps);
        _cpuMsBuffer.push(_cpuMs);
        _gpuMsBuffer.push(_gpuMs);
        _totalMsBuffer.push(_msPerFrame);

        // reset the counters
        _frameCount = 0;
        _fpsTimer -= 1.0f;
        _cpuTimeSum = 0.0f;
        _gpuTimeSum = 0.0f;
    }
}

void DebugUI::render()
{
    // main panel: top-left corner
    beginOverlay("Debug pannel", ImVec2(0.0f, 0.0f));

    ImGui::Text("FPS: %.1f", _fps);
    ImGui::Text("ms per frame: %.3f", _msPerFrame);
    ImGui::Text("CPU: %.3f ms", _cpuMs);
    ImGui::Text("GPU: %.3f ms", _gpuMs);
    ImGui::Text("x:%.2f y:%.2f z:%.2f", _info.cameraPos.x, _info.cameraPos.y, _info.cameraPos.z);
    ImGui::Text("Loaded chunks: %d", _info.loadedChunks);
    ImGui::Text("Rendered chunks: %d", _info.renderedChunks);
    ImGui::Text("PV: %.3f", _info.pvNoise);
    ImGui::Text("Erosion: %.3f", _info.erosionNoise);
    ImGui::Text("Continentalness: %.3f", _info.continentalnessNoise);

    ImGui::End();

    // debug graphs: top-right corner
    beginOverlay("Debug Graphs", ImVec2(1.0f, 0.0f));

    // y axes fitted to the visible values only (the buffers hold more than GRAPH_WINDOW)
    updateAxisMax(_fpsAxisMax, recentMax(_fpsBuffer, GRAPH_WINDOW), _info.frameTime);
    float msTarget = std::max({recentMax(_cpuMsBuffer, GRAPH_WINDOW),
                               recentMax(_gpuMsBuffer, GRAPH_WINDOW),
                               recentMax(_totalMsBuffer, GRAPH_WINDOW)});
    updateAxisMax(_msAxisMax, msTarget, _info.frameTime);

    if (beginGraph("FPS",
                   ImVec2(GRAPH_WIDTH, GRAPH_HEIGHT),
                   GRAPH_WINDOW,
                   _fpsAxisMax,
                   "time (s)",
                   nullptr))
    {
        plotSeries("FPS", _fpsBuffer);
        ImPlot::EndPlot();
    }

    if (beginGraph("Frame time",
                   ImVec2(GRAPH_WIDTH, GRAPH_HEIGHT),
                   GRAPH_WINDOW,
                   _msAxisMax,
                   "time (s)",
                   "ms"))
    {
        plotSeries("CPU", _cpuMsBuffer);
        plotSeries("GPU", _gpuMsBuffer);
        plotSeries("TOTAL", _totalMsBuffer);
        ImPlot::EndPlot();
    }

    ImGui::End();
}
