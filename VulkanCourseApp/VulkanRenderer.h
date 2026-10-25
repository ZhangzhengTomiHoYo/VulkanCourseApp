#pragma once
#define NOMINMAX
#define VK_USE_PLATFORM_WIN32_KHR
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <vector>
#include <set>
#include <stdexcept>
#include <iostream>
#include <algorithm>
#include <limits>
#include <array>
#include "Utilities.h"
#include "Mesh.h"

#include "stb_image.h"

class VulkanRenderer
{
private:
	GLFWwindow* window = nullptr; // GLFW 窗口；创建 Surface 和查询实际像素尺寸时使用。

	int currentFrame = 0; // 当前使用第几组帧同步对象；只在 0 到 MAX_FRAME_DRAWS - 1 之间循环。

	// 场景对象
	std::vector<Mesh> meshList; // 保存场景中所有 Mesh 对象；每个 Mesh 都有自己的顶点缓冲区和索引缓冲区。

	// binding 0 的 View/Projection 数据块；每帧通过 updateUniformBuffer() 写入对应交换链图像的 Uniform Buffer。
	struct UboViewProjection{
		glm::mat4 view; // 世界空间 -> 观察空间（相机）。
		glm::mat4 proj; // 观察空间 -> 裁剪空间（透视投影）。
	} uboViewProjection;

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

	VkImage depthBufferImage;
	VkDeviceMemory depthBufferImageMemory;
	VkImageView depthBufferImageView;

	VkSampler textureSampler;

	// - 描述符集 Descriptors
	VkDescriptorSetLayout descriptorSetLayout; // 声明各 binding 的类型与使用阶段（binding 0 为 view/proj UBO）。
	VkDescriptorSetLayout samplerSetLayout;
	VkPushConstantRange pushConstantRange;     // Push Constant 改造：声明 PushModel 的阶段、偏移和大小（待在 createPushConstantRange() 中填写）。

	VkDescriptorPool descriptorPool = VK_NULL_HANDLE; // 分配描述符集的池；销毁池会同时释放其中的描述符集。
	VkDescriptorPool samplerDescriptorPool;
	std::vector<VkDescriptorSet> descriptorSets;      // 每张交换链图像一个；录制命令时绑定到管线。
	std::vector<VkDescriptorSet> samplerDescriptorSets;

	// 每张交换链图像一份 view/proj Uniform Buffer 及其设备内存（每帧只写对应下标，避免读写冲突）。
	std::vector<VkBuffer> viewProjUniformBuffers;
	std::vector<VkDeviceMemory> viewProjUniformBuffersMemory;

	// Push Constants 改造：动态 Model Uniform Buffer 已暂停创建，vector 保留为空，cleanup() 仍可安全遍历/clear。
	std::vector<VkBuffer> modelUniformBuffers;
	std::vector<VkDeviceMemory> modelUniformBuffersMemory;

	//  -Assets
	std::vector<VkImage> textureImages;
	std::vector<VkDeviceMemory> textureImageMemory;
	std::vector<VkImageView> textureImageViews;

	// - Pipeline
	VkPipelineLayout pipelineLayout = VK_NULL_HANDLE; // 着色器资源接口：绑定 1 个描述符集布局（view/proj）；Push Constant 范围待接入。
	VkRenderPass renderPass = VK_NULL_HANDLE;         // 规定附件、Subpass 及图像布局变化。
	VkPipeline graphicsPipeline = VK_NULL_HANDLE;     // 汇总着色器、光栅化、混色等图形处理规则。

	// - Pools
	VkCommandPool graphicsCommandPool = VK_NULL_HANDLE; // 为图形队列族管理 Command Buffer 的分配内存。

	// - Utility
	VkFormat swapChainImageFormat = VK_FORMAT_UNDEFINED; // 交换链图像采用的像素格式。
	VkExtent2D swapChainExtent{};                        // 交换链图像的实际宽高。

