#version 450

// location 必须与 VkVertexInputAttributeDescription::location 一致。
layout(location = 0) in vec3 pos; // 从 Vertex::pos 读取当前顶点的位置。
layout(location = 1) in vec3 col; // 从 Vertex::col 读取当前顶点的颜色。

// 把颜色传给片段着色器；相同 location 表示两个着色器阶段的接口相连。
layout(location = 0) out vec3 fragColour;

void main() {
    // 当前没有 Model、View、Projection 矩阵，直接把输入位置作为裁剪空间坐标。
    gl_Position = vec4(pos, 1.0);
    // 三个顶点的颜色会在光栅化阶段插值，片段着色器会收到插值后的颜色。
    fragColour = col;
}
