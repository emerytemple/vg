
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <vulkan/vulkan.h>

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <stdbool.h>

#include "render.h"

GLFWwindow *create_window()
{
	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
	glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

	GLFWwindow *window = glfwCreateWindow(800, 600, "Vulkan", NULL, NULL);
	if(window == NULL) printf("failed to create glfw window\n ");

	return window;
}

VkInstance create_instance(bool validation_layers_enabled, uint32_t validation_layer_count, const char **validation_layers, uint32_t instance_extension_count, char **instance_extensions)
{
	VkApplicationInfo app_info = {
		.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
		.pNext = NULL,
		.pApplicationName = "Hello Triangle",
		.applicationVersion = VK_MAKE_VERSION(1, 0, 0),
		.pEngineName = "No Engine",
		.engineVersion = VK_MAKE_VERSION(1, 0, 0),
		.apiVersion = VK_API_VERSION_1_0,
	};

	VkInstanceCreateInfo instance_info = {
		.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
		.pNext = NULL,
		.flags = 0,
		.pApplicationInfo = &app_info,
		.enabledLayerCount = 0,
		.ppEnabledLayerNames = NULL,
		.enabledExtensionCount = instance_extension_count,
		.ppEnabledExtensionNames = (const char **) instance_extensions,
	};

    if (validation_layers_enabled)
	{
		VkDebugUtilsMessengerCreateInfoEXT debug_messenger_info = {
			.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT,
			.pNext = NULL,
			.flags = 0,
			.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT |
								VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
								VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT,
			.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
							VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
							VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT,
			.pfnUserCallback = debug_callback,
			.pUserData = NULL,
		};

        instance_info.pNext = (VkDebugUtilsMessengerCreateInfoEXT*) &debug_messenger_info;

        instance_info.enabledLayerCount = validation_layer_count;
        instance_info.ppEnabledLayerNames = validation_layers;
    }

    VkInstance instance;
    VkResult result = vkCreateInstance(&instance_info, NULL, &instance);
	if(result != VK_SUCCESS) printf("failed to create instance\n");

	return instance;
}

VkDebugUtilsMessengerEXT create_debug_messenger(bool validation_layers_enabled, VkInstance instance)
{
    if (!validation_layers_enabled) return VK_NULL_HANDLE;

	VkDebugUtilsMessengerCreateInfoEXT debug_messenger_info = {
		.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT,
		.pNext = NULL,
		.flags = 0,
		.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT |
							VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
							VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT,
		.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
						VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
						VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT,
		.pfnUserCallback = debug_callback,
		.pUserData = NULL,
	};

    PFN_vkCreateDebugUtilsMessengerEXT func = (PFN_vkCreateDebugUtilsMessengerEXT) vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT");
    if (func == VK_NULL_HANDLE) printf("failed to load vkCreateDebugUtilsMessengerEXT function\n");

    VkDebugUtilsMessengerEXT debug_messenger;
    VkResult result = func(instance, &debug_messenger_info, NULL, &debug_messenger);
	if(result != VK_SUCCESS) printf("failed to set up debug messenger\n");

	return debug_messenger;
}

VkSurfaceKHR create_surface(GLFWwindow *window, VkInstance instance)
{
    VkSurfaceKHR surface;

    VkResult result = glfwCreateWindowSurface(instance, window, NULL, &surface);
	if(result != VK_SUCCESS) printf("failed to create window surface\n");

	return surface;
}

VkPhysicalDevice create_physical_device(VkInstance instance, VkSurfaceKHR surface, uint32_t device_extension_count, const char **device_extensions)
{
    uint32_t device_count = 0;
    vkEnumeratePhysicalDevices(instance, &device_count, NULL);
    if (device_count == 0) printf("failed to find GPUs with Vulkan support\n");

    VkPhysicalDevice *devices = malloc(device_count * sizeof(VkPhysicalDevice));
    vkEnumeratePhysicalDevices(instance, &device_count, devices);

    VkPhysicalDevice physical_device = VK_NULL_HANDLE;
	bool is_selected_device_discrete = false;
	VkDeviceSize selected_vram = 0;

    for (int i = 0; i < device_count; i++)
	{
		VkPhysicalDeviceProperties device_properties;
		vkGetPhysicalDeviceProperties(devices[i], &device_properties);

		bool is_integrated_gpu = (device_properties.deviceType == VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU);
		bool is_discrete_gpu = (device_properties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU);

		VkPhysicalDeviceMemoryProperties memory_properties;
		vkGetPhysicalDeviceMemoryProperties(devices[i], &memory_properties);

		VkDeviceSize vram = 0;
		for(int j = 0; j < memory_properties.memoryHeapCount; j++)
		{
			VkMemoryHeap heap = memory_properties.memoryHeaps[j];

			if(heap.flags & VK_MEMORY_HEAP_DEVICE_LOCAL_BIT)
			{
				vram = heap.size; // uint64_t
				break;
			}
		}

		printf("device[%d]: %s with vram = %ld, ", i, (is_integrated_gpu) ? "integrated" : (is_discrete_gpu) ? "discreet" : "other" , vram);

		if(!is_integrated_gpu && !is_discrete_gpu)
		{
			printf("not a gpu");
		}
		else if (physical_device == VK_NULL_HANDLE)
		{
			printf("first available");

			physical_device = devices[i];
			is_selected_device_discrete = (is_discrete_gpu) ? true : false;
			selected_vram = vram;
		}
		else if (is_discrete_gpu && !is_selected_device_discrete)
		{
			printf("integrated to discrete");

			physical_device = devices[i];
			is_selected_device_discrete = true;
			selected_vram = vram;
		}
		else if(vram > selected_vram)
		{
			printf("new choice has more vram");

			physical_device = devices[i];
			is_selected_device_discrete = true;
			selected_vram = vram;
		}
		else
		{
			printf("previous selection was same or better");
		}

		printf("\n");
    }

	// printf("chosen device = %d\n", physical_device);

	free(devices);

    if (physical_device == VK_NULL_HANDLE) printf("failed to find a suitable GPU\n");

	// check for swapchain extension support

	uint32_t available_device_extension_count = 0;
	vkEnumerateDeviceExtensionProperties(physical_device, NULL, &available_device_extension_count, NULL);

	VkExtensionProperties *available_device_extensions = malloc(available_device_extension_count * sizeof(VkExtensionProperties));
	vkEnumerateDeviceExtensionProperties(physical_device, NULL, &available_device_extension_count, available_device_extensions);

	bool extensions_supported = check_extension_support(device_extensions, device_extension_count, available_device_extensions, available_device_extension_count);

	if(!extensions_supported) printf("physical device extensions requested, but not available\n");

	return physical_device;
}

