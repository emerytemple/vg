
// gcc trianglec.c -lglfw -lvulkan -std=c11 -pedantic

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <vulkan/vulkan.h>

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <stdbool.h>

#include "render.h"

int main() {

	bool validation_layers_enabled = true;

	uint32_t validation_layer_count = 1;
	const char *validation_layers[] = {
		"VK_LAYER_KHRONOS_validation",
	};

	uint32_t device_extension_count = 1;
	const char *device_extensions[] = {
		VK_KHR_SWAPCHAIN_EXTENSION_NAME
	};

	glfwInit();

	uint32_t instance_extension_count = 0;
	char **instance_extensions = get_required_instance_extensions(validation_layers_enabled, &instance_extension_count);

	check_validation_layer_support(validation_layers_enabled, validation_layers, validation_layer_count);
	check_instance_extension_support(instance_extensions, instance_extension_count);

	GLFWwindow *window                      = create_window();

	VkInstance instance                     = create_instance(validation_layers_enabled, validation_layer_count, validation_layers, instance_extension_count, instance_extensions);
	VkSurfaceKHR surface                    = create_surface(window, instance);
	VkDebugUtilsMessengerEXT debugMessenger = create_debug_messenger(validation_layers_enabled, instance);

	// print_physical_device_info(instance, surface, device_extension_count, device_extensions);

	VkPhysicalDevice physicalDevice         = create_physical_device(instance, surface, device_extension_count, device_extensions);
    struct QueueFamilyIndices indices 		= create_queue_families(physicalDevice, surface);
	VkSurfaceFormatKHR surfaceFormat 		= create_format(physicalDevice, surface);
	VkPresentModeKHR presentMode 			= create_present_mode(physicalDevice, surface); // move inside swpachain creation?
	VkSurfaceCapabilitiesKHR capabilities 	= create_capabilities(physicalDevice, surface);

	VkDevice device 						= create_device(validation_layers_enabled, validation_layers, validation_layer_count, physicalDevice, indices, device_extension_count, device_extensions);
	VkQueue graphicsQueue 					= create_device_queue(device, indices.graphicsFamily, 0);
	VkQueue presentQueue 					= create_device_queue(device, indices.presentFamily, 0);

	VkExtent2D extent 						= create_swap_extent(window, capabilities);
	uint32_t imageCount 					= create_image_count(capabilities);
	VkSwapchainKHR swapChain 				= create_swapchain(device, surface, imageCount, surfaceFormat, extent, indices, capabilities, presentMode);
	VkImageView *swapChainImageViews		= create_swapchain_image_views(device, swapChain, surfaceFormat.format, imageCount);
	VkRenderPass renderPass 				= create_render_pass(device, surfaceFormat.format);
	VkPipelineLayout pipelineLayout			= create_pipeline_layout(device);
    VkPipeline graphicsPipeline				= create_graphics_pipeline(device, extent, renderPass, pipelineLayout);
	VkFramebuffer *swapChainFramebuffers 	= create_swapchain_framebuffer(device, swapChainImageViews, imageCount, renderPass, extent);
	VkCommandPool commandPool 				= create_command_pool(device, indices);
	VkCommandBuffer commandBuffer 			= create_command_buffer(device, commandPool);
	VkSemaphore imageAvailableSemaphores 	= create_semaphore(device);
	VkSemaphore renderFinishedSemaphores 	= create_semaphore(device);
	VkFence inFlightFence 					= create_fence(device);

	// main loop

    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();

		// draw frame

	    vkWaitForFences(device, 1, &inFlightFence, VK_TRUE, UINT64_MAX);
	    vkResetFences(device, 1, &inFlightFence);

	    uint32_t imageIndex;
	    vkAcquireNextImageKHR(device, swapChain, UINT64_MAX, imageAvailableSemaphores, VK_NULL_HANDLE, &imageIndex);

	    vkResetCommandBuffer(commandBuffer, /*VkCommandBufferResetFlagBits*/ 0);

		// begin record command buffer

	    VkCommandBufferBeginInfo beginInfo = {
			.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
			.pNext = NULL,
			.flags = 0,
			.pInheritanceInfo = NULL,
		};

	    VkResult result = vkBeginCommandBuffer(commandBuffer, &beginInfo);
	    if(result != VK_SUCCESS) printf("failed to begin recording command buffer\n");

		VkOffset2D offset = {
			.x = 0,
			.y = 0,
		};

		VkRect2D renderArea = {
			.offset = offset,
			.extent = extent,
		};

		VkRenderPassBeginInfo renderPassInfo = {
			.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
			.pNext = NULL,
			.renderPass = renderPass,
			.framebuffer = swapChainFramebuffers[imageIndex],
			.renderArea = renderArea,
			.clearValueCount = 1,
			.pClearValues = &(VkClearValue) {{{0.0f, 0.0f, 0.0f, 1.0f}}},
		};

		vkCmdBeginRenderPass(commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

		vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, graphicsPipeline);

		vkCmdDraw(commandBuffer, 3, 1, 0, 0);

	    vkCmdEndRenderPass(commandBuffer);

	    result = vkEndCommandBuffer(commandBuffer);
		if(result != VK_SUCCESS) printf("failed to record command buffer\n");

		// end record command buffer

		VkSemaphore signalSemaphores[] = {renderFinishedSemaphores};

		VkSubmitInfo submitInfo = {
			.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
			.pNext = NULL,
			.waitSemaphoreCount = 1,
			.pWaitSemaphores = &(VkSemaphore) {imageAvailableSemaphores},
			.pWaitDstStageMask = &(VkPipelineStageFlags) {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT},
			.commandBufferCount = 1,
			.pCommandBuffers = &commandBuffer,
			.signalSemaphoreCount = 1,
			.pSignalSemaphores = signalSemaphores,
		};

	    result = vkQueueSubmit(graphicsQueue, 1, &submitInfo, inFlightFence);
	    if(result != VK_SUCCESS) printf("failed to submit draw command buffer!");

		VkPresentInfoKHR presentInfo = {
			.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
			.pNext = NULL,
			.waitSemaphoreCount = 1,
			.pWaitSemaphores = signalSemaphores,
			.swapchainCount = 1,
			.pSwapchains = &(VkSwapchainKHR) {swapChain},
			.pImageIndices = &imageIndex,
			.pResults = NULL,
		};

	    vkQueuePresentKHR(presentQueue, &presentInfo);
    }

    vkDeviceWaitIdle(device);

	// cleanup

    vkDestroySemaphore(device, renderFinishedSemaphores, NULL);
    vkDestroySemaphore(device, imageAvailableSemaphores, NULL);
    vkDestroyFence(device, inFlightFence, NULL);

    vkDestroyCommandPool(device, commandPool, NULL);

	for(int i = 0; i < imageCount; i++)
	{
        vkDestroyFramebuffer(device, swapChainFramebuffers[i], NULL);
    }

    vkDestroyPipeline(device, graphicsPipeline, NULL);
    vkDestroyPipelineLayout(device, pipelineLayout, NULL);
    vkDestroyRenderPass(device, renderPass, NULL);

	for(int i = 0; i < imageCount; i++)
	{
        vkDestroyImageView(device, swapChainImageViews[i], NULL);
    }

    vkDestroySwapchainKHR(device, swapChain, NULL);
    vkDestroyDevice(device, NULL);

    if (validation_layers_enabled)
	{
		PFN_vkDestroyDebugUtilsMessengerEXT func = (PFN_vkDestroyDebugUtilsMessengerEXT) vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT");
		if(func == VK_NULL_HANDLE) printf("failed to load vkDestroyDebugUtilsMessengerEXT function\n");

		func(instance, debugMessenger, NULL);
    }

    vkDestroySurfaceKHR(instance, surface, NULL);
    vkDestroyInstance(instance, NULL);

    glfwDestroyWindow(window);

    glfwTerminate();

    return EXIT_SUCCESS;
}


























































