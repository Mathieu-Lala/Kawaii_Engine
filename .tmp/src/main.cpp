#include <cstdio>
#include <cstdlib>
#include <fstream>

#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_vulkan.h>
#include <GLFW/glfw3.h>

#include <spdlog/spdlog.h>

#include "RenderBackend.hpp"
#include "RenderWindow.hpp"

#if defined(_MSC_VER) && (_MSC_VER >= 1900) && !defined(IMGUI_DISABLE_WIN32_FUNCTIONS)
#    pragma comment(lib, "legacy_stdio_definitions")
#endif

//#define IMGUI_UNLIMITED_FRAME_RATE

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
        backend =
            std::make_unique<RenderBackend>(std::vector<std::string>(extensions, extensions + extensions_count));

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


static std::vector<char> readFile(const std::string &filename)
{
    std::ifstream file(filename, std::ios::ate | std::ios::binary);
    if (!file.is_open()) { throw std::runtime_error("failed to open file!"); }

    size_t fileSize = static_cast<size_t>(file.tellg());
    std::vector<char> buffer(fileSize);

    file.seekg(0);
    file.read(buffer.data(), static_cast<std::streamsize>(fileSize));
    file.close();

    return buffer;
}

static VkShaderModule createShaderModule(VkDevice device, const std::vector<char> &code)
{
    VkShaderModuleCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    createInfo.codeSize = code.size();
    createInfo.pCode = reinterpret_cast<const uint32_t *>(code.data());

    VkShaderModule shaderModule;
    if (vkCreateShaderModule(device, &createInfo, nullptr, &shaderModule) != VK_SUCCESS) {
        throw std::runtime_error("failed to create shader module!");
    }

    return shaderModule;
}

