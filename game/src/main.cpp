#include "game.h"

#include <iostream>

#include "app/application.h"

int main(void)
{
    try
    {
        Game game;
        Application app(game, 1600, 900, "MinecraftPP");
        app.run();
    }
    catch (const std::exception &e)
    {
        std::cerr << e.what() << '\n';
    }
}