	//VkDeviceSize minUniformBufferOffsetAlignment = 1; // GPU 要求动态 Uniform 起始地址遵守的最小对齐值。
	//VkDeviceSize modelUniformAlignment = 0;           // 一份 Model 对齐后的跨度；用它定位下一份模型数据。
	//Model* modelTransferSpace = nullptr;            // CPU 端对齐的临时区；每帧先在这里整理所有模型矩阵。

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
	void createDescriptorSetLayout(); // 声明 binding 0 的 view/proj UBO 描述符布局（binding 1 已废弃）。
	void createGraphicsPipeline(); // 创建着色器模块并组装固定图形管线。
	void createFramebuffers();     // 把每张交换链 ImageView 绑定到 RenderPass 附件。
	void createCommandPool();      // 为图形队列族创建 Command Buffer 内存池。
	void createCommandBuffers();   // 从 Command Pool 为每张交换链图像分配命令缓冲区。
	void recordCommands(uint32_t currentImage);         // 把 RenderPass、管线绑定和绘制命令写入 Command Buffer；Push Constants 改造后每帧重新录制。
	void createSynchronization();  // 创建按 currentFrame 或交换链图像编号使用的同步对象。
	void createUniformBuffers();   // 为每张交换链图像创建 view/proj Uniform Buffer（动态 Model Buffer 已废弃）。
	void createDescriptorPool();   // 创建描述符池，供每张交换链图像分配一个描述符集。
	void createDescriptorSets();   // 分配描述符集并把各 Uniform Buffer 写入对应 binding。
	void createPushConstantRange(); // Push Constant 改造：填写 pushConstantRange 的阶段、偏移与大小（cpp 中尚未实现，跟视频补）。

	void createDepthBufferImage();

	void createTextureSampler();

	void updateUniformBuffer(uint32_t imageIndex); // 每帧把 view/proj 矩阵写入本次交换链图像对应的 Uniform Buffer。

	// - Get
	void getPhysicalDevice(); // 枚举并选出满足要求的 GPU（Push Constants 改造后不再查询动态 UBO 对齐值）。

	// Push Constants 改造：CPU 对齐临时区相关函数暂停使用，声明一并注释。
	//void allocateDynamicBufferTransferSpace();

	// - Support
	// -- Checker
	bool checkInstanceExtensionSupport(std::vector<const char*>* checkExtensions); // 检查实例层是否支持所需扩展。
	bool checkDeviceExtensionSupport(VkPhysicalDevice device); // 检查指定 GPU 是否支持所需设备扩展。
	bool checkDeviceSuitable(VkPhysicalDevice device); // 检查队列族、扩展和交换链支持是否全部满足。
	// -- Getter
	QueueFamilyIndices getQueueFamilies(VkPhysicalDevice device); // 取得图形/呈现队列族编号。
	SwapChainDetails getSwapChainDetails(VkPhysicalDevice device); // 取得交换链能力（格式、呈现模式、分辨率）。
	// -- Choose
	VkSurfaceFormatKHR chooseBestSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& formats); // 从支持格式中选颜色格式。
	VkPresentModeKHR chooseBestPresentationMode(const std::vector<VkPresentModeKHR>& presentationModes); // 从呈现模式中选呈现模式。
	VkExtent2D chooseSwapExtent(const VkSurfaceCapabilitiesKHR& surfaceCapabilities); // 在窗口尺寸和图像能力之间确定交换链分辨率。

	VkFormat chooseSupportedFormat(const std::vector<VkFormat>& formats, VkImageTiling tiling, VkFormatFeatureFlags featureFlags);

	// -- Create
	VkImage createImage(uint32_t width, uint32_t height, VkFormat format, VkImageTiling tiling, VkImageUsageFlags useFlags, VkMemoryPropertyFlags propFlags, VkDeviceMemory* imageMemory);

	VkImageView createImageView(VkImage image, VkFormat format, VkImageAspectFlags aspectFlags); // 创建图像访问视图。
	VkShaderModule createShaderModule(const std::vector<char>& code);                            // 把 SPIR-V 包装成 Shader Module。

	int createTextureImage(std::string fileName);
	int createTexture(std::string fileName);
	int createTextureDescriptor(VkImageView textureImage);

	// -- Loader
	stbi_uc* loadTextureFile(std::string fileName, int* width, int* height, VkDeviceSize* imageSize);

public:
	VulkanRenderer();
	~VulkanRenderer();

	int init(GLFWwindow* newWindow); // 按依赖顺序创建全部 Vulkan 对象；失败时清理并返回 EXIT_FAILURE。

	void updateModel(int modelID, glm::mat4 newModel); // 主循环每帧调用：写入指定 Mesh 的模型矩阵，越界抛出 out_of_range。

	void draw();    // 取得交换链图像、每帧录制并提交一帧绘制命令，再提交呈现。
	void cleanup(); // 等待 GPU 空闲后，按创建依赖的反方向释放全部 Vulkan 资源。


};

