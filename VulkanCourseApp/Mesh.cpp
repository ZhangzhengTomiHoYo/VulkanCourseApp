#include "Mesh.h"

#include <cstring>

Mesh::Mesh()
{
	// VulkanRenderer 作为成员保存 helloMesh，因此会先调用默认构造函数。
	// 真正取得逻辑设备以后，init() 才能创建带顶点缓冲区的 Mesh。
}

Mesh::Mesh(VkPhysicalDevice newPhysicalDevice, VkDevice newDevice, VkQueue transferQueue, VkCommandPool transferCommandPool, std::vector<Vertex>* vertices, std::vector<uint32_t>* indices, int newTexId)
{
	// size() 返回元素数量，不是字节数；索引绘制最终使用 indexCount 决定读取多少个索引。
	vertexCount = static_cast<uint32_t>(vertices->size());
	indexCount = static_cast<uint32_t>(indices->size());

	// 保存设备句柄，后面创建 Buffer、寻找内存类型和释放资源都要使用它们。
	physicalDevice = newPhysicalDevice;
	device = newDevice;

	// 顶点和索引使用两份独立 Buffer，但都通过暂存 Buffer 上传到 NVIDIA 设备本地显存。
	createVertexBuffer(transferQueue, transferCommandPool, vertices);
	createIndexBuffer(transferQueue, transferCommandPool, indices);

	// 模型矩阵初始化为单位矩阵：不做任何平移/旋转/缩放，顶点保持在建模空间的原始位置。
	model.model = glm::mat4(1.0f);
	texId = newTexId;
}

void Mesh::setModel(glm::mat4 newModel)
{
	// Model 是与着色器 push constant 块对应的外层结构；真正保存模型变换的是其中的 model 成员。
	model.model = newModel;
}

Model Mesh::getModel()
{
	return model;
}

int Mesh::getTexId()
{
	return texId;
}

uint32_t Mesh::getVertexCount()
{
	return vertexCount;
}

VkBuffer Mesh::getVertexBuffer()
{
	return vertexBuffer;
}

uint32_t Mesh::getIndexCount()
{
	return indexCount;
}

VkBuffer Mesh::getIndexBuffer()
{
	return indexBuffer;
}

void Mesh::destroyBuffers()
{
	// 每个 Buffer 都使用自己绑定的 VkDeviceMemory，因此分别按“先 Buffer、后内存”释放。
	// 它们都是 VkDevice 的子对象，必须在 vkDestroyDevice() 之前释放。
	if (vertexBuffer != VK_NULL_HANDLE) {
		vkDestroyBuffer(device, vertexBuffer, nullptr);
		vertexBuffer = VK_NULL_HANDLE;
	}
	if (vertexBufferMemory != VK_NULL_HANDLE) {
		vkFreeMemory(device, vertexBufferMemory, nullptr);
		vertexBufferMemory = VK_NULL_HANDLE;
	}
	if (indexBuffer != VK_NULL_HANDLE) {
		vkDestroyBuffer(device, indexBuffer, nullptr);
		indexBuffer = VK_NULL_HANDLE;
	}
	if (indexBufferMemory != VK_NULL_HANDLE) {
		vkFreeMemory(device, indexBufferMemory, nullptr);
		indexBufferMemory = VK_NULL_HANDLE;
	}
}

Mesh::~Mesh()
{
	// Buffer 与显存的释放统一由 destroyBuffers() 负责，析构函数本身不重复释放。
}