struct QueueFamilyIndices create_queue_families(VkPhysicalDevice device, VkSurfaceKHR surface)
{
    struct QueueFamilyIndices indices;

	bool graphics_family_has_value = false;
	bool present_family_has_value = false;

    uint32_t queue_family_count = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(device, &queue_family_count, NULL);

    VkQueueFamilyProperties *queue_family_properties = malloc(queue_family_count * sizeof(VkQueueFamilyProperties));
    vkGetPhysicalDeviceQueueFamilyProperties(device, &queue_family_count, queue_family_properties);

    int i = 0;
    for (int i = 0; i < queue_family_count; i++) {
        if (queue_family_properties[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) {
            indices.graphicsFamily = i;
			graphics_family_has_value = true;
        }

        VkBool32 present_support = false;
        vkGetPhysicalDeviceSurfaceSupportKHR(device, i, surface, &present_support);

        if (present_support) {
            indices.presentFamily = i;
			present_family_has_value = true;
        }

        if (graphics_family_has_value && present_family_has_value) {
            break;
        }
    }

    if (!graphics_family_has_value || !present_family_has_value) {
		printf("could not find queue family with both graphics and present support\n");
    }

    return indices;
}

VkSurfaceFormatKHR create_format(VkPhysicalDevice physical_device, VkSurfaceKHR surface)
{
	uint32_t format_count = 0;
	vkGetPhysicalDeviceSurfaceFormatsKHR(physical_device, surface, &format_count, NULL);
	if(!format_count) printf("no surface format available for physcial device queue\n");

	VkSurfaceFormatKHR *formats = malloc(format_count * sizeof(VkSurfaceFormatKHR));
	vkGetPhysicalDeviceSurfaceFormatsKHR(physical_device, surface, &format_count, formats);

	VkSurfaceFormatKHR chosen_format = {
		.format = VK_FORMAT_B8G8R8A8_SRGB,
		.colorSpace = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR,
	};

	bool chosen = false;

	for(int i = 0; i < format_count; i++)
	{
		if(
			formats[i].format == VK_FORMAT_B8G8R8A8_SRGB &&
			formats[i].colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR
		)
		{
			chosen = true;
			break;
		}
	}

	free(formats);

	if(!chosen) printf("could not find suitable device surface format\n");

	return chosen_format;
}

VkPresentModeKHR create_present_mode(VkPhysicalDevice physical_device, VkSurfaceKHR surface)
{
	// uint32_t present_mode_count = 0;
	// vkGetPhysicalDeviceSurfacePresentModesKHR(physical_device, surface, &present_mode_count, NULL);
	// if(!present_mode_count) printf("physical device queue does not have present support\n");

	// VkPresentModeKHR *present_modes = malloc(present_mode_count * sizeof(VkPresentModeKHR));
	// vkGetPhysicalDeviceSurfacePresentModesKHR(physical_device, surface, &present_mode_count, present_modes);

	VkPresentModeKHR chosen_mode = VK_PRESENT_MODE_FIFO_KHR; // default mode

	// free(present_modes);

	return chosen_mode;
}

VkSurfaceCapabilitiesKHR create_capabilities(VkPhysicalDevice physical_device, VkSurfaceKHR surface)
{
	VkSurfaceCapabilitiesKHR capabilities;

	VkResult result = vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physical_device, surface, &capabilities);
	if(result != VK_SUCCESS) printf("failed to get physical device surface (swapchain) capabilities\n");

	return capabilities;
}

VkDevice create_device(bool validation_layers_enabled, const char **validation_layers, uint32_t validation_layer_count, VkPhysicalDevice physical_device, struct QueueFamilyIndices indices, uint32_t device_extension_count, const char **device_extensions)
{
    float queue_priority = 1.0f;

	VkDeviceQueueCreateInfo device_queue_info = {
		.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
		.pNext = NULL,
		.flags = 0,
		.queueFamilyIndex = indices.graphicsFamily,
		.queueCount = 1,
		.pQueuePriorities = &queue_priority,
	};

	VkPhysicalDeviceFeatures device_features = {0};

	VkDeviceCreateInfo device_info = {
		.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
		.pNext = NULL,
		.flags = 0,
		.queueCreateInfoCount = 1,
		.pQueueCreateInfos = &device_queue_info,
		.enabledLayerCount = 0,
		.ppEnabledLayerNames = NULL,
		.enabledExtensionCount = device_extension_count,
		.ppEnabledExtensionNames = device_extensions,
		.pEnabledFeatures = &device_features,
	};

    if (validation_layers_enabled)
	{
        device_info.enabledLayerCount = validation_layer_count;
        device_info.ppEnabledLayerNames = validation_layers;
    }

    VkDevice device;
	VkResult result = vkCreateDevice(physical_device, &device_info, NULL, &device);
    if (result != VK_SUCCESS) printf("failed to create logical device\n");

	return device;
}

