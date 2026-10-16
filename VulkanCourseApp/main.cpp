#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <vector>
#include <stdexcept>
#include <iostream>

#include "VulkanRenderer.h"

GLFWwindow* window;
VulkanRenderer vulkanRenderer;

void initWindow(std::string wName = "Test Window", const int width = 800, const int height = 600) {
	// 初始化 GLFW
	glfwInit();

	// 设置 GLFW 不创建 OpenGL 上下文
	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
	glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

	window = glfwCreateWindow(width, height, wName.c_str(), nullptr, nullptr);
}

int main() {
	// ###############################################################################
	// # 程序最顶层流程                                                              #
	// #                                                                             #
	// # 1. 创建窗口                                                                 #
	// # 2. 初始化 Vulkan 渲染器                                                     #
	// # 3. 循环绘制并呈现                                                           #
	// # 4. 反向释放资源                                                             #
	// ###############################################################################

	// 总流程第 1 步：创建供 Vulkan 显示图像的 GLFW 窗口。
	initWindow("Vulkan Window", 800, 600);

	// 总流程第 2 步：创建 Vulkan 对象、渲染资源和同步对象。
	if (vulkanRenderer.init(window) == EXIT_FAILURE) {
		// 渲染器会清理已创建的 Vulkan 对象；窗口和 GLFW 由这里负责清理。
		glfwDestroyWindow(window);
		glfwTerminate();
		return EXIT_FAILURE;
	}

	// 总流程第 3 步：不断处理窗口事件，并绘制、呈现下一张图像。
	while (!glfwWindowShouldClose(window)) {
		glfwPollEvents();
		// 每轮循环提交一帧绘制任务，并把画好的交换链图像提交给呈现队列。
		vulkanRenderer.draw();
	}

	// 总流程第 4 步：等待 GPU 空闲，并按依赖关系的反方向释放资源。
	vulkanRenderer.cleanup();

	// Vulkan 已不再使用窗口，此时才能销毁窗口并结束 GLFW。
	glfwDestroyWindow(window);
	glfwTerminate();

	return 0;
}
