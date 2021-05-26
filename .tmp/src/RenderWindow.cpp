#include <cstdlib>
#include <spdlog/spdlog.h>

#include "RenderWindow.hpp"
#include "RenderBackend.hpp"

static void check_vk_result(VkResult err)
{
    if (err == 0) return;
    spdlog::error("[vulkan] Error: VkResult = {}\n", err);
    if (err < 0) std::abort();
}

RenderWindow::RenderWindow(RenderBackend &backend, VkSurfaceKHR surface, int width, int height) :
    m_backend{backend}, wd{}
{
    wd.Surface = surface;

    // Check for WSI support
    VkBool32 res;
    vkGetPhysicalDeviceSurfaceSupportKHR(m_backend.g_PhysicalDevice, m_backend.g_QueueFamily, wd.Surface, &res);
    if (res != VK_TRUE) {
        spdlog::error("Error no WSI support on physical device 0");
        exit(-1);
    }

    // Select Surface Format
    const VkFormat requestSurfaceImageFormat[] = {
        VK_FORMAT_B8G8R8A8_UNORM, VK_FORMAT_R8G8B8A8_UNORM, VK_FORMAT_B8G8R8_UNORM, VK_FORMAT_R8G8B8_UNORM};
    const VkColorSpaceKHR requestSurfaceColorSpace = VK_COLORSPACE_SRGB_NONLINEAR_KHR;
    wd.SurfaceFormat = ImGui_ImplVulkanH_SelectSurfaceFormat(
        m_backend.g_PhysicalDevice,
        wd.Surface,
        requestSurfaceImageFormat,
        (size_t) IM_ARRAYSIZE(requestSurfaceImageFormat),
        requestSurfaceColorSpace);

    // Select Present Mode
#ifdef IMGUI_UNLIMITED_FRAME_RATE
    VkPresentModeKHR present_modes[] = {
        VK_PRESENT_MODE_MAILBOX_KHR, VK_PRESENT_MODE_IMMEDIATE_KHR, VK_PRESENT_MODE_FIFO_KHR};
#else
    VkPresentModeKHR present_modes[] = {VK_PRESENT_MODE_FIFO_KHR};
#endif
    wd.PresentMode = ImGui_ImplVulkanH_SelectPresentMode(
        m_backend.g_PhysicalDevice, wd.Surface, &present_modes[0], IM_ARRAYSIZE(present_modes));
    // printf("[vulkan] Selected PresentMode = %d\n", wd.PresentMode);

    // Create SwapChain, RenderPass, Framebuffer, etc.
    IM_ASSERT(g_MinImageCount >= 2);
    ImGui_ImplVulkanH_CreateOrResizeWindow(
        m_backend.g_Instance,
        m_backend.g_PhysicalDevice,
        m_backend.g_Device,
        &wd,
        m_backend.g_QueueFamily,
        m_backend.g_Allocator,
        width,
        height,
        g_MinImageCount);
}

RenderWindow::~RenderWindow()
{
    ImGui_ImplVulkanH_DestroyWindow(m_backend.g_Instance, m_backend.g_Device, &wd, m_backend.g_Allocator);
}

void RenderWindow::FrameRender(ImDrawData *draw_data)
{
    VkResult err;

    VkSemaphore image_acquired_semaphore = wd.FrameSemaphores[wd.SemaphoreIndex].ImageAcquiredSemaphore;
    VkSemaphore render_complete_semaphore = wd.FrameSemaphores[wd.SemaphoreIndex].RenderCompleteSemaphore;
    err = vkAcquireNextImageKHR(
        m_backend.g_Device, wd.Swapchain, UINT64_MAX, image_acquired_semaphore, VK_NULL_HANDLE, &wd.FrameIndex);
    if (err == VK_ERROR_OUT_OF_DATE_KHR || err == VK_SUBOPTIMAL_KHR) {
        g_SwapChainRebuild = true;
        return;
    }
    check_vk_result(err);

    ImGui_ImplVulkanH_Frame *fd = &wd.Frames[wd.FrameIndex];
    {
        // wait indefinitely instead of periodically checking
        err = vkWaitForFences(m_backend.g_Device, 1, &fd->Fence, VK_TRUE, UINT64_MAX);
        check_vk_result(err);

        err = vkResetFences(m_backend.g_Device, 1, &fd->Fence);
        check_vk_result(err);
    }
    {
        err = vkResetCommandPool(m_backend.g_Device, fd->CommandPool, 0);
        check_vk_result(err);
        VkCommandBufferBeginInfo info = {};
        info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        info.flags |= VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
        err = vkBeginCommandBuffer(fd->CommandBuffer, &info);
        check_vk_result(err);
    }
    {
        VkRenderPassBeginInfo info = {};
        info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
        info.renderPass = wd.RenderPass;
        info.framebuffer = fd->Framebuffer;
        info.renderArea.extent.width = static_cast<uint32_t>(wd.Width);
        info.renderArea.extent.height = static_cast<uint32_t>(wd.Height);
        info.clearValueCount = 1;
        info.pClearValues = &wd.ClearValue;
        vkCmdBeginRenderPass(fd->CommandBuffer, &info, VK_SUBPASS_CONTENTS_INLINE);
    }

    // Record dear imgui primitives into command buffer
    ImGui_ImplVulkan_RenderDrawData(draw_data, fd->CommandBuffer);

    // Submit command buffer
    vkCmdEndRenderPass(fd->CommandBuffer);
    {
        VkPipelineStageFlags wait_stage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        VkSubmitInfo info = {};
        info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        info.waitSemaphoreCount = 1;
        info.pWaitSemaphores = &image_acquired_semaphore;
        info.pWaitDstStageMask = &wait_stage;
        info.commandBufferCount = 1;
        info.pCommandBuffers = &fd->CommandBuffer;
        info.signalSemaphoreCount = 1;
        info.pSignalSemaphores = &render_complete_semaphore;

        err = vkEndCommandBuffer(fd->CommandBuffer);
        check_vk_result(err);
        err = vkQueueSubmit(m_backend.g_Queue, 1, &info, fd->Fence);
        check_vk_result(err);
    }
}

void RenderWindow::FramePresent()
{
    if (g_SwapChainRebuild) return;
    VkSemaphore render_complete_semaphore = wd.FrameSemaphores[wd.SemaphoreIndex].RenderCompleteSemaphore;
    VkPresentInfoKHR info = {};
    info.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    info.waitSemaphoreCount = 1;
    info.pWaitSemaphores = &render_complete_semaphore;
    info.swapchainCount = 1;
    info.pSwapchains = &wd.Swapchain;
    info.pImageIndices = &wd.FrameIndex;
    VkResult err = vkQueuePresentKHR(m_backend.g_Queue, &info);
    if (err == VK_ERROR_OUT_OF_DATE_KHR || err == VK_SUBOPTIMAL_KHR) {
        g_SwapChainRebuild = true;
        return;
    }
    check_vk_result(err);
    wd.SemaphoreIndex = (wd.SemaphoreIndex + 1) % wd.ImageCount; // Now we can use the next set of semaphores
}