VkQueue create_device_queue(VkDevice device, uint32_t queue_family_index, uint32_t queue_index)
{
	VkQueue queue;

    vkGetDeviceQueue(device, queue_family_index, queue_index, &queue);

	return queue;
}

VkExtent2D create_swap_extent(GLFWwindow *window, VkSurfaceCapabilitiesKHR capabilities)
{
    if (capabilities.currentExtent.width != UINT32_MAX) return capabilities.currentExtent;

    int width, height;
    glfwGetFramebufferSize(window, &width, &height);

    VkExtent2D actual_extent = {
		.width = (uint32_t)width,
		.height = (uint32_t)height,
	};

	uint32_t v = actual_extent.width;
	uint32_t lo = capabilities.minImageExtent.width;
	uint32_t hi = capabilities.maxImageExtent.width;
	actual_extent.width = (v < lo) ? lo : ((hi < v) ? hi : v );  // clamp

	v = actual_extent.height;
	lo = capabilities.minImageExtent.height;
	hi = capabilities.maxImageExtent.height;
	actual_extent.height = (v < lo) ? lo : ((hi < v) ? hi : v );

	return actual_extent;
}

uint32_t create_image_count(VkSurfaceCapabilitiesKHR capabilities)
{
    uint32_t image_count = capabilities.minImageCount + 1;

    if (capabilities.maxImageCount > 0 && image_count > capabilities.maxImageCount)
	{
        image_count = capabilities.maxImageCount;
    }

	return image_count;
}

VkSwapchainKHR create_swapchain(VkDevice device, VkSurfaceKHR surface, uint32_t imageCount, VkSurfaceFormatKHR surfaceFormat, VkExtent2D extent, struct QueueFamilyIndices indices, VkSurfaceCapabilitiesKHR capabilities, VkPresentModeKHR presentMode)
{
	VkSwapchainCreateInfoKHR swapchain_info = {
		.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,
		.pNext= NULL,
		.flags = 0,
		.surface = surface,
		.minImageCount = imageCount,
		.imageFormat = surfaceFormat.format,
		.imageColorSpace = surfaceFormat.colorSpace,
		.imageExtent = extent,
		.imageArrayLayers = 1,
		.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
		.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE,
		.queueFamilyIndexCount = 0,
		.pQueueFamilyIndices = NULL,
		.preTransform = capabilities.currentTransform,
		.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,
		.presentMode = presentMode,
		.clipped = VK_TRUE,
		.oldSwapchain = VK_NULL_HANDLE,
	};

    uint32_t queueFamilyIndices[] = {indices.graphicsFamily, indices.presentFamily};

    if (indices.graphicsFamily != indices.presentFamily) {
		swapchain_info.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
		swapchain_info.queueFamilyIndexCount = 2;
		swapchain_info.pQueueFamilyIndices = queueFamilyIndices;
	}

    VkSwapchainKHR swapchain;
    VkResult result = vkCreateSwapchainKHR(device, &swapchain_info, NULL, &swapchain);
    if(result != VK_SUCCESS) printf("failed to create swap chain!");

	return swapchain;
}

VkImageView *create_swapchain_image_views(VkDevice device, VkSwapchainKHR swapChain, VkFormat swapChainImageFormat, uint32_t imageCount)
{
    vkGetSwapchainImagesKHR(device, swapChain, &imageCount, NULL);

    VkImage *swapChainImages = malloc(imageCount * sizeof(VkImage));
    vkGetSwapchainImagesKHR(device, swapChain, &imageCount, swapChainImages);

    VkImageView *swapChainImageViews = malloc(imageCount * sizeof(VkImageView));

    for (size_t i = 0; i < imageCount; i++) {
        VkImageViewCreateInfo createInfo = {0};
        createInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        createInfo.image = swapChainImages[i];
        createInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
        createInfo.format = swapChainImageFormat;
        createInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
        createInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
        createInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
        createInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
        createInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        createInfo.subresourceRange.baseMipLevel = 0;
        createInfo.subresourceRange.levelCount = 1;
        createInfo.subresourceRange.baseArrayLayer = 0;
        createInfo.subresourceRange.layerCount = 1;

        VkResult result = vkCreateImageView(device, &createInfo, NULL, &swapChainImageViews[i]);
        if(result != VK_SUCCESS) printf("failed to create image views!");
    }

	return swapChainImageViews;
}

VkRenderPass create_render_pass(VkDevice device, VkFormat swapChainImageFormat)
{
	VkAttachmentDescription colorAttachment = {
		.flags = 0,
		.format = swapChainImageFormat,
		.samples = VK_SAMPLE_COUNT_1_BIT,
		.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
		.storeOp = VK_ATTACHMENT_STORE_OP_STORE,
		.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
		.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
		.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
		.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
	};

	VkAttachmentReference colorAttachmentRef = {
		.attachment = 0,
		.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
	};

	VkSubpassDescription subpass = {
		.flags = 0,
		.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS,
		.inputAttachmentCount = 0,
		.pInputAttachments = NULL,
		.colorAttachmentCount = 1,
		.pColorAttachments = &colorAttachmentRef,
		.pResolveAttachments = NULL,
		.pDepthStencilAttachment = NULL,
		.preserveAttachmentCount = 0,
		.pPreserveAttachments = NULL,
	};

	VkSubpassDependency dependency = {
		.srcSubpass = VK_SUBPASS_EXTERNAL,
		.dstSubpass = 0,
		.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
		.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
		.srcAccessMask = 0,
		.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
		.dependencyFlags = 0,
	};

	VkRenderPassCreateInfo renderPassInfo = {
		.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO,
		.pNext = NULL,
		.flags = 0,
		.attachmentCount = 1,
		.pAttachments = &colorAttachment,
		.subpassCount = 1,
		.pSubpasses = &subpass,
		.dependencyCount = 1,
		.pDependencies = &dependency,
	};

    VkRenderPass renderPass;
    VkResult result = vkCreateRenderPass(device, &renderPassInfo, NULL, &renderPass);
    if(result != VK_SUCCESS) printf("failed to create render pass!");

	return renderPass;
}

