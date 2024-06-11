#include "stdafx.h"
#include "Engine.h"

#define GLFW_INCLUDE_NONE
#ifdef _WIN32
#define GLFW_EXPOSE_NATIVE_WIN32
#endif
#include <GLFW/glfw3.h>
#include <GLFW/glfw3native.h>

#include "Game.h"
#include "Platform/Console.h"
#include "Platform/Platform.h"
#include "Platform/Timer.h"
#include "Platform/Window.h"
#include "Platform/Assert.h"

#include "Gpu/GpuState.h"

#include "util/SparseStorage.h"

namespace {
    ct::EngineSptr sGlobalEngine{nullptr};
}

namespace ct {


    EngineSptr getEngine() {
        ASSERT(sGlobalEngine);
        return sGlobalEngine;
    }

    void createEngine(const EngineInfo& tInfo) {
        sGlobalEngine = std::make_shared<ct::Engine>(tInfo);
    }

    Engine::Engine(const EngineInfo& tInfo)
    : mClientWindow(nullptr)
    , mAssetDirectory(tInfo.mAssetDirectory)
    , mShaderLoader(mAssetDirectory / "Shaders")
    {
        os::initOSState();

        constexpr console::Flags logFlags = console::Flag::Console | console::Flag::DebugConsole;
        console::setFlags(logFlags);

        glfwInit();
        glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);

        mClientWindow = std::make_unique<os::Window>(tInfo.mWindowWidth, tInfo.mWindowHeight, tInfo.WindowTitle);
        mGpuState = std::make_unique<GpuState>(*mClientWindow);

        console::info("Engine initialized");
    }

    void Engine::shutdown()
    {
        mGpuState->destroy();

        glfwTerminate();
        os::deinitOSState();
    }

    void Engine::run(Game* tGame)
    {
        console::info("Engine running.");

        // TODO(enylnn): respond to the refresh rate of the monitor.
        constexpr f32 cRefreshRateHz = 1.0f / 60.0f;

        auto elapsedTimer = os::Timer();
        elapsedTimer.start();

        auto frameTimer = os::Timer();

        while (true) {
            if (mClientWindow && !mClientWindow->isRunning()) {
                break;
            }

            frameTimer.start();

            // poll for user input
            mClientWindow->pollInputs();

            // Update the game app
            const bool onUpdateResult = tGame->onUpdate(*this);
            if (!onUpdateResult) {
                break;
            }

            // Render the game app
            const bool onRenderResult = tGame->onRender(*this);
            if (!onRenderResult) {
                break;
            }

            // End of the frame
            elapsedTimer.update();
            frameTimer.update();

            // Meet the target frame rate so we don't melt the CPU/GPU
            const f64 targetRefreshRate      = cRefreshRateHz;
            const f64 workSecondsElapsed     = frameTimer.getSecondsElapsed();
            const f64 secondsElapsedForFrame = workSecondsElapsed;
            if (secondsElapsedForFrame < targetRefreshRate)
            {
                s64 sleepMS = (s64)(1000.0 * (targetRefreshRate - secondsElapsedForFrame));
                if (sleepMS > 0)
                {
                    os::sleepMainThread((u32)sleepMS);
                }
            }
        }

        console::info("Engine finished running.");
    }

    GpuState* Engine::getGpuState() const {
        return mGpuState.get();
    }

    Engine::~Engine() {
        // required for partial types to know how to destruct
    }

    ShaderResource Engine::loadShader(std::string_view tShaderName, ShaderStage tStage) {
        ShaderResourceBlob blob = mShaderLoader.loadShader(tShaderName, tStage);
        return ShaderResource(tStage, blob);
    }

    void Engine::unloadShader(ShaderResource tShader) {

    }
}
