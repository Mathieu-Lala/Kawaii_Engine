#pragma once

#include <imgui_impl_vulkan.h>
#include <vulkan/vulkan.h>

struct RenderBackend;

struct RenderWindow {
    RenderBackend &m_backend;

    ImGui_ImplVulkanH_Window wd;
    uint32_t g_MinImageCount = 2;
    bool g_SwapChainRebuild = false;

    RenderWindow(RenderBackend &backend, VkSurfaceKHR surface, int width, int height);
    ~RenderWindow();

    void FrameRender(ImDrawData *draw_data);

    void FramePresent();
};