VkPipelineLayout create_pipeline_layout(VkDevice device)
{
	VkPipelineLayoutCreateInfo pipelineLayoutInfo = {
		.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
		.pNext = NULL,
		.flags = 0,
		.setLayoutCount = 0,
		.pSetLayouts = NULL,
		.pushConstantRangeCount = 0,
		.pPushConstantRanges = NULL,
	};

    VkPipelineLayout pipelineLayout;
    VkResult result = vkCreatePipelineLayout(device, &pipelineLayoutInfo, NULL, &pipelineLayout);
    if(result != VK_SUCCESS) printf("failed to create pipeline layout!");

	return pipelineLayout;
}

VkPipeline create_graphics_pipeline(VkDevice device, VkExtent2D swapChainExtent, VkRenderPass renderPass, VkPipelineLayout pipelineLayout)
{
	int vert_size, frag_size;

	char *vertShaderCode = readFile("../assets/shaders/vert.spv", &vert_size);
	char *fragShaderCode = readFile("../assets/shaders/frag.spv", &frag_size);

	VkShaderModule vertShaderModule = createShaderModule(vertShaderCode, vert_size, device);
	VkShaderModule fragShaderModule = createShaderModule(fragShaderCode, frag_size, device);

	VkPipelineShaderStageCreateInfo vertShaderStageInfo = {
		.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
		.pNext = NULL,
		.flags = 0,
		.stage = VK_SHADER_STAGE_VERTEX_BIT,
		.module = vertShaderModule,
		.pName = "main",
		.pSpecializationInfo = NULL,
	};

	VkPipelineShaderStageCreateInfo fragShaderStageInfo = {
		.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
		.pNext = NULL,
		.flags = 0,
		.stage = VK_SHADER_STAGE_FRAGMENT_BIT,
		.module = fragShaderModule,
		.pName = "main",
		.pSpecializationInfo = NULL,
	};

	VkPipelineShaderStageCreateInfo shaderStages[] = {vertShaderStageInfo, fragShaderStageInfo};

	VkPipelineVertexInputStateCreateInfo vertexInputInfo = {
		.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
		.pNext= NULL,
		.flags = 0,
		.vertexBindingDescriptionCount = 0,
		.pVertexBindingDescriptions = NULL,
		.vertexAttributeDescriptionCount = 0,
		.pVertexAttributeDescriptions = NULL,
	};

	VkPipelineInputAssemblyStateCreateInfo inputAssembly = {
		.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,
		.pNext = NULL,
		.flags = 0,
		.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,
		.primitiveRestartEnable = VK_FALSE,
	};

	VkViewport viewport = {
		.x = 0.0f,
		.y = 0.0f,
		.width = (float) swapChainExtent.width,
		.height = (float) swapChainExtent.height,
		.minDepth = 0.0f,
		.maxDepth = 1.0f,
	};

	VkRect2D scissor = {
		.offset = {
			.x = 0,
			.y = 0,
		},
		.extent = swapChainExtent,
	};

	VkPipelineViewportStateCreateInfo viewportState = {
		.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO,
		.pNext = NULL,
		.flags = 0,
		.viewportCount = 1,
		.pViewports = &viewport,
		.scissorCount = 1,
		.pScissors = &scissor,
	};

	VkPipelineRasterizationStateCreateInfo rasterizer = {
		.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO,
		.pNext = NULL,
		.flags = 0,
		.depthClampEnable = VK_FALSE,
		.rasterizerDiscardEnable = VK_FALSE,
		.polygonMode = VK_POLYGON_MODE_FILL,
		.cullMode = VK_CULL_MODE_BACK_BIT,
		.frontFace = VK_FRONT_FACE_CLOCKWISE,
		.depthBiasEnable = VK_FALSE,
		.depthBiasConstantFactor = 0,
		.depthBiasClamp = 0,
		.depthBiasSlopeFactor = 0,
		.lineWidth = 1.0f,
	};

	VkPipelineMultisampleStateCreateInfo multisampling = {
		.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO,
		.pNext = NULL,
		.flags = 0,
		.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT,
		.sampleShadingEnable = VK_FALSE,
		.minSampleShading = 0,
		.pSampleMask = NULL,
		.alphaToCoverageEnable = VK_FALSE,
		.alphaToOneEnable = VK_FALSE,
	};

	VkPipelineColorBlendAttachmentState colorBlendAttachment = {
		.blendEnable = VK_FALSE,
		.srcColorBlendFactor = VK_BLEND_FACTOR_ZERO,
		.dstColorBlendFactor = VK_BLEND_FACTOR_ZERO,
		.colorBlendOp = VK_BLEND_OP_ADD,
		.srcAlphaBlendFactor = VK_BLEND_FACTOR_ZERO,
		.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO,
		.alphaBlendOp = VK_BLEND_OP_ADD,
		.colorWriteMask = VK_COLOR_COMPONENT_R_BIT |
						  VK_COLOR_COMPONENT_G_BIT |
						  VK_COLOR_COMPONENT_B_BIT |
						  VK_COLOR_COMPONENT_A_BIT,

	};

	VkPipelineColorBlendStateCreateInfo colorBlending = {
		.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO,
		.pNext = NULL,
		.flags = 0,
		.logicOpEnable = VK_FALSE,
		.logicOp = VK_LOGIC_OP_COPY,
		.attachmentCount = 1,
		.pAttachments = &colorBlendAttachment,
		.blendConstants[0] = 0.0f,
		.blendConstants[1] = 0.0f,
		.blendConstants[2] = 0.0f,
		.blendConstants[3] = 0.0f,
	};

	VkGraphicsPipelineCreateInfo pipelineInfo = {
		.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
		.pNext = NULL,
		.flags = 0,
		.stageCount = 2,
		.pStages = shaderStages,
		.pVertexInputState = &vertexInputInfo,
		.pInputAssemblyState = &inputAssembly,
		.pTessellationState = NULL,
		.pViewportState = &viewportState,
		.pRasterizationState = &rasterizer,
		.pMultisampleState = &multisampling,
		.pDepthStencilState = NULL,
		.pColorBlendState = &colorBlending,
		.pDynamicState = NULL,
		.layout = pipelineLayout,
		.renderPass = renderPass,
		.subpass = 0,
		.basePipelineHandle = VK_NULL_HANDLE,
		.basePipelineIndex = 0,
	};

    VkPipeline graphicsPipeline;
    VkResult result = vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &pipelineInfo, NULL, &graphicsPipeline);
    if(result != VK_SUCCESS) printf("failed to create graphics pipeline!");

    vkDestroyShaderModule(device, fragShaderModule, NULL);
    vkDestroyShaderModule(device, vertShaderModule, NULL);

	return graphicsPipeline;
}

