
#pragma once

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <vulkan/vulkan.h>

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <stdbool.h>

struct QueueFamilyIndices {
	uint32_t graphicsFamily;
	uint32_t presentFamily;
};

GLFWwindow *create_window();
VkInstance create_instance(bool validation_layers_enabled, uint32_t validation_layer_count, const char **validation_layers, uint32_t instance_extension_count, char **instance_extensions);
VkDebugUtilsMessengerEXT create_debug_messenger(bool validation_layers_enabled, VkInstance instance);
VkSurfaceKHR create_surface(GLFWwindow *window, VkInstance instance);
VkPhysicalDevice create_physical_device(VkInstance instance, VkSurfaceKHR surface, uint32_t device_extension_count, const char **device_extensions);
VkDevice create_device(bool validation_layers_enabled, const char **validation_layers, uint32_t validation_layer_count, VkPhysicalDevice physicalDevice, struct QueueFamilyIndices indices, uint32_t device_extension_count, const char **device_extensions);
VkQueue create_device_queue(VkDevice device, uint32_t queue_family_index, uint32_t queue_index);
VkSurfaceFormatKHR create_format(VkPhysicalDevice physical_device, VkSurfaceKHR surface);
VkPresentModeKHR create_present_mode(VkPhysicalDevice physical_device, VkSurfaceKHR surface);
VkSurfaceCapabilitiesKHR create_capabilities(VkPhysicalDevice physical_device, VkSurfaceKHR surface);
VkExtent2D create_swap_extent(GLFWwindow *window, VkSurfaceCapabilitiesKHR capabilities);
uint32_t create_image_count(VkSurfaceCapabilitiesKHR capabilities);
VkSwapchainKHR create_swapchain(VkDevice device, VkSurfaceKHR surface, uint32_t imageCount, VkSurfaceFormatKHR surfaceFormat, VkExtent2D extent, struct QueueFamilyIndices indices, VkSurfaceCapabilitiesKHR capabilities, VkPresentModeKHR presentMode);
VkImageView *create_swapchain_image_views(VkDevice device, VkSwapchainKHR swapChain, VkFormat swapChainImageFormat, uint32_t imageCount);
VkRenderPass create_render_pass(VkDevice device, VkFormat swapChainImageFormat);
VkPipelineLayout create_pipeline_layout(VkDevice device);
VkPipeline create_graphics_pipeline(VkDevice device, VkExtent2D swapChainExtent, VkRenderPass renderPass, VkPipelineLayout pipelineLayout);
VkFramebuffer *create_swapchain_framebuffer(VkDevice device, VkImageView *swapChainImageViews, uint32_t image_count, VkRenderPass renderPass, VkExtent2D swapChainExtent);
VkCommandPool create_command_pool(VkDevice device, struct QueueFamilyIndices indices);
VkCommandBuffer create_command_buffer(VkDevice device, VkCommandPool commandPool);
VkSemaphore create_semaphore(VkDevice device);
VkFence create_fence(VkDevice device);
char *readFile(const char *filename, int *file_size);
VkShaderModule createShaderModule(char *code, int size, VkDevice device);
struct QueueFamilyIndices create_queue_families(VkPhysicalDevice device, VkSurfaceKHR surface);

char **get_required_instance_extensions(bool validation_layers_enabled, uint32_t *instance_extension_count);
void check_validation_layer_support(bool validation_layers_enabled, const char **validation_layers, uint32_t validation_layer_count);
void check_instance_extension_support(char **required_extensions, uint32_t required_extension_count);

bool check_extension_support(const char **required_extensions, uint32_t required_count, VkExtensionProperties *available_extensions, uint32_t available_count);
bool check_layer_support(const char **required_layers, uint32_t required_count, VkLayerProperties *available_layers, uint32_t available_count);

void print_physical_device_info(VkInstance instance, VkSurfaceKHR surface, uint32_t device_extension_count, const char **device_extensions);
void print_queue_family_info(VkPhysicalDevice device, VkSurfaceKHR surface, VkQueueFamilyProperties *queue_family, uint32_t queue_family_index);
char *get_device_type_string(enum VkPhysicalDeviceType device_type);
char *get_present_mode_string(enum VkPresentModeKHR present_mode);
char *get_color_format_string(VkFormat color_format);
char *get_color_space_string(VkColorSpaceKHR color_space);
char *get_surface_transform_string(VkSurfaceTransformFlagBitsKHR surface_transform_flags);
void print_memory_property_flags(VkMemoryPropertyFlags flags);

VKAPI_ATTR VkBool32 VKAPI_CALL debug_callback(
	VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
	VkDebugUtilsMessageTypeFlagsEXT messageType,
	const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
	void* pUserData
);

#endif
