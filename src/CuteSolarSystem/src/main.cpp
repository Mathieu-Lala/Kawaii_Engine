#include <cstdio>
#include <cstdlib>

#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_vulkan.h>
#include <GLFW/glfw3.h>
#include <vulkan/vulkan.h>

#include <spdlog/spdlog.h>

#include "RenderBackend.hpp"
#include "RenderWindow.hpp"

#if defined(_MSC_VER) && (_MSC_VER >= 1900) && !defined(IMGUI_DISABLE_WIN32_FUNCTIONS)
#    pragma comment(lib, "legacy_stdio_definitions")
#endif

//#define IMGUI_UNLIMITED_FRAME_RATE
#ifdef _DEBUG
#    define IMGUI_VULKAN_DEBUG_REPORT
#endif

#ifdef IMGUI_VULKAN_DEBUG_REPORT
static VKAPI_ATTR VkBool32 VKAPI_CALL debug_report(
    VkDebugReportFlagsEXT flags,
    VkDebugReportObjectTypeEXT objectType,
    uint64_t object,
    size_t location,
    int32_t messageCode,
    const char *pLayerPrefix,
    const char *pMessage,
    void *pUserData)
{
    (void) flags;
    (void) object;
    (void) location;
    (void) messageCode;
    (void) pUserData;
    (void) pLayerPrefix; // Unused arguments
    fprintf(stderr, "[vulkan] Debug report from ObjectType: %i\nMessage: %s\n\n", objectType, pMessage);
    return VK_FALSE;
}
#endif // IMGUI_VULKAN_DEBUG_REPORT


static void check_vk_result(VkResult err)
{
    if (err == 0) return;
    spdlog::error("[vulkan] Error: VkResult = {}\n", err);
    if (err < 0) std::abort();
}

struct Context {
    GLFWwindow *window;
    std::unique_ptr<RenderBackend> backend;
    std::unique_ptr<RenderWindow> renderWindow;

    Context()
    {
        glfwSetErrorCallback([](int error, const char *description) {
            spdlog::error("Glfw Error {}: {}\n", error, description);
        });
        if (!glfwInit()) { std::exit(1); }

        glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
        window = glfwCreateWindow(1280, 720, "Dear ImGui GLFW+Vulkan example", nullptr, nullptr);

        if (!glfwVulkanSupported()) {
            spdlog::error("GLFW: Vulkan Not Supported\n");
            std::exit(1);
        }
        uint32_t extensions_count = 0;
        const auto extensions = glfwGetRequiredInstanceExtensions(&extensions_count);
        backend = std::make_unique<RenderBackend>(extensions, extensions_count);

        VkSurfaceKHR surface;
        VkResult err = glfwCreateWindowSurface(backend->g_Instance, window, backend->g_Allocator, &surface);
        check_vk_result(err);

        // Create Framebuffers
        int w;
        int h;
        glfwGetFramebufferSize(window, &w, &h);
        renderWindow = std::make_unique<RenderWindow>(*backend, surface, w, h);

        // Setup Dear ImGui context
        IMGUI_CHECKVERSION();
        ImGui::CreateContext();
        [[maybe_unused]] ImGuiIO &io = ImGui::GetIO();
        // io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
        // io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls

        ImGui::StyleColorsDark();
        // ImGui::StyleColorsClassic();


        ImGui_ImplGlfw_InitForVulkan(window, true);
        ImGui_ImplVulkan_InitInfo init_info = {
            .Instance = backend->g_Instance,
            .PhysicalDevice = backend->g_PhysicalDevice,
            .Device = backend->g_Device,
            .QueueFamily = backend->g_QueueFamily,
            .Queue = backend->g_Queue,
            .PipelineCache = backend->g_PipelineCache,
            .DescriptorPool = backend->g_DescriptorPool,
            .Subpass = 0,
            .MinImageCount = renderWindow->g_MinImageCount,
            .ImageCount = renderWindow->wd.ImageCount,
            .MSAASamples = VK_SAMPLE_COUNT_1_BIT,
            .Allocator = backend->g_Allocator,
            .CheckVkResultFn = check_vk_result,
        };
        ImGui_ImplVulkan_Init(&init_info, renderWindow->wd.RenderPass);

        // Load Fonts
        // - If no fonts are loaded, dear imgui will use the default font. You can also load multiple fonts and
        // use ImGui::PushFont()/PopFont() to select them.
        // - AddFontFromFileTTF() will return the ImFont* so you can store it if you need to select the font among
        // multiple.
        // - If the file cannot be loaded, the function will return NULL. Please handle those errors in your
        // application (e.g. use an assertion, or display an error and quit).
        // - The fonts will be rasterized at a given size (w/ oversampling) and stored into a texture when calling
        // ImFontAtlas::Build()/GetTexDataAsXXXX(), which ImGui_ImplXXXX_NewFrame below will call.
        // - Read 'docs/FONTS.md' for more instructions and details.
        // - Remember that in C/C++ if you want to include a backslash \ in a string literal you need to write a
        // double backslash \\ !
        // io.Fonts->AddFontDefault();
        // io.Fonts->AddFontFromFileTTF("../../misc/fonts/Roboto-Medium.ttf", 16.0f);
        // io.Fonts->AddFontFromFileTTF("../../misc/fonts/Cousine-Regular.ttf", 15.0f);
        // io.Fonts->AddFontFromFileTTF("../../misc/fonts/DroidSans.ttf", 16.0f);
        // io.Fonts->AddFontFromFileTTF("../../misc/fonts/ProggyTiny.ttf", 10.0f);
        // ImFont* font = io.Fonts->AddFontFromFileTTF("c:\\Windows\\Fonts\\ArialUni.ttf", 18.0f, NULL,
        // io.Fonts->GetGlyphRangesJapanese()); IM_ASSERT(font != NULL);

        // Upload Fonts
        {
            // Use any command queue
            VkCommandPool command_pool = renderWindow->wd.Frames[renderWindow->wd.FrameIndex].CommandPool;
            VkCommandBuffer command_buffer = renderWindow->wd.Frames[renderWindow->wd.FrameIndex].CommandBuffer;

            err = vkResetCommandPool(backend->g_Device, command_pool, 0);
            check_vk_result(err);
            VkCommandBufferBeginInfo begin_info = {};
            begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
            begin_info.flags |= VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
            err = vkBeginCommandBuffer(command_buffer, &begin_info);
            check_vk_result(err);

            ImGui_ImplVulkan_CreateFontsTexture(command_buffer);

            VkSubmitInfo end_info = {};
            end_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
            end_info.commandBufferCount = 1;
            end_info.pCommandBuffers = &command_buffer;
            err = vkEndCommandBuffer(command_buffer);
            check_vk_result(err);
            err = vkQueueSubmit(backend->g_Queue, 1, &end_info, VK_NULL_HANDLE);
            check_vk_result(err);

            err = vkDeviceWaitIdle(backend->g_Device);
            check_vk_result(err);
            ImGui_ImplVulkan_DestroyFontUploadObjects();
        }
    }

