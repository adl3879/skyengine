#include "application.h"

extern sky::Application *CreateApplication();

int main(int argc, char **argv)
{
    auto app = sky::CreateApplication();
    app->run();
    delete app;
}
