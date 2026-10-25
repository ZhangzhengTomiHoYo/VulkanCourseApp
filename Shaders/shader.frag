#version 450

// 接收顶点着色器 location 0 输出的、经过光栅化插值后的颜色。
layout(location = 0) in vec3 fragColour;
layout(location = 1) in vec2 fragTex;

layout(set = 1, binding = 0) uniform sampler2D textureSampler;

// 写入 RenderPass 第 0 个颜色附件，当前对应交换链图像。
layout(location = 0) out vec4 outColour;

void main() {
    // fragColour 提供 RGB，alpha = 1.0 表示完全不透明。
    outColour = texture(textureSampler, fragTex);


}
