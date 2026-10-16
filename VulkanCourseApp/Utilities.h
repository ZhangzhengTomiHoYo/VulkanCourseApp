#pragma once
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <cstdint>
#include <fstream>
#include <stdexcept>
#include <string>
#include <vector>

#include <glm/glm.hpp>

// 程序准备两组可循环使用的帧同步对象，限制最多有两帧绘制任务处于处理中。
// 它不是交换链图像数量；交换链图像数量由 Vulkan 和窗口系统共同决定。
const int MAX_FRAME_DRAWS = 2;

const std::vector<const char*> deviceExtensions = {
	VK_KHR_SWAPCHAIN_EXTENSION_NAME
};

struct Vertex
{
	glm::vec3 pos; // 顶点位置，对应顶点着色器的 layout(location = 0) in vec3 pos。
	glm::vec3 col; // 顶点颜色，对应顶点着色器的 layout(location = 1) in vec3 col。
};

// 队列族 的 索引(位置)
struct QueueFamilyIndices
{
	int graphicsFamily = -1;
	int presentationFamily = -1;

	// 检查队列族是否有效
	bool isValid() {
		return graphicsFamily >= 0 && presentationFamily >= 0;
	}
};

struct SwapChainDetails {
	VkSurfaceCapabilitiesKHR surfaceCapabilities;        // 表面属性，例如图像尺寸
	std::vector<VkSurfaceFormatKHR> formats;             // 表面图像格式，例如 RGBA 和每个颜色分量的位数
	std::vector<VkPresentModeKHR> presentationModes;     // 图像呈现到屏幕的方式
};

struct SwapchainImage {
	VkImage image;         // 交换链提供的图像句柄，随交换链销毁，不由我们单独释放。
	VkImageView imageView; // 我们创建的访问视图，必须在销毁交换链之前释放。
};

// 把整个二进制文件读进连续内存，主要用于读取编译后的 SPIR-V 着色器。
// static 在这里表示“当前编译单元私有”。由于本函数定义在头文件中，
// 每个包含 Utilities.h 的 .cpp 都会得到自己的一份 readFile()。
// filename 使用 const 引用传入：不复制字符串，也不允许函数修改文件名。
static std::vector<char> readFile(const std::string& filename) {
	// binary：按原始字节读取，避免文本模式转换内容。
	// ate：打开后先把读取位置放到文件末尾，方便用 tellg() 得到文件大小。
	std::ifstream file(filename, std::ios::ate | std::ios::binary);

	// 文件不存在、相对路径错误或没有读取权限时，打开会失败。
	if (!file.is_open()) {
		throw std::runtime_error("Failed to open file: " + filename);
	}

	// 因为使用了 ate，当前读取位置就在文件末尾，所以该位置也就是文件字节数。
	// tellg() 返回流的位置类型；转换成 size_t 后可用于创建 vector。
	size_t fileSize = static_cast<size_t>(file.tellg());

	// 一次准备与文件等大的连续内存。char 占一个字节，适合原样保存二进制数据。
	std::vector<char> buffer(fileSize);

	// 刚才只是到末尾测量大小；正式读取前把读取位置移回文件开头。
	file.seekg(0);

	// data() 返回 vector 第一个字节的地址，read() 从这里开始写入 fileSize 个字节。
	file.read(buffer.data(), fileSize);

	// close() 在这里写出来便于学习；即使省略，ifstream 析构时也会自动关闭文件。
	file.close();
	// 返回装有全部 SPIR-V 字节的 vector。现代 C++ 通常会省略不必要的复制。
	return buffer;
}

static uint32_t findMemoryTypeIndex(VkPhysicalDevice physicalDevice, uint32_t allowedTypes, VkMemoryPropertyFlags properties)
{
	// ###############################################################################
	// # 内存类型选择流程（顶点 Buffer 第 4 步）                                     #
	// #                                                                             #
	// # 1. 查询物理设备提供的全部内存类型                                           #
	// # 2. 筛选 Buffer 允许并且具备所需属性的类型                                   #
	// ###############################################################################
	// 查询物理设备提供的全部内存类型，以及每种类型支持的属性。
	VkPhysicalDeviceMemoryProperties memoryProperties;
	vkGetPhysicalDeviceMemoryProperties(physicalDevice, &memoryProperties);

	for (uint32_t i = 0; i < memoryProperties.memoryTypeCount; i++) {
		// allowedTypes 的第 i 位为 1，表示当前 Buffer 允许使用第 i 种内存类型。
		// 第二个条件要求该类型同时包含调用者指定的全部属性位。
		if ((allowedTypes & (1 << i)) && (memoryProperties.memoryTypes[i].propertyFlags & properties) == properties) {
			return i;
		}
	}

	throw std::runtime_error("Failed to find suitable memory type!");
}

