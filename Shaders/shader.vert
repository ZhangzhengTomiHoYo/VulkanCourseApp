#version 450

// location 必须与 VkVertexInputAttributeDescription::location 一致。
layout(location = 0) in vec3 pos; // 从 Vertex::pos 读取当前顶点的位置。
layout(location = 1) in vec3 col; // 从 Vertex::col 读取当前顶点的颜色。

layout(binding = 0) uniform UboViewProjection {
    mat4 view;
    mat4 proj;
} uboViewProjection;

// Push Constants 改造：模型矩阵不再走动态 Uniform Buffer，binding 1 暂停使用。
//layout(binding = 1) uniform UboModel {
//    mat4 model;
//} uboModel;

// Push Constant 块：不需要 descriptor 或 buffer，模型矩阵由 C++ 端 vkCmdPushConstants 直接写进命令流。
// 它必须与 C++ 端 VkPushConstantRange 声明的 stage/offset/size 一致；一个 mat4 为 64 字节。
layout(push_constant) uniform PushModel {
    mat4 model;
} pushModel;

// 把颜色传给片段着色器；相同 location 表示两个着色器阶段的接口相连。
layout(location = 0) out vec3 fragColour;

void main() {
    gl_Position = uboViewProjection.proj * uboViewProjection.view * pushModel.model * vec4(pos, 1.0);

    fragColour = col;
}
