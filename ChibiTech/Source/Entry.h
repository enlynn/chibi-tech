#pragma once

// This file must be included at MOST one cpp file in the game project.

#include "Engine.h"
#include "Game.h"

extern std::unique_ptr<ct::Game> onGameLoad();

static void GameEntryPoint()
{
    auto game = onGameLoad();
    if (!game) { // Failed to create the game.
        return;
    }

    ct::GameInfo gameInfo = game->getGameInfo();

    auto createInfo = ct::EngineInfo{
        .WindowTitle     = gameInfo.mWindowTitle.empty() ? "Chibi Tech" : gameInfo.mWindowTitle,
        .mWindowWidth    = gameInfo.mWindowWidth  < 8 ? 1920 : gameInfo.mWindowWidth,
        .mWindowHeight   = gameInfo.mWindowHeight < 8 ? 1080 : gameInfo.mWindowHeight,
        .mAssetDirectory = gameInfo.mAssetPath,
    };

    ct::createEngine(createInfo);

    auto engine = ct::getEngine();
    if (!engine) {
        return;
    }

    if (!game->onInit(*engine)) {
        return; // failed to initialize game project.
    }

    engine->run(game.get());
    engine->shutdown();

    game->onDestroy();
}

#ifdef CT_PLATFORM_WINDOWS
#include <Windows.h>
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, PSTR lpCmdLine, int nCmdShow)
{
    GameEntryPoint();
    return 0;
}
#elif CT_PLATFORM_NIX
int main(void) {
    GameEntryPoint();
    return 0;
}
#else 
#error Unsupported platform.
#endif 

