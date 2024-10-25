#include "application.h"

#include <iostream>

extern sky::Application *CreateApplication();

int main(int argc, char **argv)
{
    auto app = sky::CreateApplication();
    app->run();
    delete app;
    std::cin.get();
}
