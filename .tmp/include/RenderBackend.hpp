#pragma once

#include <vulkan/vulkan.h>

struct RenderBackend {
    VkAllocationCallbacks *g_Allocator = nullptr;
    VkInstance g_Instance = VK_NULL_HANDLE;
    VkPhysicalDevice g_PhysicalDevice = VK_NULL_HANDLE;
    VkDevice g_Device = VK_NULL_HANDLE;
    uint32_t g_QueueFamily = static_cast<uint32_t>(-1);
    VkQueue g_Queue = VK_NULL_HANDLE;
    VkPipelineCache g_PipelineCache = VK_NULL_HANDLE;
    VkDescriptorPool g_DescriptorPool = VK_NULL_HANDLE;

    VkPipelineLayout pipelineLayout;
    VkRenderPass renderPass;

#ifdef IMGUI_VULKAN_DEBUG_REPORT
    VkDebugReportCallbackEXT g_DebugReport = VK_NULL_HANDLE;
#endif

    RenderBackend(std::vector<std::string> &&extensions);
    ~RenderBackend();
};
