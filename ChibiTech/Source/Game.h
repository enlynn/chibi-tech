#pragma once

#include <string_view>

namespace ct {
    class Engine;

    struct GameInfo {
        std::string_view mWindowTitle;
        size_t           mWindowWidth{0};
        size_t           mWindowHeight{0};
        const char*      mAssetPath;
    };

    class Game {
    public:
        [[nodiscard]] virtual GameInfo getGameInfo() const = 0;

        virtual bool onInit(Engine& tEngine)    { return true; }
        virtual bool onUpdate(Engine& tEngine)  { return true; }
        virtual bool onRender(Engine& tEngine)  { return true; }
        virtual bool onDestroy(Engine& tEngine) { return true; }
    };
}
