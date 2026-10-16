#pragma once

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <vector>

#include "Utilities.h"

class Mesh
{
public:
	Mesh(); // 先创建一个空 Mesh，VulkanRenderer::init() 中再为它创建顶点缓冲区。
	Mesh(VkPhysicalDevice newPhysicalDevice, VkDevice newDevice, VkQueue transferQueue, VkCommandPool transferCommandPool, std::vector<Vertex>* vertices, std::vector<uint32_t>* indices); // 根据 CPU 顶点数组创建 Mesh。

	uint32_t getVertexCount();  // 返回顶点数组中的 Vertex 数量；当前索引绘制不直接使用它。
	VkBuffer getVertexBuffer(); // 返回顶点缓冲区句柄，供 vkCmdBindVertexBuffers() 绑定。

	uint32_t getIndexCount();  // 返回索引数量，供 vkCmdDrawIndexed() 决定绘制多少个索引。
	VkBuffer getIndexBuffer(); // 返回索引缓冲区句柄，供 vkCmdBindIndexBuffer() 绑定。

	void destroyBuffers(); // 在销毁逻辑设备之前，销毁 Buffer 并释放它使用的显存。

	~Mesh();

private:
	uint32_t vertexCount = 0;                         // 这份 Mesh 保存了多少个 Vertex；类型与 vkCmdDraw() 的要求一致。
	VkBuffer vertexBuffer = VK_NULL_HANDLE;           // Buffer 句柄：描述一段可作为顶点输入的数据区域。
	VkDeviceMemory vertexBufferMemory = VK_NULL_HANDLE; // Buffer 实际绑定的设备内存。

	uint32_t indexCount = 0;                         // 索引数量；作为 vkCmdDrawIndexed() 的 indexCount。
	VkBuffer indexBuffer = VK_NULL_HANDLE;           // 保存顶点编号的索引 Buffer。
	VkDeviceMemory indexBufferMemory = VK_NULL_HANDLE; // 索引 Buffer 绑定的 NVIDIA 设备本地内存。

	VkPhysicalDevice physicalDevice = VK_NULL_HANDLE; // 查询 GPU 提供的内存类型时使用。
	VkDevice device = VK_NULL_HANDLE;                 // 创建、销毁 Buffer 和分配、释放内存时使用。

	void createVertexBuffer(VkQueue transferQueue, VkCommandPool transferCommandPool, std::vector<Vertex>* vertices); // 通过暂存 Buffer 把 CPU 顶点上传到 NVIDIA 设备本地显存。
	void createIndexBuffer(VkQueue transferQueue, VkCommandPool transferCommandPool, std::vector<uint32_t>* indices);  // 通过暂存 Buffer 把 CPU 索引上传到 NVIDIA 设备本地显存。
};

