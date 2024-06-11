#pragma once

#include <memory>
#include <optional>
#include <string_view>
#include <filesystem>

#include "Types.h"

#include "Platform/Window.h"

#include "Systems/ShaderLoader.h"

namespace ct {
    class Game;
}

class GpuState;

namespace ct {
    struct EngineInfo {
        std::string_view WindowTitle;
        size_t           mWindowWidth;
        size_t           mWindowHeight;
        const char*      mAssetDirectory;
    };

    class Engine {
    public:
        explicit Engine(const EngineInfo& tInfo);
        void shutdown();

        ~Engine();

        void run(Game* tGame);

        GpuState* getGpuState() const;

        ShaderResource loadShader(std::string_view tShaderName, ShaderStage tStage);
        void unloadShader(ShaderResource tShader);

    private:
        std::unique_ptr<os::Window> mClientWindow{nullptr};
        std::unique_ptr<GpuState>   mGpuState{nullptr};
        std::filesystem::path       mAssetDirectory{};

        ShaderLoader                mShaderLoader;
    };

    using EngineSptr = std::shared_ptr<Engine>;

    EngineSptr getEngine();
    void createEngine(const EngineInfo& tInfo);
}