VkFramebuffer *create_swapchain_framebuffer(VkDevice device, VkImageView *swapChainImageViews, uint32_t image_count, VkRenderPass renderPass, VkExtent2D swapChainExtent)
{
    VkFramebuffer *swapchain_framebuffers = malloc(image_count * sizeof(VkFramebuffer));

    for (size_t i = 0; i < image_count; i++) {
        VkImageView attachments[] = {
            swapChainImageViews[i]
        };

		VkFramebufferCreateInfo framebuffer_info = {
			.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,
			.pNext = NULL,
			.flags = 0,
			.renderPass = renderPass,
			.attachmentCount = 1,
			.pAttachments = attachments,
			.width = swapChainExtent.width,
			.height = swapChainExtent.height,
			.layers = 1,
		};

        VkResult result = vkCreateFramebuffer(device, &framebuffer_info, NULL, &swapchain_framebuffers[i]);
        if(result != VK_SUCCESS) printf("failed to create framebuffer!");
    }

	return swapchain_framebuffers;
}

VkCommandPool create_command_pool(VkDevice device, struct QueueFamilyIndices indices)
{
	VkCommandPoolCreateInfo command_pool_info = {
		.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
		.pNext = NULL,
		.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT,
		.queueFamilyIndex = indices.graphicsFamily,
	};

	VkCommandPool command_pool;
	VkResult result = vkCreateCommandPool(device, &command_pool_info, NULL, &command_pool);
	if(result != VK_SUCCESS) printf("failed to create command pool!");

	return command_pool;
}

VkCommandBuffer create_command_buffer(VkDevice device, VkCommandPool command_pool)
{
	VkCommandBufferAllocateInfo command_buffer_info = {
		.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
		.pNext = NULL,
		.commandPool = command_pool,
		.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
		.commandBufferCount = 1,
	};

	VkCommandBuffer command_buffer;
	VkResult result = vkAllocateCommandBuffers(device, &command_buffer_info, &command_buffer);
	if(result != VK_SUCCESS) printf("failed to allocate command buffers!");

	return command_buffer;
}

VkSemaphore create_semaphore(VkDevice device)
{
	VkSemaphoreCreateInfo semaphore_info = {
		.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO,
		.pNext = NULL,
		.flags = 0,
	};

	VkSemaphore semaphore;
    VkResult result = vkCreateSemaphore(device, &semaphore_info, NULL, &semaphore);
    if(result != VK_SUCCESS) printf("failed to create synchronization semaphore for a frame!");

	return semaphore;
}

VkFence create_fence(VkDevice device)
{
	VkFence in_flight_fence;

	VkFenceCreateInfo fence_info = {
		.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
		.pNext = NULL,
		.flags = VK_FENCE_CREATE_SIGNALED_BIT,
	};

	VkResult result = vkCreateFence(device, &fence_info, NULL, &in_flight_fence);
	if(result != VK_SUCCESS) printf("failed to create synchronization fence for a frame!");

	return in_flight_fence;
}

char *readFile(const char *filename, int *file_size)
{
	FILE *fp = fopen(filename, "rb");
	if(!fp) printf("failed to open file\n");

	fseek(fp, 0, SEEK_END);
	*file_size = ftell(fp);
	rewind(fp);

	char *buffer = malloc( (*file_size) * sizeof(char));

	fread(buffer, *file_size, 1, fp);

	fclose(fp);

	return buffer;
}

VkShaderModule createShaderModule(char *code, int size, VkDevice device)
{
    VkShaderModuleCreateInfo shader_module_info = {
		.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
		.pNext = NULL,
		.flags = 0,
		.codeSize = size,
		.pCode = (const uint32_t*)code,
	};

	VkShaderModule shader_module;
	VkResult result = vkCreateShaderModule(device, &shader_module_info, NULL, &shader_module);
	if(result != VK_SUCCESS) printf("failed to create shader module!");

	return shader_module;
}

