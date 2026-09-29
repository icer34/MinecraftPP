#include "debug_ui.h"

#include "util/input.h"

#include <imgui.h>

void DebugUI::handleInput(Input &window) {}

void DebugUI::recordFrame(float dt)
{
    _frameCount++;
    _fpsTimer += dt;

    if (_fpsTimer >= 1.0f)
    {
        _fps = static_cast<float>(_frameCount) / _fpsTimer;
        _frameCount = 0;
        _fpsTimer -= 1.0f;
        // average over the same window as _fps, not just the last frame of it
        _msPerFrame = 1000.0f / _fps;
    }
}

void DebugUI::render(const DebugFrameInfo &info)
{
    ImGui::Begin("Debug pannel");

    ImGui::Text("FPS: %.1f", _fps);
    ImGui::Text("ms per frame: %.3f", _msPerFrame);
    ImGui::Text("x:%.2f y:%.2f z:%.2f", info.cameraPos.x, info.cameraPos.y, info.cameraPos.z);
    ImGui::Text("Loaded chunks: %d", info.loadedChunks);
    ImGui::Text("Rendered chunks: %d", info.renderedChunks);
    ImGui::Text("PV: %.3f", info.pvNoise);
    ImGui::Text("Erosion: %.3f", info.erosionNoise);
    ImGui::Text("Continentalness: %.3f", info.continentalnessNoise);

    ImGui::End();
}