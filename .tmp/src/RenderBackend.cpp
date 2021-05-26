#include <cstdlib>

#include <spdlog/spdlog.h>
#include <imgui.h>

#include "RenderBackend.hpp"

static void check_vk_result(VkResult err)
{
    if (err == 0) return;
    spdlog::error("[vulkan] Error: VkResult = {}\n", err);
    if (err < 0) std::abort();
}


#ifdef IMGUI_VULKAN_DEBUG_REPORT
static VKAPI_ATTR VkBool32 VKAPI_CALL debug_report(
    [[maybe_unused]] VkDebugReportFlagsEXT flags,
    VkDebugReportObjectTypeEXT objectType,
    [[maybe_unused]] uint64_t object,
    [[maybe_unused]] size_t location,
    [[maybe_unused]] int32_t messageCode,
    [[maybe_unused]] const char *pLayerPrefix,
    const char *pMessage,
    [[maybe_unused]] void *pUserData)
{
    spdlog::warn("[vulkan] Debug report from ObjectType: {}\nMessage: {}\n", objectType, pMessage);
    return VK_FALSE;
}
#endif // IMGUI_VULKAN_DEBUG_REPORT

RenderBackend::RenderBackend(std::vector<std::string> &&extensions)
{
    VkResult err;

    // Create Vulkan Instance
    {
        std::vector<const char *> cstrings;
        cstrings.reserve(extensions.size());
        for (auto &s : extensions) cstrings.push_back(&s[0]);

#ifdef IMGUI_VULKAN_DEBUG_REPORT
        cstrings.push_back("VK_EXT_debug_report");
        constexpr auto layers = std::to_array({"VK_LAYER_KHRONOS_validation"});
#endif

        VkApplicationInfo appInfo{
            .sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
            .pNext = nullptr,
            .pApplicationName = "Hello Triangle",
            .applicationVersion = VK_MAKE_VERSION(1, 0, 0),
            .pEngineName = "No Engine",
            .engineVersion = VK_MAKE_VERSION(1, 0, 0),
            .apiVersion = VK_API_VERSION_1_0,
        };

        VkInstanceCreateInfo create_info = {
            .sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
            .pNext = nullptr,
            .flags = 0,
            .pApplicationInfo = &appInfo,
#ifdef IMGUI_VULKAN_DEBUG_REPORT
            // Enabling validation layers
            .enabledLayerCount = 1,
            .ppEnabledLayerNames = layers.data(),
#else
            .enabledLayerCount = 0,
            .ppEnabledLayerNames = nullptr,
#endif
            .enabledExtensionCount = static_cast<uint32_t>(cstrings.size()),
            .ppEnabledExtensionNames = cstrings.data(),
        };

        // Create Vulkan Instance
        err = vkCreateInstance(&create_info, g_Allocator, &g_Instance);
        check_vk_result(err);

#ifdef IMGUI_VULKAN_DEBUG_REPORT
        // Get the function pointer (required for any extensions)
        auto vkCreateDebugReportCallbackEXT = reinterpret_cast<PFN_vkCreateDebugReportCallbackEXT>(
            vkGetInstanceProcAddr(g_Instance, "vkCreateDebugReportCallbackEXT"));
        IM_ASSERT(vkCreateDebugReportCallbackEXT != nullptr);

        // Setup the debug report callback
        VkDebugReportCallbackCreateInfoEXT debug_report_ci = {
            .sType = VK_STRUCTURE_TYPE_DEBUG_REPORT_CALLBACK_CREATE_INFO_EXT,
            .pNext = nullptr,
            .flags = VK_DEBUG_REPORT_ERROR_BIT_EXT | VK_DEBUG_REPORT_WARNING_BIT_EXT
                     | VK_DEBUG_REPORT_PERFORMANCE_WARNING_BIT_EXT,
            .pfnCallback = debug_report,
            .pUserData = nullptr,
        };
        err = vkCreateDebugReportCallbackEXT(g_Instance, &debug_report_ci, g_Allocator, &g_DebugReport);
        check_vk_result(err);
#endif
    }

    // Select GPU
    {
        uint32_t gpu_count;
        err = vkEnumeratePhysicalDevices(g_Instance, &gpu_count, nullptr);
        check_vk_result(err);
        IM_ASSERT(gpu_count > 0);

        VkPhysicalDevice *gpus = static_cast<VkPhysicalDevice *>(malloc(sizeof(VkPhysicalDevice) * gpu_count));
        err = vkEnumeratePhysicalDevices(g_Instance, &gpu_count, gpus);
        check_vk_result(err);

        // If a number >1 of GPUs got reported, find discrete GPU if present, or use first one available.
        // This covers most common cases (multi-gpu/integrated+dedicated graphics). Handling more
        // complicated setups (multiple dedicated GPUs) is out of scope of this sample.
        uint32_t use_gpu = 0;
        for (uint32_t i = 0; i < gpu_count; i++) {
            VkPhysicalDeviceProperties properties;
            vkGetPhysicalDeviceProperties(gpus[i], &properties);
            if (properties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU) {
                use_gpu = i;
                break;
            }
        }

        g_PhysicalDevice = gpus[use_gpu];
        free(gpus);
    }

    // Select graphics queue family
    {
        uint32_t count;
        vkGetPhysicalDeviceQueueFamilyProperties(g_PhysicalDevice, &count, nullptr);
        VkQueueFamilyProperties *queues =
            static_cast<VkQueueFamilyProperties *>(malloc(sizeof(VkQueueFamilyProperties) * count));
        vkGetPhysicalDeviceQueueFamilyProperties(g_PhysicalDevice, &count, queues);
        for (uint32_t i = 0; i < count; i++)
            if (queues[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) {
                g_QueueFamily = i;
                break;
            }
        free(queues);
        IM_ASSERT(g_QueueFamily != static_cast<uint32_t>(-1));
    }

    // Create Logical Device (with 1 queue)
    {
        uint32_t device_extension_count = 1;
        const char *device_extensions[] = {"VK_KHR_swapchain"};
        const float queue_priority[] = {1.0f};
        VkDeviceQueueCreateInfo queue_info[1] = {};
        queue_info[0].sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        queue_info[0].queueFamilyIndex = g_QueueFamily;
        queue_info[0].queueCount = 1;
        queue_info[0].pQueuePriorities = queue_priority;
        VkDeviceCreateInfo create_info = {};
        create_info.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
        create_info.queueCreateInfoCount = sizeof(queue_info) / sizeof(queue_info[0]);
        create_info.pQueueCreateInfos = queue_info;
        create_info.enabledExtensionCount = device_extension_count;
        create_info.ppEnabledExtensionNames = device_extensions;
        err = vkCreateDevice(g_PhysicalDevice, &create_info, g_Allocator, &g_Device);
        check_vk_result(err);
        vkGetDeviceQueue(g_Device, g_QueueFamily, 0, &g_Queue);
    }

    // Create Descriptor Pool
    {
        VkDescriptorPoolSize pool_sizes[] = {
            {VK_DESCRIPTOR_TYPE_SAMPLER, 1000},
            {VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1000},
            {VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, 1000},
            {VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1000},
            {VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER, 1000},
            {VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER, 1000},
            {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1000},
            {VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1000},
            {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, 1000},
            {VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC, 1000},
            {VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, 1000}};
        VkDescriptorPoolCreateInfo pool_info = {};
        pool_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
        pool_info.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
        pool_info.maxSets = 1000 * IM_ARRAYSIZE(pool_sizes);
        pool_info.poolSizeCount = (uint32_t) IM_ARRAYSIZE(pool_sizes);
        pool_info.pPoolSizes = pool_sizes;
        err = vkCreateDescriptorPool(g_Device, &pool_info, g_Allocator, &g_DescriptorPool);
        check_vk_result(err);
    }
}

RenderBackend::~RenderBackend()
{
    vkDestroyDescriptorPool(g_Device, g_DescriptorPool, g_Allocator);

#ifdef IMGUI_VULKAN_DEBUG_REPORT
    // Remove the debug report callback
    auto vkDestroyDebugReportCallbackEXT = reinterpret_cast<PFN_vkDestroyDebugReportCallbackEXT>(
        vkGetInstanceProcAddr(g_Instance, "vkDestroyDebugReportCallbackEXT"));
    vkDestroyDebugReportCallbackEXT(g_Instance, g_DebugReport, g_Allocator);
#endif // IMGUI_VULKAN_DEBUG_REPORT

    vkDestroyDevice(g_Device, g_Allocator);
    vkDestroyInstance(g_Instance, g_Allocator);
}