void Mesh::createVertexBuffer(VkQueue transferQueue, VkCommandPool transferCommandPool, std::vector<Vertex>* vertices)
{
	// ###############################################################################
	// # 顶点 Buffer 创建总流程                                                      #
	// #                                                                             #
	// # 1. 计算顶点数据的字节数                                                     #
	// # 2. 创建 CPU 可写的暂存 Buffer                                               #
	// # 3. 把 CPU 顶点数据复制到暂存内存                                            #
	// # 4. 创建 NVIDIA 设备本地的正式顶点 Buffer                                    #
	// # 5. 把暂存 Buffer 复制到正式顶点 Buffer                                      #
	// # 6. 等待复制完成并释放暂存资源                                                #
	// ###############################################################################

	// 顶点 Buffer 第 1 步：计算所有顶点一共占用多少字节。
	// VkDeviceSize 表示字节大小；VkDevice 则是逻辑设备句柄，不能用来保存大小。
	VkDeviceSize bufferSize = sizeof(Vertex) * vertices->size();

	// 顶点 Buffer 第 2 步：创建临时的暂存 Buffer。
	// stagingBuffer 声明为 TRANSFER_SRC，表示它可以作为 Buffer 复制操作的数据来源。
	VkBuffer stagingBuffer = VK_NULL_HANDLE;
	VkDeviceMemory stagingBufferMemory = VK_NULL_HANDLE;

	// HOST_VISIBLE：CPU 可以使用 vkMapMemory() 取得访问地址。
	// HOST_COHERENT：memcpy() 写入后不需要再调用 vkFlushMappedMemoryRanges()。
	createBuffer(physicalDevice, device, bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, &stagingBuffer, &stagingBufferMemory);

	// 顶点 Buffer 第 3 步：把暂存内存映射为 CPU 地址，并复制全部顶点字节。
	// data 指向的是 stagingBufferMemory，不是最终的 vertexBufferMemory。
	void* data = nullptr;
	VkResult result = vkMapMemory(device, stagingBufferMemory, 0, bufferSize, 0, &data);
	if (result != VK_SUCCESS) {
		vkDestroyBuffer(device, stagingBuffer, nullptr);
		vkFreeMemory(device, stagingBufferMemory, nullptr);
		throw std::runtime_error("Failed to map vertex staging buffer memory!");
	}
	// vector 的元素连续存放；data() 给出首地址，把全部 Vertex 原样复制到映射内存。
	memcpy(data, vertices->data(), static_cast<size_t>(bufferSize));
	// 解除映射只会撤销 CPU 地址；刚写入的顶点数据仍保存在暂存内存中。
	vkUnmapMemory(device, stagingBufferMemory);

	// 顶点 Buffer 第 4 步：创建最终供顶点输入阶段读取的 Buffer。
	// TRANSFER_DST：允许它接收另一个 Buffer 复制来的数据。
	// VERTEX_BUFFER：允许后续通过 vkCmdBindVertexBuffers() 绑定为顶点数据源。
	// DEVICE_LOCAL：要求使用 NVIDIA 设备本地内存，适合 GPU 高频读取。
	createBuffer(physicalDevice, device, bufferSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, &vertexBuffer, &vertexBufferMemory);

	// 顶点 Buffer 第 5 步：录制并提交 Buffer 复制命令。
	// copyBuffer() 返回时已经等待 transferQueue 完成，因此正式顶点 Buffer 已经含有数据。
	copyBuffer(device, transferQueue, transferCommandPool, stagingBuffer, vertexBuffer, bufferSize);

	// 顶点 Buffer 第 6 步：复制完成后，暂存 Buffer 已完成使命，可以立即释放。
	// 正式顶点数据保存在 vertexBufferMemory 中，不会随暂存资源销毁而消失。
	vkDestroyBuffer(device, stagingBuffer, nullptr);
	vkFreeMemory(device, stagingBufferMemory, nullptr);
}

void Mesh::createIndexBuffer(VkQueue transferQueue, VkCommandPool transferCommandPool, std::vector<uint32_t>* indices)
{
	// ###############################################################################
	// # 索引 Buffer 创建总流程                                                      #
	// #                                                                             #
	// # 1. 计算索引数据的字节数                                                     #
	// # 2. 创建 CPU 可写的暂存 Buffer                                               #
	// # 3. 把 CPU 索引数据复制到暂存内存                                            #
	// # 4. 创建 NVIDIA 设备本地的正式索引 Buffer                                    #
	// # 5. 把暂存 Buffer 复制到正式索引 Buffer                                      #
	// # 6. 等待复制完成并释放暂存资源                                                #
	// ###############################################################################

	// 索引 Buffer 第 1 步：每个索引都是 uint32_t，总大小为单个索引大小乘以索引数量。
	VkDeviceSize bufferSize = sizeof(uint32_t) * indices->size();

	// 索引 Buffer 第 2 步：创建 CPU 可写、并且允许作为复制来源的暂存 Buffer。
	VkBuffer stagingBuffer = VK_NULL_HANDLE;
	VkDeviceMemory stagingBufferMemory = VK_NULL_HANDLE;
	createBuffer(physicalDevice, device, bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, &stagingBuffer, &stagingBufferMemory);

	// 索引 Buffer 第 3 步：把暂存内存映射成 CPU 地址，再复制全部索引字节。
	// data 指向 stagingBufferMemory，不是最终的 indexBufferMemory。
	void* data = nullptr;
	VkResult result = vkMapMemory(device, stagingBufferMemory, 0, bufferSize, 0, &data);
	if (result != VK_SUCCESS) {
		vkDestroyBuffer(device, stagingBuffer, nullptr);
		vkFreeMemory(device, stagingBufferMemory, nullptr);
		throw std::runtime_error("Failed to map index staging buffer memory!");
	}
	// vector 的元素连续存放；data() 给出首地址，把全部 uint32_t 索引原样复制进去。
	memcpy(data, indices->data(), static_cast<size_t>(bufferSize));
	// 解除映射只撤销 CPU 地址；索引数据仍保存在暂存内存中。
	vkUnmapMemory(device, stagingBufferMemory);

	// 索引 Buffer 第 4 步：TRANSFER_DST 允许接收复制，INDEX_BUFFER 允许用于索引绘制。
	// DEVICE_LOCAL 要求把正式索引数据保存在 NVIDIA 设备本地内存中。
	createBuffer(physicalDevice, device, bufferSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, &indexBuffer, &indexBufferMemory);

	// 索引 Buffer 第 5 步：提交复制，并等待正式索引 Buffer 获得全部数据。
	copyBuffer(device, transferQueue, transferCommandPool, stagingBuffer, indexBuffer, bufferSize);

	// 索引 Buffer 第 6 步：复制完成后释放临时 Buffer 及其内存。
	vkDestroyBuffer(device, stagingBuffer, nullptr);
	vkFreeMemory(device, stagingBufferMemory, nullptr);
}


