#pragma once

#include "layer_stack.h"
#include "window.h"

int main(int argc, char** argv);

namespace sky {
class Application
{
  public:
    Application();
    virtual ~Application();

    Application(const Application&) = delete;
    Application& operator=(const Application&) = delete;

    static constexpr int WIDTH = 1400;
    static constexpr int HEIGHT = 1000;
    static constexpr const char* TITLE = "Sky Engine";

    void onEvent(Event &e);

    void pushLayer(Layer *layer);
    void pushOverlay(Layer *layer);

    static Ref<Window> getWindow() { return m_window; }

  private:
    void run();

  private:
    friend int ::main(int argc, char** argv);

  private:
    bool m_isRunning = true;

    LayerStack m_layerStack{};
    static Ref<Window> m_window;
};

Application *CreateApplication();
} // namespace sky