char **get_required_instance_extensions(bool validation_layers_enabled, uint32_t *instance_extension_count)
{
	char **instance_extensions = (char **) glfwGetRequiredInstanceExtensions(instance_extension_count);

	if(validation_layers_enabled)
	{
		*instance_extension_count += 1;
		instance_extensions = realloc(instance_extensions, (*instance_extension_count) * sizeof(*instance_extensions));
		instance_extensions[(*instance_extension_count)-1] = VK_EXT_DEBUG_UTILS_EXTENSION_NAME;
	}

	return instance_extensions;
}

void check_validation_layer_support(bool validation_layers_enabled, const char **required_layers, uint32_t required_layer_count)
{
	if(!validation_layers_enabled)
	{
		return;
	}

	uint32_t available_layer_count;
	vkEnumerateInstanceLayerProperties(&available_layer_count, NULL);

	VkLayerProperties *available_layers = malloc(available_layer_count * sizeof(VkLayerProperties));
	vkEnumerateInstanceLayerProperties(&available_layer_count, available_layers);

	bool layers_supported = check_layer_support(required_layers, required_layer_count, available_layers, available_layer_count);
	if(!layers_supported) printf("validation layers requested, but not available\n");

	free(available_layers);
}

void check_instance_extension_support(char **required_extensions, uint32_t required_extension_count)
{
	uint32_t available_extension_count = 0;
	vkEnumerateInstanceExtensionProperties(NULL, &available_extension_count, NULL);

	VkExtensionProperties *available_extensions = malloc(available_extension_count * sizeof(VkExtensionProperties));
	vkEnumerateInstanceExtensionProperties(NULL, &available_extension_count, available_extensions);

	bool extensions_supported = check_extension_support((const char **)required_extensions, required_extension_count, available_extensions, available_extension_count);
	if(!extensions_supported) printf("instance extensions requested, but not available\n");

	free(available_extensions);
}

bool check_extension_support(const char **required_extensions, uint32_t required_count, VkExtensionProperties *available_extensions, uint32_t available_count)
{
	for(int i = 0; i < required_count; i++)
	{
		bool extension_found = false;

		for(int j = 0; j < available_count; j++)
		{
			if(strcmp(required_extensions[i], available_extensions[j].extensionName) == 0)
			{
				extension_found = true;
				break;
			}
		}

		if(!extension_found)
		{
			return false;
		}
	}

	return true;
}

bool check_layer_support(const char **required_layers, uint32_t required_count, VkLayerProperties *available_layers, uint32_t available_count)
{
	for(int i = 0; i < required_count; i++)
	{
		bool layer_found = false;

		for(int j = 0; j < available_count; j++)
		{
			if(strcmp(required_layers[i], available_layers[j].layerName) == 0)
			{
				layer_found = true;
				break;
			}
		}

		if(!layer_found)
		{
			return false;
		}
	}

	return true;
}

VKAPI_ATTR VkBool32 VKAPI_CALL debug_callback(
	VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
	VkDebugUtilsMessageTypeFlagsEXT messageType,
	const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
	void* pUserData
)
{
	printf("validation layer: %s\n", pCallbackData->pMessage);

    return VK_FALSE;
}

