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
#include "Mesh.h"

class VulkanRenderer
{
private:
	GLFWwindow* window = nullptr; // GLFW 窗口；创建 Surface 和查询实际像素尺寸时使用。

	int currentFrame = 0; // 当前使用第几组帧同步对象；只在 0 到 MAX_FRAME_DRAWS - 1 之间循环。

	// 场景对象
	std::vector<Mesh> meshList; // 保存场景中所有 Mesh 对象；每个 Mesh 都有自己的顶点缓冲区和索引缓冲区。

	// Vulkan 对象
	VkInstance instance = VK_NULL_HANDLE; // Vulkan 入口；通过它枚举 GPU、创建窗口 Surface。
	struct {
		VkPhysicalDevice physicalDevice = VK_NULL_HANDLE; // 选中的 GPU 句柄；它由 Vulkan 枚举出来。
		VkDevice logicalDevice = VK_NULL_HANDLE;           // 操作该 GPU 的接口；用于创建资源和取得队列。
	} mainDevices;
	VkQueue graphicsQueue = VK_NULL_HANDLE;                 // 接收并执行绘图命令。
	VkQueue presentationQueue = VK_NULL_HANDLE;             // 把交换链图像提交给窗口系统显示。
	VkSurfaceKHR surface = VK_NULL_HANDLE;                   // Vulkan 与 GLFW 窗口之间的呈现连接。
	VkSwapchainKHR swapchain = VK_NULL_HANDLE;               // 管理一组轮流渲染、呈现的图像。
	std::vector<SwapchainImage> SwapChainImages;             // 保存每张交换链 Image 及其 ImageView。
	std::vector<VkFramebuffer> swapchainFramebuffers;       // 每项把同下标 ImageView 绑定到 RenderPass 附件。
	std::vector<VkCommandBuffer> commandBuffers;            // 每张交换链图像对应一份已经录制的绘图命令。

	// - Pipeline
	VkPipelineLayout pipelineLayout = VK_NULL_HANDLE; // 着色器资源接口；当前没有描述符和 Push Constant。
	VkRenderPass renderPass = VK_NULL_HANDLE;         // 规定附件、Subpass 及图像布局变化。
	VkPipeline graphicsPipeline = VK_NULL_HANDLE;     // 汇总着色器、光栅化、混色等图形处理规则。

	// - Pools
	VkCommandPool graphicsCommandPool = VK_NULL_HANDLE; // 为图形队列族管理 Command Buffer 的分配内存。

	// - Utility
	VkFormat swapChainImageFormat = VK_FORMAT_UNDEFINED; // 交换链图像采用的像素格式。
	VkExtent2D swapChainExtent{};                        // 交换链图像的实际宽高。

	// - Synchronisation
	// - 同步
	std::vector<VkSemaphore> imageAvailableSemaphores; // 每个 currentFrame 一个：交换链图像已经可以开始绘制。
	std::vector<VkSemaphore> renderFinishedSemaphores; // 每张交换链图像一个：该图像已经绘制完成，可以呈现。
	std::vector<VkFence> drawFences;                   // 每个 currentFrame 一个：该组同步对象对应的提交已经执行完毕。

	// Vulkan 函数
	// - Create
	void createInstance();         // 创建 Vulkan 入口。
	void createLogicalDevice();    // 创建操作 GPU 的设备及所需队列。
	void createSurface();          // 建立 Vulkan 与窗口的呈现连接。
	void createSwapChain();        // 创建交换链、取得图像并创建 ImageView。
	void createRenderPass();       // 描述附件、Subpass 和布局转换规则。
	void createGraphicsPipeline(); // 创建着色器模块并组装固定图形管线。
	void createFramebuffers();     // 把每张交换链 ImageView 绑定到 RenderPass 附件。
	void createCommandPool();      // 为图形队列族创建 Command Buffer 内存池。
	void createCommandBuffers();   // 从 Command Pool 为每张交换链图像分配命令缓冲区。
	void recordCommands();         // 把 RenderPass、管线绑定和绘制命令写入 Command Buffer。
	void createSynchronization();  // 创建按 currentFrame 或交换链图像编号使用的同步对象。

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
	VkImageView createImageView(VkImage image, VkFormat format, VkImageAspectFlags aspectFlags); // 创建图像访问视图。
	VkShaderModule createShaderModule(const std::vector<char>& code);                            // 把 SPIR-V 包装成 Shader Module。

public:
	VulkanRenderer();
	~VulkanRenderer();

	int init(GLFWwindow* newWindow);
	void draw();
	void cleanup();


};

