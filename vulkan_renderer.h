#pragma once
#include <vulkan/vulkan.h>
#include <fstream>

#define VK_CHECK_FATAL(result)                                     \
    if (result != VK_SUCCESS)                                      \
    {                                                              \
        std::cerr << "Vulkan Error Code: " << result << std::endl; \
        return false;                                              \
    }

#define VK_CHECK(result)                                           \
    if (result != VK_SUCCESS)                                      \
    {                                                              \
        std::cerr << "Vulkan Error Code: " << result << std::endl; \
    }

#define ArraySize(arr) sizeof((arr)) / sizeof((arr[0]))
#define INVALID_IDX UINT32_MAX

static uint32_t vk_get_memory_type_index(
    VkPhysicalDevice gpu,
    VkMemoryRequirements memRequirements,
    VkMemoryPropertyFlags memProps)
{
    uint32_t typeIdx = INVALID_IDX;

    VkPhysicalDeviceMemoryProperties gpuMemProps;
    vkGetPhysicalDeviceMemoryProperties(gpu, &gpuMemProps);

    VkMemoryAllocateInfo allocInfo = {};
    allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocInfo.allocationSize = memRequirements.size;
    for (uint32_t i = 0; i < gpuMemProps.memoryTypeCount; i++)
    {
        if (memRequirements.memoryTypeBits & (1 << i) &&
            (gpuMemProps.memoryTypes[i].propertyFlags & memProps) == memProps)
        {
            typeIdx = i;
            break;
        }
    }

    if (typeIdx == INVALID_IDX)
    {
        std::cerr << "Failed to find proper type Index for Memory Properties: " << memProps << std::endl;
    }

    return typeIdx;
}

struct Buffer
{
    VkBuffer buffer;
    VkDeviceMemory memory;
    uint32_t size;
    void *data;
};

static Buffer vk_allocate_buffer(
    VkDevice device,
    VkPhysicalDevice gpu,
    uint32_t size,
    VkBufferUsageFlags bufferUsage,
    VkMemoryPropertyFlags memProps)
{
    Buffer buffer = {};
    buffer.size = size;

    VkBufferCreateInfo bufferInfo = {};
    bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bufferInfo.usage = bufferUsage;
    bufferInfo.size = size;
    VK_CHECK(vkCreateBuffer(device, &bufferInfo, 0, &buffer.buffer));

    VkMemoryRequirements memRequirements;
    vkGetBufferMemoryRequirements(device, buffer.buffer, &memRequirements);

    VkMemoryAllocateInfo allocInfo = {};
    allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocInfo.allocationSize = memRequirements.size;
    allocInfo.memoryTypeIndex = vk_get_memory_type_index(gpu, memRequirements, memProps);

    VK_CHECK(vkAllocateMemory(device, &allocInfo, 0, &buffer.memory));

    // Only map memory we can actually write to from the CPU
    if (memProps & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT)
    {
        VK_CHECK(vkMapMemory(device, buffer.memory, 0, memRequirements.size, 0, &buffer.data));
    }

    VK_CHECK(vkBindBufferMemory(device, buffer.buffer, buffer.memory, 0));

    return buffer;
}

void vk_copy_to_buffer(Buffer *buffer, const void *data, uint32_t size)
{
    if (buffer->size >= size)
    {
        // If we have mapped data
        if (buffer->data)
        {
            memcpy(buffer->data, data, size);
        }
        else
        {
            // TODO: Implement GPU Only buffers
        }
    }
    else
    {
        std::cerr << "Buffer too small: " << buffer->size << " for data: " << size << std::endl;
    }
}

static VKAPI_ATTR VkBool32 VKAPI_CALL vk_debug_callback(
    VkDebugUtilsMessageSeverityFlagBitsEXT msgSeverity,
    VkDebugUtilsMessageTypeFlagsEXT msgFlags,
    const VkDebugUtilsMessengerCallbackDataEXT *pCallbackData,
    void *pUserData)
{
    std::cerr << pCallbackData->pMessage << std::endl;
    return false;
}