static void createBuffer(VkPhysicalDevice physicalDevice, VkDevice device, VkDeviceSize bufferSize, VkBufferUsageFlags bufferUsage, VkMemoryPropertyFlags bufferProperties, VkBuffer* buffer, VkDeviceMemory* bufferMemory) {
	// ###############################################################################
	// # 通用 Buffer 创建总流程                                                      #
	// #                                                                             #
	// # 1. 创建 Buffer                                                              #
	// # 2. 查询 Buffer 对内存的要求                                                 #
	// # 3. 选择合适的内存类型                                                       #
	// # 4. 分配 VkDeviceMemory                                                      #
	// # 5. 把 Buffer 与内存绑定                                                     #
	// ###############################################################################

	// 通用 Buffer 第 1 步：创建 Buffer；此时只有句柄，还没有实际存储空间。
	VkBufferCreateInfo bufferCreateInfo{};
	bufferCreateInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	bufferCreateInfo.size = bufferSize;       // Buffer 需要容纳多少字节。
	bufferCreateInfo.usage = bufferUsage;     // Buffer 将被用作顶点、索引或其他用途。
	bufferCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE; // 当前只交给一个队列族使用。

	VkResult result = vkCreateBuffer(device, &bufferCreateInfo, nullptr, buffer);
	if (result != VK_SUCCESS) {
		throw std::runtime_error("Failed to create buffer!");
	}

	// 通用 Buffer 第 2 步：查询大小、对齐和允许使用的内存类型。
	VkMemoryRequirements memRequirements{};
	vkGetBufferMemoryRequirements(device, *buffer, &memRequirements);

	// 通用 Buffer 第 3、4 步：选择满足要求的类型，并分配实际内存。
	VkMemoryAllocateInfo memoryAllocInfo{};
	memoryAllocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	// 使用查询结果中的 size，因为它可能包含 Vulkan 要求的对齐空间。
	memoryAllocInfo.allocationSize = memRequirements.size;
	memoryAllocInfo.memoryTypeIndex = findMemoryTypeIndex(
		physicalDevice,
		memRequirements.memoryTypeBits,
		bufferProperties
	);

	result = vkAllocateMemory(device, &memoryAllocInfo, nullptr, bufferMemory);
	if (result != VK_SUCCESS) {
		vkDestroyBuffer(device, *buffer, nullptr);
		*buffer = VK_NULL_HANDLE;
		throw std::runtime_error("Failed to allocate buffer memory!");
	}

	// 通用 Buffer 第 5 步：让 Buffer 使用刚分配的内存。
	result = vkBindBufferMemory(device, *buffer, *bufferMemory, 0);
	if (result != VK_SUCCESS) {
		vkFreeMemory(device, *bufferMemory, nullptr);
		vkDestroyBuffer(device, *buffer, nullptr);
		*bufferMemory = VK_NULL_HANDLE;
		*buffer = VK_NULL_HANDLE;
		throw std::runtime_error("Failed to bind buffer memory!");
	}
}

static void copyBuffer(VkDevice device, VkQueue transferQueue, VkCommandPool transferCommandPool, VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize bufferSize) {
	// ###############################################################################
	// # 一次性 Buffer 复制总流程                                                    #
	// #                                                                             #
	// # 1. 从 Command Pool 分配临时 Command Buffer                                  #
	// # 2. 录制 vkCmdCopyBuffer()                                                   #
	// # 3. 把复制命令提交给 Queue                                                   #
	// # 4. 等待 Queue 完成复制                                                      #
	// # 5. 释放临时 Command Buffer                                                  #
	// ###############################################################################

	// 第 1 步：临时 Command Buffer 只负责这一次上传，完成后立即释放。
	VkCommandBuffer transferCommandBuffer = VK_NULL_HANDLE;

	VkCommandBufferAllocateInfo commandBufferAllocateInfo{};
	commandBufferAllocateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	commandBufferAllocateInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	// 必须传入已经创建、并且属于 transferQueue 所在队列族的 Command Pool。
	commandBufferAllocateInfo.commandPool = transferCommandPool;
	commandBufferAllocateInfo.commandBufferCount = 1;

	VkResult result = vkAllocateCommandBuffers(device, &commandBufferAllocateInfo, &transferCommandBuffer);
	if (result != VK_SUCCESS) {
		throw std::runtime_error("Failed to allocate transfer command buffer!");
	}

	// 第 2 步：ONE_TIME_SUBMIT 表示这份命令只会提交一次。
	VkCommandBufferBeginInfo commandBufferBeginInfo{};
	commandBufferBeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	commandBufferBeginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

	result = vkBeginCommandBuffer(transferCommandBuffer, &commandBufferBeginInfo);
	if (result != VK_SUCCESS) {
		vkFreeCommandBuffers(device, transferCommandPool, 1, &transferCommandBuffer);
		throw std::runtime_error("Failed to begin transfer command buffer!");
	}

	// 描述复制范围：从源 Buffer 的第 0 字节开始，复制 bufferSize 字节到目标 Buffer 的第 0 字节。
	VkBufferCopy bufferCopyRegion{};
	bufferCopyRegion.srcOffset = 0;
	bufferCopyRegion.dstOffset = 0;
	bufferCopyRegion.size = bufferSize;

	vkCmdCopyBuffer(transferCommandBuffer, srcBuffer, dstBuffer, 1, &bufferCopyRegion);

	result = vkEndCommandBuffer(transferCommandBuffer);
	if (result != VK_SUCCESS) {
		vkFreeCommandBuffers(device, transferCommandPool, 1, &transferCommandBuffer);
		throw std::runtime_error("Failed to end transfer command buffer!");
	}

	// 第 3 步：把刚录制的复制命令提交给 transferQueue。
	VkSubmitInfo submitInfo{};
	submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &transferCommandBuffer;

	result = vkQueueSubmit(transferQueue, 1, &submitInfo, VK_NULL_HANDLE);
	if (result != VK_SUCCESS) {
		vkFreeCommandBuffers(device, transferCommandPool, 1, &transferCommandBuffer);
		throw std::runtime_error("Failed to submit buffer copy command!");
	}

	// 第 4 步：必须等复制完成，之后才可以销毁作为复制来源的 stagingBuffer。
	result = vkQueueWaitIdle(transferQueue);
	if (result != VK_SUCCESS) {
		vkFreeCommandBuffers(device, transferCommandPool, 1, &transferCommandBuffer);
		throw std::runtime_error("Failed to wait for buffer copy queue!");
	}

	// 第 5 步：复制已经完成，这份临时命令不再需要。
	vkFreeCommandBuffers(device, transferCommandPool, 1, &transferCommandBuffer);
}
