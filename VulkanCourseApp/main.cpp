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
	// 创建窗口
	initWindow("Vulkan Window", 800, 600);

	// 创建 Vulkan 渲染器
	if (vulkanRenderer.init(window) == EXIT_FAILURE) {
		return EXIT_FAILURE;
	}

	// 循环直到用户关闭窗口
	while (!glfwWindowShouldClose(window)) {
		glfwPollEvents();
	}

	vulkanRenderer.cleanup();

	// 销毁窗口
	glfwDestroyWindow(window);
	glfwTerminate();

	return 0;
}