    ~Context()
    {
        const auto err = vkDeviceWaitIdle(backend->g_Device);
        check_vk_result(err);
        ImGui_ImplVulkan_Shutdown();
        ImGui_ImplGlfw_Shutdown();
        ImGui::DestroyContext();

        glfwDestroyWindow(window);
        glfwTerminate();
    }
};

int main(int, char **)
{
    Context context;

    bool show_demo_window = true;
    bool show_another_window = false;
    ImVec4 clear_color = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);

    while (!glfwWindowShouldClose(context.window)) {
        // Poll and handle events (inputs, window resize, etc.)
        // You can read the io.WantCaptureMouse, io.WantCaptureKeyboard flags to tell if dear imgui wants to use your inputs.
        // - When io.WantCaptureMouse is true, do not dispatch mouse input data to your main application.
        // - When io.WantCaptureKeyboard is true, do not dispatch keyboard input data to your main application.
        // Generally you may always pass all inputs to dear imgui, and hide them from your application based on those two flags.
        glfwPollEvents();

        if (context.renderWindow->g_SwapChainRebuild) {
            int width, height;
            glfwGetFramebufferSize(context.window, &width, &height);
            if (width > 0 && height > 0) {
                ImGui_ImplVulkan_SetMinImageCount(context.renderWindow->g_MinImageCount);
                ImGui_ImplVulkanH_CreateOrResizeWindow(
                    context.backend->g_Instance,
                    context.backend->g_PhysicalDevice,
                    context.backend->g_Device,
                    &context.renderWindow->wd,
                    context.backend->g_QueueFamily,
                    context.backend->g_Allocator,
                    width,
                    height,
                    context.renderWindow->g_MinImageCount);
                context.renderWindow->wd.FrameIndex = 0;
                context.renderWindow->g_SwapChainRebuild = false;
            }
        }

        ImGui_ImplVulkan_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        if (show_demo_window) ImGui::ShowDemoWindow(&show_demo_window);

        {
            static float f = 0.0f;
            static int counter = 0;

            ImGui::Begin("Hello, world!");

            ImGui::Text("This is some useful text.");
            ImGui::Checkbox("Demo Window", &show_demo_window);
            ImGui::Checkbox("Another Window", &show_another_window);

            ImGui::SliderFloat("float", &f, 0.0f, 1.0f);
            ImGui::ColorEdit3("clear color", reinterpret_cast<float *>(&clear_color));

            if (ImGui::Button("Button")) counter++;
            ImGui::SameLine();
            ImGui::Text("counter = %d", counter);

            ImGui::Text(
                "Application average %.3f ms/frame (%.1f FPS)",
                1000.0 / static_cast<double>(ImGui::GetIO().Framerate),
                static_cast<double>(ImGui::GetIO().Framerate));
            ImGui::End();
        }

        if (show_another_window) {
            ImGui::Begin("Another Window", &show_another_window);
            ImGui::Text("Hello from another window!");
            if (ImGui::Button("Close Me")) show_another_window = false;
            ImGui::End();
        }

        ImGui::Render();
        ImDrawData *draw_data = ImGui::GetDrawData();
        const bool is_minimized = (draw_data->DisplaySize.x <= 0.0f || draw_data->DisplaySize.y <= 0.0f);
        if (!is_minimized) {
            context.renderWindow->wd.ClearValue.color.float32[0] = clear_color.x * clear_color.w;
            context.renderWindow->wd.ClearValue.color.float32[1] = clear_color.y * clear_color.w;
            context.renderWindow->wd.ClearValue.color.float32[2] = clear_color.z * clear_color.w;
            context.renderWindow->wd.ClearValue.color.float32[3] = clear_color.w;
            context.renderWindow->FrameRender(draw_data);
            context.renderWindow->FramePresent();
        }
    }

    return 0;
}
