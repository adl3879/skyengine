#pragma once

#include <skypch.h>
#include <any>
#include <tracy/Tracy.hpp>

namespace sky
{
enum class EditorEventType
{
    NewProject,
    OpenProject,
    SaveSceneAs,
    SaveCurrentScene,
    SaveAllScenes,
    Reset,
    Exit,
    OpenMaterialEditor,
    CreateNewMaterialFrom,
    CreateDefaultMaterial,
    ToggleEnvironmentPanel,
};

struct EditorEvent
{
    EditorEventType type;
    std::any data; // Optional: Additional data for the event
};

class EditorEventBus
{
  public:
    using EditorEventHandler = std::function<void(const EditorEvent &)>;

    static EditorEventBus &get() 
    {
        static EditorEventBus instance;
        return instance;
    }

    // Push a new event into the queue
    void pushEvent(const EditorEvent &event) { eventQueue.push(event); }

    // Register a handler for a specific event type
    void registerHandler(EditorEventType type, const EditorEventHandler &handler) 
    { 
        handlers[type].push_back(handler);
    }

    // Process all events in the queue
    void processEvents()
    {
        ZoneScopedN("Editor events loop");
        while (!eventQueue.empty())
        {
            const EditorEvent &event = eventQueue.front();

            if (handlers.find(event.type) != handlers.end())
            {
                for (const auto &handler : handlers[event.type])
                {
                    handler(event);
                }
            }

            eventQueue.pop();
        }
    }

  private:
    std::queue<EditorEvent> eventQueue;
    std::unordered_map<EditorEventType, std::vector<EditorEventHandler>> handlers;
};
}