#pragma once

#include <skypch.h>

namespace sky
{
class FPSCounter
{
  public:
    FPSCounter() 
        : frameCount(0), elapsedTime(0.0), deltaTime(0), lastTime(std::chrono::high_resolution_clock::now()), fps(0.0) {}

    void frameRendered()
    {
        // Calculate time since the last frame
        auto currentTime = std::chrono::high_resolution_clock::now();
        deltaTime = std::chrono::duration<double>(currentTime - lastTime).count();
        lastTime = currentTime;

        // Update elapsed time and frame count
        elapsedTime += deltaTime;
        frameCount++;

        // Update FPS every second
        if (elapsedTime >= 1.0)
        {
            fps = frameCount / elapsedTime;
            frameCount = 0;
            elapsedTime = 0.0;
        }
    }

    double getFPS() const { return fps; }
    double getDeltaTime() const { return deltaTime; }

  private:
    int frameCount;                                          // Number of frames rendered
    double elapsedTime;                                      // Time elapsed since the last FPS update
    std::chrono::high_resolution_clock::time_point lastTime; // Time point of the last frame
    double fps;                                              // Current FPS
    double deltaTime;
};
}