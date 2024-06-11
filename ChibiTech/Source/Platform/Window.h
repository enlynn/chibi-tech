#pragma once

#include <memory>

#include <Types.h>

struct GLFWwindow;

namespace ct::os {
    // TODO(enlynn): Ideally this could be used to identify user callback functions for input.
    constexpr u32 cMainClientWindowId = 0;

    class Window {
    public:
        Window(size_t tWidth, size_t tHeight, std::string_view tWindowTitle);
        ~Window();

        [[nodiscard]] bool isValid() const { return mHandle != nullptr; };
        [[nodiscard]] bool isRunning() const;

        [[nodiscard]] GLFWwindow* asHandle() const { return mHandle; }

        [[nodiscard]] std::tuple<u32, u32> getSize() const;

        void pollInputs();

    private:
        GLFWwindow* mHandle;

        //  TODO:
        // - Callback functions
        // - Window Input.
    };

    using WindowUptr = std::unique_ptr<Window>;
} // ct
