#pragma once
#define NOMINMAX
#define VK_USE_PLATFORM_WIN32_KHR
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <vector>
#include <set>
#include <stdexcept>
#include <iostream>
#include <algorithm>
#include <limits>
#include <array>
#include "Utilities.h"

class VulkanRenderer
{
private:
	GLFWwindow* window = nullptr;

	// 实例是使用 Vulkan 的入口：通过它枚举 GPU、创建窗口表面。
	VkInstance instance = VK_NULL_HANDLE;

	struct {
		// 物理设备是选中的 GPU 的句柄；我们选择它，而不是创建一块显卡。
		VkPhysicalDevice physicalDevice = VK_NULL_HANDLE;
		// 逻辑设备是使用这块 GPU 的接口，用于创建资源和获取队列。
		VkDevice logicalDevice = VK_NULL_HANDLE;
	} mainDevices;
	// 创建逻辑设备时申请的队列会同时被创建。这里保存队列句柄，方便以后提交工作。
	// 句柄是访问已有对象的标识，不是复制出一份队列；队列随逻辑设备一起销毁。
	VkQueue graphicsQueue = VK_NULL_HANDLE;      // 提交绘图工作。
	VkQueue presentationQueue = VK_NULL_HANDLE; // 提交向窗口呈现图像的请求。
	VkSurfaceKHR surface = VK_NULL_HANDLE;      // Vulkan 与这个窗口的呈现连接。
	VkSwapchainKHR swapchain = VK_NULL_HANDLE;   // 管理一组轮流用于呈现的图像。
	std::vector<SwapchainImage> SwapChainImages;

	// - Pipeline
	// Pipeline Layout 规定着色器能访问哪些描述符和推送常量；当前两者都没有，所以它是“空布局”。
	VkPipelineLayout pipelineLayout = VK_NULL_HANDLE;
	// Render Pass 描述一次渲染如何使用颜色附件，以及附件开始、结束时应处于什么状态。
	VkRenderPass renderPass = VK_NULL_HANDLE;
	// 图形管线把着色器和顶点输入、光栅化、混色等固定状态组合成一个可绑定对象。
	VkPipeline graphicsPipeline = VK_NULL_HANDLE;

	// - Utility
	VkFormat swapChainImageFormat = VK_FORMAT_UNDEFINED;
	VkExtent2D swapChainExtent{};

	// Vulkan 函数
	// - Create
	void createInstance();
	void createLogicalDevice();
	void createSurface();
	void createSwapChain();
	// 描述交换链图像在一次渲染中的用途、布局变化和子通道关系。
	void createRenderPass();
	// 读取编译后的着色器，并逐步组装固定的图形管线状态。
	void createGraphicsPipeline();
	// - Get
	void getPhysicalDevice();

	// - Support
	// -- Checker
	bool checkInstanceExtensionSupport(std::vector<const char*>* checkExtensions);
	bool checkDeviceExtensionSupport(VkPhysicalDevice device);
	bool checkDeviceSuitable(VkPhysicalDevice device);
	// -- Getter
	QueueFamilyIndices getQueueFamilies(VkPhysicalDevice device);
	SwapChainDetails getSwapChainDetails(VkPhysicalDevice device);
	// -- Choose
	VkSurfaceFormatKHR chooseBestSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& formats);
	VkPresentModeKHR chooseBestPresentationMode(const std::vector<VkPresentModeKHR>& presentationModes);
	VkExtent2D chooseSwapExtent(const VkSurfaceCapabilitiesKHR& surfaceCapabilities);

	// -- Create
	VkImageView createImageView(VkImage image, VkFormat format, VkImageAspectFlags aspectFlags);
	// 把内存中的 SPIR-V 字节包装成逻辑设备可使用的 Shader Module。
	VkShaderModule createShaderModule(const std::vector<char>& code);

public:
	VulkanRenderer();
	~VulkanRenderer();

	int init(GLFWwindow* newWindow);
	void cleanup();


};