void print_physical_device_info(VkInstance instance, VkSurfaceKHR surface, uint32_t device_extension_count, const char **device_extensions)
{
    uint32_t device_count = 0;
    vkEnumeratePhysicalDevices(instance, &device_count, NULL);
    if (device_count == 0) printf("failed to find GPUs with Vulkan support\n");

    VkPhysicalDevice *devices = malloc(device_count * sizeof(VkPhysicalDevice));
    vkEnumeratePhysicalDevices(instance, &device_count, devices);

	printf("\ndevice_count = %d\n", device_count);
    for (int i = 0; i < device_count; i++)
	{
		VkPhysicalDevice device = devices[i];

		// per device info

		VkPhysicalDeviceProperties device_properties;
		vkGetPhysicalDeviceProperties(devices[i], &device_properties);

		// VkPhysicalDeviceFeatures device_features;
		// vkGetPhysicalDeviceFeatures(devices[i], &device_features);

		char *type = get_device_type_string(device_properties.deviceType);

		printf("\ndevice[%d] = %s, %s, ", i, type, device_properties.deviceName);

		uint32_t version = device_properties.apiVersion;

		uint32_t major = VK_VERSION_MAJOR(version);
		uint32_t minor = VK_VERSION_MINOR(version);
		uint32_t patch = VK_VERSION_PATCH(version);

		printf("api = %d.%d.%d, ", major, minor, patch);

		// check for swapchain extension support

		uint32_t available_device_extension_count = 0;
		vkEnumerateDeviceExtensionProperties(device, NULL, &available_device_extension_count, NULL);

		VkExtensionProperties *available_device_extensions = malloc(available_device_extension_count * sizeof(VkExtensionProperties));
		vkEnumerateDeviceExtensionProperties(device, NULL, &available_device_extension_count, available_device_extensions);

		bool extensions_supported = check_extension_support(device_extensions, device_extension_count, available_device_extensions, available_device_extension_count);

		printf("swapchain support = %s\n", extensions_supported ? "true" : "false" );

		// get memory heap properties

		VkPhysicalDeviceMemoryProperties memory_properties;
		vkGetPhysicalDeviceMemoryProperties(devices[i], &memory_properties);

		uint32_t heap_count = memory_properties.memoryHeapCount;

		printf("\n\tmemory heap count = %d\n", heap_count);
		for(int j = 0; j < heap_count; j++)
		{
			VkMemoryHeap heap = memory_properties.memoryHeaps[j];

			VkDeviceSize size = heap.size; // uint64_t
			VkMemoryHeapFlags flags = heap.flags;

			printf("\t%d, %ld, ", j, size);

			if(flags & VK_MEMORY_HEAP_DEVICE_LOCAL_BIT) printf("device local memory");
			if(flags & VK_MEMORY_HEAP_MULTI_INSTANCE_BIT) printf("multi instance memory");
			printf("\n");
		}

		// get memory type properties

		uint32_t type_count = memory_properties.memoryTypeCount;

		printf("\n\tmemory type count = %d\n", type_count);
		for(int j = 0; j < type_count; j++)
		{
			VkMemoryType type = memory_properties.memoryTypes[j];

			VkMemoryPropertyFlags flags = type.propertyFlags;
			uint32_t index = type.heapIndex;

			printf("\t%d  %d  ", j, index);
			print_memory_property_flags(flags);
		}

		// queue family info

		uint32_t queue_family_count = 0;
		vkGetPhysicalDeviceQueueFamilyProperties(devices[i], &queue_family_count, NULL);

		VkQueueFamilyProperties *queue_families = malloc(queue_family_count * sizeof(VkQueueFamilyProperties));
		vkGetPhysicalDeviceQueueFamilyProperties(devices[i], &queue_family_count, queue_families);

		printf("\n\tqueue family count = %d\n", queue_family_count);
		for(int j = 0; j < queue_family_count; j++)
		{
			print_queue_family_info(devices[i], surface, &queue_families[j], j);
		}

		// color formats

		uint32_t format_count = 0;
		vkGetPhysicalDeviceSurfaceFormatsKHR(devices[i], surface, &format_count, NULL);

		VkSurfaceFormatKHR *formats = malloc(format_count * sizeof(VkSurfaceFormatKHR));
		vkGetPhysicalDeviceSurfaceFormatsKHR(devices[i], surface, &format_count, formats);

		printf("\n\tformat count = %d\n", format_count);
		for(int j = 0; j < format_count; j++)
		{
			char *color_space_string = get_color_space_string(formats[j].colorSpace);
			char *color_format_string = get_color_format_string(formats[j].format);
			printf("\tformat[%d]: format = %s, color space = %s\n", j, color_format_string, color_space_string);
		}

		// present mode

		uint32_t present_mode_count = 0;
		vkGetPhysicalDeviceSurfacePresentModesKHR(devices[i], surface, &present_mode_count, NULL);

		VkPresentModeKHR *present_modes = malloc(present_mode_count * sizeof(VkPresentModeKHR));
		vkGetPhysicalDeviceSurfacePresentModesKHR(devices[i], surface, &present_mode_count, present_modes);

		printf("\n\tpresent mode count = %d\n", present_mode_count);
		for(int j = 0; j < present_mode_count; j++)
		{
			char *present_string = get_present_mode_string(present_modes[j]);
			printf("\tmode[%d] = %s\n", j, present_string);
		}

		// device capabilities

		VkSurfaceCapabilitiesKHR capabilities;
		vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, surface, &capabilities);

		printf("\n\tcapabilities\n");
		printf("\tmin image count = %d\n", capabilities.minImageCount);
		printf("\tmax image count = %d\n", capabilities.maxImageCount);
		printf("\tcurrent extent: width = %d, height = %d\n", capabilities.currentExtent.width, capabilities.currentExtent.height);
		printf("\tmin image extent: width = %d, height = %d\n", capabilities.minImageExtent.width, capabilities.minImageExtent.height);
		printf("\tmax image extent: width = %d, height = %d\n", capabilities.maxImageExtent.width, capabilities.maxImageExtent.height);
		printf("\tmax image array layers = %d\n", capabilities.maxImageArrayLayers);

		printf("\tsupported transforms = ");
		if(capabilities.supportedTransforms & VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR) printf("identity, ");
		printf("\n");

		char *surface_transform_string = get_surface_transform_string(capabilities.currentTransform);
		printf("\tcurrent transform = %s\n", surface_transform_string);

		printf("\tsupported composite alpha = ");
		if(capabilities.supportedCompositeAlpha & VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR) printf("opaque, ");
		if(capabilities.supportedCompositeAlpha & VK_COMPOSITE_ALPHA_PRE_MULTIPLIED_BIT_KHR) printf("pre multiplied, ");
		if(capabilities.supportedCompositeAlpha & VK_COMPOSITE_ALPHA_POST_MULTIPLIED_BIT_KHR) printf("post multiplied, ");
		if(capabilities.supportedCompositeAlpha & VK_COMPOSITE_ALPHA_INHERIT_BIT_KHR) printf("inherit, ");
		printf("\n");

		printf("\tsupported image usage = ");
		if(capabilities.supportedUsageFlags & VK_IMAGE_USAGE_TRANSFER_SRC_BIT) printf("transfer src, ");
		if(capabilities.supportedUsageFlags & VK_IMAGE_USAGE_TRANSFER_DST_BIT) printf("transfer dst, ");
		if(capabilities.supportedUsageFlags & VK_IMAGE_USAGE_SAMPLED_BIT) printf("sampled, ");
		if(capabilities.supportedUsageFlags & VK_IMAGE_USAGE_STORAGE_BIT) printf("storage, ");
		if(capabilities.supportedUsageFlags & VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT) printf("color attachment, ");
		if(capabilities.supportedUsageFlags & VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT) printf("depth stencil attachment, ");
		if(capabilities.supportedUsageFlags & VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT) printf("transient attachment, ");
		if(capabilities.supportedUsageFlags & VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT) printf("input attachment, ");
		printf("\n");

    }
}

