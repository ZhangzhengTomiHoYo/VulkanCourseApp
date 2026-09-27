#pragma once

// 队列族 的 索引(位置)
struct QueueFamilyIndices
{
	int graphicsFamily = -1;

	// 检查队列族是否有效
	bool isValid() {
		return graphicsFamily >= 0;
	}
};
