#version 450

// 接收顶点着色器从 location 0 输出的颜色。
// 三角形内部的 fragColour 已由光栅化阶段根据三个顶点的颜色自动插值。
layout(location = 0) in vec3 fragColour;

// 把最终颜色写到第 0 个颜色附件；以后它通常对应交换链图像。
layout(location = 0) out vec4 outColour;

void main() {
    // 片段着色器大致对三角形覆盖的每个像素运行一次。
    // fragColour 提供 RGB；补上的 alpha = 1.0 表示完全不透明。
    outColour = vec4(fragColour, 1.0);
}
