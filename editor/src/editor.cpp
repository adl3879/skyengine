#include <core/application.h>

#include "editor_layer.h"
#include <skypch.h>

class Editor : public sky::Application
{
public:
    Editor() 
    {
        pushLayer(new sky::EditorLayer());
    };
    ~Editor() = default;
};

sky::Application *sky::CreateApplication()
{
    return new Editor();
}