void print_queue_family_info(VkPhysicalDevice device, VkSurfaceKHR surface, VkQueueFamilyProperties *queue_family, uint32_t queue_family_index)
{
	VkQueueFlags queue_flags = queue_family->queueFlags;

	printf("\tqueue_family[%d] (%d max queues) { ", queue_family_index, queue_family->queueCount);
	if(queue_flags & VK_QUEUE_GRAPHICS_BIT) printf("graphics, ");
	if(queue_flags & VK_QUEUE_COMPUTE_BIT) printf("compute, ");
	if(queue_flags & VK_QUEUE_TRANSFER_BIT) printf("transfer, ");
	if(queue_flags & VK_QUEUE_SPARSE_BINDING_BIT) printf("sparse binding, ");
	printf("}");

	VkBool32 present_support = false;
	vkGetPhysicalDeviceSurfaceSupportKHR(device, queue_family_index, surface, &present_support);

	printf("\tpresent support = %s\n", present_support ? "true" : "false" );
}

char *get_device_type_string(enum VkPhysicalDeviceType device_type)
{
	char *device_string;

	switch(device_type)
	{
		case VK_PHYSICAL_DEVICE_TYPE_OTHER:
			device_string = (char *)"other";
			break;
		case VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU:
			device_string = (char *)"integrated gpu";
			break;
		case VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU:
			device_string = (char *)"discrete gpu";
			break;
		case VK_PHYSICAL_DEVICE_TYPE_VIRTUAL_GPU:
			device_string = (char *)"virtual gpu";
			break;
		case VK_PHYSICAL_DEVICE_TYPE_CPU:
			device_string = (char *)"cpu";
			break;
		default:
			device_string = (char *)"unknown";
			break;
	}

	return device_string;
}

char *get_present_mode_string(enum VkPresentModeKHR present_mode)
{
	char *present_string;

	switch(present_mode)
	{
		case VK_PRESENT_MODE_IMMEDIATE_KHR:
			present_string = (char *)"immediate khr";
			break;
		case VK_PRESENT_MODE_MAILBOX_KHR:
			present_string = (char *)"mailbox";
			break;
		case VK_PRESENT_MODE_FIFO_KHR:
			present_string = (char *)"fifo";
			break;
		case VK_PRESENT_MODE_FIFO_RELAXED_KHR:
			present_string = (char *)"fifo relaxed";
			break;
		default:
			present_string = (char *)"unknown";
			break;
	}

	return present_string;
}

char *get_color_format_string(VkFormat color_format)
{
	char *color_format_string;

	switch(color_format)
	{
		case VK_FORMAT_B8G8R8A8_UNORM:
			color_format_string = (char *)"B8G8R8A8 UNORM";
			break;
		case VK_FORMAT_B8G8R8A8_SRGB:
			color_format_string = (char *)"B8G8R8A8 SRGB";
			break;
		default:
			color_format_string = (char *)"too lazy to put all the formats so not found";
			break;
	}

	return color_format_string;
}

char *get_color_space_string(VkColorSpaceKHR color_space)
{
	char *color_space_string;

	switch(color_space)
	{
		case VK_COLOR_SPACE_SRGB_NONLINEAR_KHR:
			color_space_string = (char *)"SRGB nonlinear KHR";
			break;
		default:
			color_space_string = (char *)"unknown";
			break;
	}

	return color_space_string;
}

char *get_surface_transform_string(VkSurfaceTransformFlagBitsKHR surface_transform_flags)
{
	char *surface_transform_string;

	switch(surface_transform_flags)
	{
		case VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR:
			surface_transform_string = (char *)"identity";
			break;
		case VK_SURFACE_TRANSFORM_ROTATE_90_BIT_KHR:
			surface_transform_string = (char *)"rotate 90";
			break;
		case VK_SURFACE_TRANSFORM_ROTATE_180_BIT_KHR:
			surface_transform_string = (char *)"rotate 180";
			break;
		case VK_SURFACE_TRANSFORM_ROTATE_270_BIT_KHR:
			surface_transform_string = (char *)"rotate 270";
			break;
		case VK_SURFACE_TRANSFORM_HORIZONTAL_MIRROR_BIT_KHR:
			surface_transform_string = (char *)"horizontal mirror";
			break;
		case VK_SURFACE_TRANSFORM_HORIZONTAL_MIRROR_ROTATE_90_BIT_KHR:
			surface_transform_string = (char *)"horizontal mirror rotate 90";
			break;
		case VK_SURFACE_TRANSFORM_HORIZONTAL_MIRROR_ROTATE_180_BIT_KHR:
			surface_transform_string = (char *)"horizontal mirror rotate 180";
			break;
		case VK_SURFACE_TRANSFORM_HORIZONTAL_MIRROR_ROTATE_270_BIT_KHR:
			surface_transform_string = (char *)"horizontal mirror rotate 270";
			break;
		case VK_SURFACE_TRANSFORM_INHERIT_BIT_KHR:
			surface_transform_string = (char *)"inherit";
			break;
		default:
			surface_transform_string = (char *)"unknown flag";
			break;
	}

	return surface_transform_string;
}

void print_memory_property_flags(VkMemoryPropertyFlags flags)
{
	printf("\tproperty flags: ");

	if(flags & VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT) printf("device local, ");
	if(flags & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT) printf("host visible, ");
	if(flags & VK_MEMORY_PROPERTY_HOST_COHERENT_BIT) printf("host coherent, ");
	if(flags & VK_MEMORY_PROPERTY_HOST_CACHED_BIT) printf("host cached, ");
	if(flags & VK_MEMORY_PROPERTY_LAZILY_ALLOCATED_BIT) printf("lazily allocated, ");

	printf("\n");
}