int main(int, char **)
{
    Context context{};

    {
        VkAttachmentDescription colorAttachment{};
        colorAttachment.format = swapChainImageFormat;
        colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
        colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
        colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
        colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        colorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

        VkAttachmentReference colorAttachmentRef{};
        colorAttachmentRef.attachment = 0;
        colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

        VkSubpassDescription subpass{};
        subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
        subpass.colorAttachmentCount = 1;
        subpass.pColorAttachments = &colorAttachmentRef;

        VkSubpassDependency dependency{};
        dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
        dependency.dstSubpass = 0;
        dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        dependency.srcAccessMask = 0;
        dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

        VkRenderPassCreateInfo renderPassInfo{};
        renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
        renderPassInfo.attachmentCount = 1;
        renderPassInfo.pAttachments = &colorAttachment;
        renderPassInfo.subpassCount = 1;
        renderPassInfo.pSubpasses = &subpass;
        renderPassInfo.dependencyCount = 1;
        renderPassInfo.pDependencies = &dependency;

        if (vkCreateRenderPass(context.backend->g_Device, &renderPassInfo, nullptr, &context.backend->renderPass)
            != VK_SUCCESS) {
            throw std::runtime_error("failed to create render pass!");
        }
    }

    {
        auto vertShaderCode = readFile("shaders/vert.spv");
        auto fragShaderCode = readFile("shaders/frag.spv");

        VkShaderModule vertShaderModule = createShaderModule(context.backend->g_Device, vertShaderCode);
        VkShaderModule fragShaderModule = createShaderModule(context.backend->g_Device, fragShaderCode);

        VkPipelineShaderStageCreateInfo vertShaderStageInfo{};
        vertShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        vertShaderStageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
        vertShaderStageInfo.module = vertShaderModule;
        vertShaderStageInfo.pName = "main";

        VkPipelineShaderStageCreateInfo fragShaderStageInfo{};
        fragShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        fragShaderStageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
        fragShaderStageInfo.module = fragShaderModule;
        fragShaderStageInfo.pName = "main";

        VkPipelineShaderStageCreateInfo shaderStages[] = {vertShaderStageInfo, fragShaderStageInfo};

        VkPipelineVertexInputStateCreateInfo vertexInputInfo{};
        vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
        vertexInputInfo.vertexBindingDescriptionCount = 0;
        vertexInputInfo.vertexAttributeDescriptionCount = 0;

        VkPipelineInputAssemblyStateCreateInfo inputAssembly{};
        inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
        inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
        inputAssembly.primitiveRestartEnable = VK_FALSE;

        VkViewport viewport{};
        viewport.x = 0.0f;
        viewport.y = 0.0f;
        viewport.width = static_cast<float>(context.renderWindow->wd.Width);
        viewport.height = static_cast<float>(context.renderWindow->wd.Height);
        viewport.minDepth = 0.0f;
        viewport.maxDepth = 1.0f;

        VkRect2D scissor{};
        scissor.offset = {0, 0};
        scissor.extent = VkExtent2D{
            static_cast<uint32_t>(context.renderWindow->wd.Width),
            static_cast<uint32_t>(context.renderWindow->wd.Height)};

        VkPipelineViewportStateCreateInfo viewportState{};
        viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
        viewportState.viewportCount = 1;
        viewportState.pViewports = &viewport;
        viewportState.scissorCount = 1;
        viewportState.pScissors = &scissor;

        VkPipelineRasterizationStateCreateInfo rasterizer{};
        rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
        rasterizer.depthClampEnable = VK_FALSE;
        rasterizer.rasterizerDiscardEnable = VK_FALSE;
        rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
        rasterizer.lineWidth = 1.0f;
        rasterizer.cullMode = VK_CULL_MODE_BACK_BIT;
        rasterizer.frontFace = VK_FRONT_FACE_CLOCKWISE;
        rasterizer.depthBiasEnable = VK_FALSE;

        VkPipelineMultisampleStateCreateInfo multisampling{};
        multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
        multisampling.sampleShadingEnable = VK_FALSE;
        multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

        VkPipelineColorBlendAttachmentState colorBlendAttachment{};
        colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT
                                              | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
        colorBlendAttachment.blendEnable = VK_FALSE;

        VkPipelineColorBlendStateCreateInfo colorBlending{};
        colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
        colorBlending.logicOpEnable = VK_FALSE;
        colorBlending.logicOp = VK_LOGIC_OP_COPY;
        colorBlending.attachmentCount = 1;
        colorBlending.pAttachments = &colorBlendAttachment;
        colorBlending.blendConstants[0] = 0.0f;
        colorBlending.blendConstants[1] = 0.0f;
        colorBlending.blendConstants[2] = 0.0f;
        colorBlending.blendConstants[3] = 0.0f;

        VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
        pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
        pipelineLayoutInfo.setLayoutCount = 0;
        pipelineLayoutInfo.pushConstantRangeCount = 0;

        if (vkCreatePipelineLayout(
                context.backend->g_Device, &pipelineLayoutInfo, nullptr, &context.backend->pipelineLayout)
            != VK_SUCCESS) {
            throw std::runtime_error("failed to create pipeline layout!");
        }

        VkGraphicsPipelineCreateInfo pipelineInfo{};
        pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
        pipelineInfo.stageCount = 2;
        pipelineInfo.pStages = shaderStages;
        pipelineInfo.pVertexInputState = &vertexInputInfo;
        pipelineInfo.pInputAssemblyState = &inputAssembly;
        pipelineInfo.pViewportState = &viewportState;
        pipelineInfo.pRasterizationState = &rasterizer;
        pipelineInfo.pMultisampleState = &multisampling;
        pipelineInfo.pColorBlendState = &colorBlending;
        pipelineInfo.layout = context.backend->pipelineLayout;
        pipelineInfo.renderPass = context.backend->renderPass;
        pipelineInfo.subpass = 0;
        pipelineInfo.basePipelineHandle = VK_NULL_HANDLE;

        if (vkCreateGraphicsPipelines(
                context.backend->g_Device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &graphicsPipeline)
            != VK_SUCCESS) {
            throw std::runtime_error("failed to create graphics pipeline!");
        }

        vkDestroyShaderModule(context.backend->g_Device, fragShaderModule, nullptr);
        vkDestroyShaderModule(context.backend->g_Device, vertShaderModule, nullptr);
    }


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
