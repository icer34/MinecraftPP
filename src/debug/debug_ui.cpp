#include "debug_ui.h"

#include "util/input.h"

#include <imgui.h>

void DebugUI::handleInput(Input &window) {}

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

        _frameCount = 0;
        _fpsTimer -= 1.0f;
        _cpuTimeSum = 0.0f;
        _gpuTimeSum = 0.0f;
    }
}

void DebugUI::render()
{
    ImGui::Begin("Debug pannel");

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
}