static std::vector<char> read_file(const std::string &filename)
{
    std::ifstream file(filename, std::ios::ate | std::ios::binary);

    if (!file.is_open())
    {
        throw std::runtime_error("failed to open file!");
    }
    size_t fileSize = (size_t)file.tellg();
    std::vector<char> buffer(fileSize);

    file.seekg(0);
    file.read(buffer.data(), fileSize);

    file.close();

    return buffer;
}

static void compile_shader(const char *input, const char *output)
{
    char command[500] = {};
    sprintf(command, "shaders_vulkan\\glslc.exe %s -o %s", input, output);
    if (system(command))
    {
        std::cerr << "Failed to compile Shader: " << input << std::endl;
        __debugbreak();
    }
}

static VkCommandBufferAllocateInfo cmd_alloc_info(VkCommandPool pool)
{
    VkCommandBufferAllocateInfo info = {};
    info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    info.commandBufferCount = 1;
    info.commandPool = pool;
    info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;

    return info;
}

static VkFenceCreateInfo fence_info(VkFenceCreateFlags flags = 0)
{
    VkFenceCreateInfo info = {};
    info.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    info.flags = flags;
    return info;
}

static VkDescriptorSetLayoutBinding layout_binding(
    VkDescriptorType type,
    VkShaderStageFlags shaderStages,
    uint32_t count,
    uint32_t bindingNumber)
{
    VkDescriptorSetLayoutBinding binding = {};
    binding.binding = bindingNumber;
    binding.descriptorCount = count;
    binding.descriptorType = type;
    binding.stageFlags = shaderStages;

    return binding;
}

struct VkContext
{
    VkInstance instance;
    VkDebugUtilsMessengerEXT debugMessenger;
    VkSurfaceKHR surface;
    VkSurfaceFormatKHR surfaceFormat;
    VkPhysicalDevice gpu;
    VkDevice device;
    VkQueue graphicsQueue;
    VkSwapchainKHR swapchain;
    VkRenderPass renderPass;
    VkCommandPool commandPool;
    VkCommandBuffer cmd;

    VkDescriptorPool descPool;
    VkDescriptorSetLayout setLayout;
    VkDescriptorSet descSet;
    VkPipelineLayout pipeLayout;
    VkPipeline pipeline;

    // Buffers
    Buffer globalUBO;
    Buffer vertexBuffer;
    Buffer indexBuffer;

    // Sync Objects
    VkSemaphore aquireSemaphore;
    VkSemaphore submitSemaphore;
    VkFence imgAvailableFence;

    uint32_t scImgCount;
    // TODO: Suballocation from Main Memory
    VkImage scImages[5];
    VkImageView scImgViews[5];
    VkFramebuffer framebuffers[5];

    int graphicsIdx;
};

static VkContext vkcontext;

