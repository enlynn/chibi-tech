#include "stdafx.h"
#include "Window.h"

#include <GLFW/glfw3.h>

namespace ct::os {
    Window::Window(size_t tWidth, size_t tHeight, std::string_view tWindowTitle)
    {
        mHandle = glfwCreateWindow(
                static_cast<int>(tWidth),
                static_cast<int>(tHeight),
                tWindowTitle.data(),
                nullptr, nullptr);

        // For multi-viewport support, we probably don't want to do this immediately
        glfwMakeContextCurrent(mHandle);
    }

    Window::~Window()
    {
        if (mHandle)
        {
            glfwDestroyWindow(mHandle);
        }
    }

    bool Window::isRunning() const
    {
        return isValid() && !glfwWindowShouldClose(mHandle);
    }

    void Window::pollInputs()
    {
        glfwMakeContextCurrent(mHandle);

        glfwPollEvents();
    }

    std::tuple<u32, u32> Window::getSize() const {
        int width, height;
        glfwGetFramebufferSize(mHandle, &width, &height);
        return std::tie(width, height);
    }
} // ct