bool init_vulkan(GLFWwindow *glfwWindow)
{
    // Compile both Shaders
    compile_shader("shaders_vulkan/modelViewProj.vert",
                   "shaders_vulkan/modelViewProj.vert.spv");

    compile_shader("shaders_vulkan/color.frag",
                   "shaders_vulkan/color.frag.spv");

    // Instance
    {
        VkApplicationInfo appInfo = {};
        appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
        appInfo.apiVersion = VK_API_VERSION_1_0;

        // GLFW Extensions for Vulkan
        uint32_t glfwExtensionCount = 0;
        const char **glfwExtensions;
        glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);

        char *layers[]{
            "VK_LAYER_KHRONOS_validation"};

        VkInstanceCreateInfo info = {};
        info.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
        info.pApplicationInfo = &appInfo;
        info.enabledExtensionCount = glfwExtensionCount;
        info.ppEnabledExtensionNames = glfwExtensions;
        info.ppEnabledLayerNames = layers;
        info.enabledLayerCount = ArraySize(layers);

        VK_CHECK_FATAL(vkCreateInstance(&info, 0, &vkcontext.instance));
    }

    // Debug Utils
    {
        // auto vkCreateDebugUtilsMessengerEXT = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(vkcontext.instance, "vkCreateDebugUtilsMessengerEXT");

        // if (vkCreateDebugUtilsMessengerEXT)
        // {
        //     VkDebugUtilsMessengerCreateInfoEXT debugInfo = {};
        //     debugInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
        //     debugInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT;
        //     debugInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT;
        //     debugInfo.pfnUserCallback = vk_debug_callback;

        //     vkCreateDebugUtilsMessengerEXT(vkcontext.instance, &debugInfo, 0, &vkcontext.debugMessenger);
        // }
        // else
        // {
        //     std::cerr << "Failed to create Vulkan Debug Messanger" << std::endl;
        //     return false;
        // }
    }

    // Surface
    {
        if (glfwCreateWindowSurface(vkcontext.instance, glfwWindow, nullptr, &vkcontext.surface) != VK_SUCCESS)
        {
            std::cerr << "failed to create window surface!" << std::endl;
            return false;
        }
    }

    // Choose GPU
    {
        vkcontext.graphicsIdx = -1;

        uint32_t gpuCount = 0;
        VkPhysicalDevice gpus[10];
        VK_CHECK_FATAL(vkEnumeratePhysicalDevices(vkcontext.instance, &gpuCount, 0));
        VK_CHECK_FATAL(vkEnumeratePhysicalDevices(vkcontext.instance, &gpuCount, gpus));

        for (uint32_t i = 0; i < gpuCount; i++)
        {
            VkPhysicalDevice gpu = gpus[i];

            uint32_t queueFamilyCount = 0;
            VkQueueFamilyProperties queueProps[10];
            vkGetPhysicalDeviceQueueFamilyProperties(gpu, &queueFamilyCount, 0);
            vkGetPhysicalDeviceQueueFamilyProperties(gpu, &queueFamilyCount, queueProps);

            for (uint32_t j = 0; j < queueFamilyCount; j++)
            {
                if (queueProps[j].queueFlags & VK_QUEUE_GRAPHICS_BIT)
                {
                    VkBool32 surfaceSupport = VK_FALSE;
                    VK_CHECK_FATAL(vkGetPhysicalDeviceSurfaceSupportKHR(gpu, j, vkcontext.surface, &surfaceSupport));

                    if (surfaceSupport)
                    {
                        vkcontext.graphicsIdx = j;
                        vkcontext.gpu = gpu;
                        break;
                    }
                }
            }
        }

        if (vkcontext.graphicsIdx < 0)
        {
            return false;
        }
    }

    // Logical Device
    {
        float queuePriority = 1.0f;

        VkDeviceQueueCreateInfo queueInfo = {};
        queueInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        queueInfo.queueFamilyIndex = vkcontext.graphicsIdx;
        queueInfo.queueCount = 1;
        queueInfo.pQueuePriorities = &queuePriority;

        char *extensions[] = {
            VK_KHR_SWAPCHAIN_EXTENSION_NAME};

        VkDeviceCreateInfo deviceInfo = {};
        deviceInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
        deviceInfo.pQueueCreateInfos = &queueInfo;
        deviceInfo.queueCreateInfoCount = 1;
        deviceInfo.ppEnabledExtensionNames = extensions;
        deviceInfo.enabledExtensionCount = ArraySize(extensions);

        VK_CHECK_FATAL(vkCreateDevice(vkcontext.gpu, &deviceInfo, 0, &vkcontext.device));

        // Get Graphics Queue
        vkGetDeviceQueue(vkcontext.device, vkcontext.graphicsIdx, 0, &vkcontext.graphicsQueue);
    }

    // Swapchain
    {
        uint32_t formatCount = 0;
        VkSurfaceFormatKHR surfaceFormats[10];
        VK_CHECK_FATAL(vkGetPhysicalDeviceSurfaceFormatsKHR(vkcontext.gpu, vkcontext.surface, &formatCount, 0));
        VK_CHECK_FATAL(vkGetPhysicalDeviceSurfaceFormatsKHR(vkcontext.gpu, vkcontext.surface, &formatCount, surfaceFormats));

        for (uint32_t i = 0; i < formatCount; i++)
        {
            VkSurfaceFormatKHR format = surfaceFormats[i];

            if (format.format == VK_FORMAT_B8G8R8A8_SRGB)
            {
                vkcontext.surfaceFormat = format;
                break;
            }
        }

        VkSurfaceCapabilitiesKHR surfaceCaps = {};
        VK_CHECK_FATAL(vkGetPhysicalDeviceSurfaceCapabilitiesKHR(vkcontext.gpu, vkcontext.surface, &surfaceCaps));
        uint32_t imgCount = surfaceCaps.minImageCount + 1;
        imgCount = imgCount > surfaceCaps.maxImageCount ? imgCount - 1 : imgCount;

        VkSwapchainCreateInfoKHR scInfo = {};
        scInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
        scInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
        scInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
        scInfo.surface = vkcontext.surface;
        scInfo.imageFormat = vkcontext.surfaceFormat.format;
        scInfo.preTransform = surfaceCaps.currentTransform;
        scInfo.imageExtent = surfaceCaps.currentExtent;
        scInfo.presentMode = VK_PRESENT_MODE_FIFO_KHR;
        scInfo.minImageCount = imgCount;
        scInfo.imageArrayLayers = 1;

        VK_CHECK_FATAL(vkCreateSwapchainKHR(vkcontext.device, &scInfo, 0, &vkcontext.swapchain));

        VK_CHECK_FATAL(vkGetSwapchainImagesKHR(vkcontext.device, vkcontext.swapchain, &vkcontext.scImgCount, 0));
        VK_CHECK_FATAL(vkGetSwapchainImagesKHR(vkcontext.device, vkcontext.swapchain, &vkcontext.scImgCount, vkcontext.scImages));

        VkImageViewCreateInfo viewInfo = {};
        viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        viewInfo.format = vkcontext.surfaceFormat.format;
        viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
        viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        viewInfo.subresourceRange.levelCount = 1;
        viewInfo.subresourceRange.layerCount = 1;
        for (uint32_t i = 0; i < vkcontext.scImgCount; i++)
        {
            viewInfo.image = vkcontext.scImages[i];
            VK_CHECK_FATAL(vkCreateImageView(vkcontext.device, &viewInfo, 0, &vkcontext.scImgViews[i]));
        }
    }

    // Render Pass
    {
        VkAttachmentDescription colorAttachment = {};
        colorAttachment.format = vkcontext.surfaceFormat.format;
        colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
        colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
        colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
        colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        colorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

        VkAttachmentDescription attachments[] = {
            colorAttachment};

        VkAttachmentReference colorAttachmentRef = {};
        colorAttachmentRef.attachment = 0; // This is an index into the attachments array
        colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

        VkSubpassDescription subpassDesc = {};
        subpassDesc.colorAttachmentCount = 1;
        subpassDesc.pColorAttachments = &colorAttachmentRef;

        VkRenderPassCreateInfo rpInfo = {};
        rpInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
        rpInfo.pAttachments = attachments;
        rpInfo.attachmentCount = ArraySize(attachments);
        rpInfo.subpassCount = 1;
        rpInfo.pSubpasses = &subpassDesc;

        VK_CHECK_FATAL(vkCreateRenderPass(vkcontext.device, &rpInfo, 0, &vkcontext.renderPass));
    }

    // Frame Buffers
    {
        VkFramebufferCreateInfo fbInfo = {};
        fbInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        fbInfo.renderPass = vkcontext.renderPass;
        fbInfo.width = SCREEN_WIDTH;
        fbInfo.height = SCREEN_HEIGHT;
        fbInfo.layers = 1;
        fbInfo.attachmentCount = 1;

        for (uint32_t i = 0; i < vkcontext.scImgCount; i++)
        {
            //HERE: Instead of using swapchain Image Views, prolly use other Image Views
            fbInfo.pAttachments = &vkcontext.scImgViews[i];
            VK_CHECK_FATAL(vkCreateFramebuffer(vkcontext.device, &fbInfo, 0, &vkcontext.framebuffers[i]));
        }
    }

    // Command Pool
    {
        VkCommandPoolCreateInfo poolInfo = {};
        poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
        poolInfo.queueFamilyIndex = vkcontext.graphicsIdx;
        poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
        VK_CHECK_FATAL(vkCreateCommandPool(vkcontext.device, &poolInfo, 0, &vkcontext.commandPool));
    }

    // Command Buffer
    {
        VkCommandBufferAllocateInfo allocInfo = cmd_alloc_info(vkcontext.commandPool);
        VK_CHECK_FATAL(vkAllocateCommandBuffers(vkcontext.device, &allocInfo, &vkcontext.cmd));
    }

    // Sync Objects
    {
        VkSemaphoreCreateInfo semaInfo = {};
        semaInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
        VK_CHECK_FATAL(vkCreateSemaphore(vkcontext.device, &semaInfo, 0, &vkcontext.aquireSemaphore));
        VK_CHECK_FATAL(vkCreateSemaphore(vkcontext.device, &semaInfo, 0, &vkcontext.submitSemaphore));

        VkFenceCreateInfo fenceInfo = fence_info(VK_FENCE_CREATE_SIGNALED_BIT);
        VK_CHECK_FATAL(vkCreateFence(vkcontext.device, &fenceInfo, 0, &vkcontext.imgAvailableFence));
    }

    // Create Descriptor Set Layouts
    {
        VkDescriptorSetLayoutBinding layoutBindings[] = {
            layout_binding(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_VERTEX_BIT, 1, 0),
            layout_binding(VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_VERTEX_BIT, 1, 1)};

        VkDescriptorSetLayoutCreateInfo layoutInfo = {};
        layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
        layoutInfo.bindingCount = ArraySize(layoutBindings);
        layoutInfo.pBindings = layoutBindings;

        VK_CHECK_FATAL(vkCreateDescriptorSetLayout(vkcontext.device, &layoutInfo, 0, &vkcontext.setLayout));
    }

    //HERE: This will need to be duplicated, line 506 - 655
    // This could be a function create_pipeline(bool frontFaceCulling, ...)
    // Create Pipeline Layout
    {
        VkPushConstantRange pushConstant = {};

        VkPipelineLayoutCreateInfo layoutInfo = {};
        layoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
        layoutInfo.setLayoutCount = 1;
        layoutInfo.pSetLayouts = &vkcontext.setLayout;
        VK_CHECK_FATAL(vkCreatePipelineLayout(vkcontext.device, &layoutInfo, 0, &vkcontext.pipeLayout));
    }

    // Create a Pipeline
    {
        // Bindings
        VkVertexInputBindingDescription bindingDescription = {};
        bindingDescription.binding = 0; // Index
        bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
        bindingDescription.stride = sizeof(VertexColor);

        // Attributes
        VkVertexInputAttributeDescription posDescription = {};
        posDescription.binding = 0;
        posDescription.location = 0;
        posDescription.offset = offsetof(VertexColor, position);
        posDescription.format = VK_FORMAT_R32G32B32_SFLOAT;

        VkVertexInputAttributeDescription colorDescription = {};
        colorDescription.binding = 0;
        colorDescription.location = 1;
        colorDescription.offset = offsetof(VertexColor, color);
        colorDescription.format = VK_FORMAT_R32G32B32_SFLOAT;

        VkVertexInputAttributeDescription attributeDescriptions[2] = {
            posDescription,
            colorDescription};

        VkPipelineVertexInputStateCreateInfo vertexInputState = {};
        vertexInputState.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
        vertexInputState.pVertexAttributeDescriptions = attributeDescriptions;
        vertexInputState.vertexAttributeDescriptionCount = ArraySize(attributeDescriptions);
        vertexInputState.pVertexBindingDescriptions = &bindingDescription;
        vertexInputState.vertexBindingDescriptionCount = 1;

        VkPipelineColorBlendAttachmentState colorBlendAttachment = {};
        colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
        colorBlendAttachment.blendEnable = VK_FALSE;

        VkPipelineColorBlendStateCreateInfo colorBlendState = {};
        colorBlendState.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
        colorBlendState.pAttachments = &colorBlendAttachment;
        colorBlendState.attachmentCount = 1;

        VkPipelineInputAssemblyStateCreateInfo inputAssembly = {};
        inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
        inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;

        VkViewport viewport = {};
        viewport.maxDepth = 1.0;

        VkRect2D scissor = {};

        VkPipelineViewportStateCreateInfo viewportState = {};
        viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
        viewportState.pViewports = &viewport;
        viewportState.viewportCount = 1;
        viewportState.pScissors = &scissor;
        viewportState.scissorCount = 1;

        VkPipelineRasterizationStateCreateInfo rasterizationState = {};
        rasterizationState.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
        rasterizationState.cullMode = VK_CULL_MODE_FRONT_BIT;
        rasterizationState.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
        rasterizationState.polygonMode = VK_POLYGON_MODE_FILL;
        rasterizationState.lineWidth = 1.0f;

        VkPipelineMultisampleStateCreateInfo multisampleState = {};
        multisampleState.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
        multisampleState.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

        VkShaderModule vertexShader, fragmentShader;

        // Vertex Shader
        std::vector<char> vertexCode = read_file("shaders_vulkan/modelViewProj.vert.spv");
        {
            uint32_t lengthInBytes = vertexCode.size();

            VkShaderModuleCreateInfo shaderInfo = {};
            shaderInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
            shaderInfo.pCode = (uint32_t *)vertexCode.data();
            shaderInfo.codeSize = lengthInBytes;
            VK_CHECK_FATAL(vkCreateShaderModule(vkcontext.device, &shaderInfo, 0, &vertexShader));
        }

        // Fragment Shader
        std::vector<char> fragmentCode = read_file("shaders_vulkan/color.frag.spv");
        {
            uint32_t lengthInBytes = fragmentCode.size();

            VkShaderModuleCreateInfo shaderInfo = {};
            shaderInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
            shaderInfo.pCode = (uint32_t *)fragmentCode.data();
            shaderInfo.codeSize = lengthInBytes;
            VK_CHECK_FATAL(vkCreateShaderModule(vkcontext.device, &shaderInfo, 0, &fragmentShader));
        }

        VkPipelineShaderStageCreateInfo vertStage = {};
        vertStage.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        vertStage.stage = VK_SHADER_STAGE_VERTEX_BIT;
        vertStage.pName = "main";
        vertStage.module = vertexShader;

        VkPipelineShaderStageCreateInfo fragStage = {};
        fragStage.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        fragStage.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
        fragStage.pName = "main";
        fragStage.module = fragmentShader;

        VkPipelineShaderStageCreateInfo shaderStages[2]{
            vertStage,
            fragStage};

        VkDynamicState dynamicStates[]{
            VK_DYNAMIC_STATE_VIEWPORT,
            VK_DYNAMIC_STATE_SCISSOR};

        VkPipelineDynamicStateCreateInfo dynamicState = {};
        dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
        dynamicState.pDynamicStates = dynamicStates;
        dynamicState.dynamicStateCount = ArraySize(dynamicStates);

        VkGraphicsPipelineCreateInfo pipelineInfo = {};
        pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
        pipelineInfo.renderPass = vkcontext.renderPass;
        pipelineInfo.pVertexInputState = &vertexInputState;
        pipelineInfo.pColorBlendState = &colorBlendState;
        pipelineInfo.pInputAssemblyState = &inputAssembly;
        pipelineInfo.pViewportState = &viewportState;
        pipelineInfo.pRasterizationState = &rasterizationState;
        pipelineInfo.pMultisampleState = &multisampleState;
        pipelineInfo.stageCount = ArraySize(shaderStages);
        pipelineInfo.pDynamicState = &dynamicState;
        pipelineInfo.layout = vkcontext.pipeLayout;
        pipelineInfo.pStages = shaderStages;

        VK_CHECK_FATAL(vkCreateGraphicsPipelines(vkcontext.device, 0, 1, &pipelineInfo, 0, &vkcontext.pipeline));

        vkDestroyShaderModule(vkcontext.device, vertexShader, 0);
        vkDestroyShaderModule(vkcontext.device, fragmentShader, 0);
    }

    // Create Global Uniform Buffer Object
    {
        vkcontext.globalUBO = vk_allocate_buffer(
            vkcontext.device,
            vkcontext.gpu,
            sizeof(glm::mat4),
            VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
            VK_MEMORY_PROPERTY_HOST_COHERENT_BIT | VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT);

        vk_copy_to_buffer(&vkcontext.globalUBO, &MVP, sizeof(glm::mat4));
    }

    // Create Descriptor Pool
    {
        VkDescriptorPoolSize poolSizes[] = {
            {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1},
            {VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1}};

        VkDescriptorPoolCreateInfo poolInfo = {};
        poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
        poolInfo.maxSets = 2;
        poolInfo.poolSizeCount = ArraySize(poolSizes);
        poolInfo.pPoolSizes = poolSizes;
        VK_CHECK_FATAL(vkCreateDescriptorPool(vkcontext.device, &poolInfo, 0, &vkcontext.descPool));
    }

    //HERE: This would need to match the shaders used in the different pieplines, so two Sets
    // Allocate Descriptor Set (Pointers to Memory where the Buffers are)
    {
        // Allocation
        VkDescriptorSetAllocateInfo allocInfo = {};
        allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
        allocInfo.pSetLayouts = &vkcontext.setLayout;
        allocInfo.descriptorSetCount = 1;
        allocInfo.descriptorPool = vkcontext.descPool;
        VK_CHECK_FATAL(vkAllocateDescriptorSets(vkcontext.device, &allocInfo, &vkcontext.descSet));

        // Update Descriptor Set
        {
            VkDescriptorBufferInfo bufferInfo = {};
            bufferInfo.buffer = vkcontext.globalUBO.buffer;
            bufferInfo.range = sizeof(glm::mat4);

            VkWriteDescriptorSet globalUBOWrite = {};
            globalUBOWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            globalUBOWrite.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
            globalUBOWrite.descriptorCount = 1;
            globalUBOWrite.dstBinding = 0;
            globalUBOWrite.pBufferInfo = &bufferInfo;
            globalUBOWrite.dstSet = vkcontext.descSet;

            VkWriteDescriptorSet writes[] = {
                globalUBOWrite};

            vkUpdateDescriptorSets(vkcontext.device, ArraySize(writes), writes, 0, 0);
        }
    }

    // Create Vertex Buffer
    {
        vkcontext.vertexBuffer = vk_allocate_buffer(
            vkcontext.device,
            vkcontext.gpu,
            sizeof(float) * vertices.size(),
            VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

        // Copy Vertices to the buffer
        {
            vk_copy_to_buffer(&vkcontext.vertexBuffer, vertices.data(), sizeof(GLfloat) * vertices.size());
        }
    }

    // Create Index Buffer
    {
        vkcontext.indexBuffer = vk_allocate_buffer(
            vkcontext.device,
            vkcontext.gpu,
            sizeof(GLuint) * indices.size(),
            VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

        // Copy Vertices to the buffer
        {
            vk_copy_to_buffer(&vkcontext.indexBuffer, indices.data(), sizeof(GLuint) * indices.size());
        }
    }

    return true;
}

void render_scene_vulkan()
{
    uint32_t imgIdx;

    // We wait on the GPU to be done with the work
    VK_CHECK(vkWaitForFences(vkcontext.device, 1, &vkcontext.imgAvailableFence, VK_TRUE, UINT64_MAX));
    VK_CHECK(vkResetFences(vkcontext.device, 1, &vkcontext.imgAvailableFence));

    // Copy Data to buffers
    {
        vk_copy_to_buffer(&vkcontext.globalUBO, &MVP, sizeof(glm::mat4));
    }

    // This waits on the timeout until the image is ready, if timeout reached -> VK_TIMEOUT
    VK_CHECK(vkAcquireNextImageKHR(vkcontext.device, vkcontext.swapchain, UINT64_MAX, vkcontext.aquireSemaphore, 0, &imgIdx));

    VkCommandBuffer cmd = vkcontext.cmd;
    vkResetCommandBuffer(cmd, 0);

    VkCommandBufferBeginInfo beginInfo = {};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
    VK_CHECK(vkBeginCommandBuffer(cmd, &beginInfo));

    // Clear Color to Yellow
    VkClearValue clearValue = {};
    clearValue.color = {0.2f, 0.2f, 0.2f, 1};

    VkRenderPassBeginInfo rpBeginInfo = {};
    rpBeginInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    rpBeginInfo.renderArea.extent = {SCREEN_WIDTH, SCREEN_HEIGHT};
    rpBeginInfo.clearValueCount = 1;
    rpBeginInfo.pClearValues = &clearValue;
    rpBeginInfo.renderPass = vkcontext.renderPass;
    rpBeginInfo.framebuffer = vkcontext.framebuffers[imgIdx];
    vkCmdBeginRenderPass(cmd, &rpBeginInfo, VK_SUBPASS_CONTENTS_INLINE);

    VkViewport viewport = {};
    viewport.maxDepth = 1.0f;
    viewport.width = SCREEN_WIDTH;
    viewport.height = SCREEN_HEIGHT;

    VkRect2D scissor = {};
    scissor.extent.width = SCREEN_WIDTH;
    scissor.extent.height = SCREEN_HEIGHT;

    vkCmdSetViewport(cmd, 0, 1, &viewport);
    vkCmdSetScissor(cmd, 0, 1, &scissor);

    //HERE: You would bind frontFacePipeline -> vkCmdDrawIndexed
    //HERE: You would bind backFacePipeline -> vkCmdDrawIndexed
    //HERE: You would bind different Descriptor -> bind finalPipeline -> vkCmdDrawIndexed
    // Render Loop
    {
        vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, vkcontext.pipeline);

        VkDeviceSize offsets[] = {0};
        vkCmdBindVertexBuffers(cmd, 0, 1, &vkcontext.vertexBuffer.buffer, offsets);
        vkCmdBindIndexBuffer(cmd, vkcontext.indexBuffer.buffer, 0, VK_INDEX_TYPE_UINT32);

        vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, vkcontext.pipeLayout,
                                0, 1, &vkcontext.descSet, 0, 0);

        vkCmdDrawIndexed(cmd, static_cast<uint32_t>(indices.size()), 1, 0, 0, 0);
    }

    vkCmdEndRenderPass(cmd);

    VK_CHECK(vkEndCommandBuffer(cmd));

    VkPipelineStageFlags waitStage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;

    // This call will signal the Fence when the GPU Work is done
    VkSubmitInfo submitInfo = {};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &cmd;
    submitInfo.pWaitDstStageMask = &waitStage;
    submitInfo.pSignalSemaphores = &vkcontext.submitSemaphore;
    submitInfo.signalSemaphoreCount = 1;
    submitInfo.pWaitSemaphores = &vkcontext.aquireSemaphore;
    submitInfo.waitSemaphoreCount = 1;
    VK_CHECK(vkQueueSubmit(vkcontext.graphicsQueue, 1, &submitInfo, vkcontext.imgAvailableFence));

    VkPresentInfoKHR presentInfo = {};
    presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    presentInfo.pSwapchains = &vkcontext.swapchain;
    presentInfo.swapchainCount = 1;
    presentInfo.pImageIndices = &imgIdx;
    presentInfo.pWaitSemaphores = &vkcontext.submitSemaphore;
    presentInfo.waitSemaphoreCount = 1;
    vkQueuePresentKHR(vkcontext.graphicsQueue, &presentInfo);
}