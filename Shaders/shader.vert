#version 450

// 顶点着色器通过 location 0 把颜色传给片段着色器。
// location 可以理解为两个着色器阶段之间约定好的“数据接口编号”。
layout(location = 0) out vec3 fragColour;

// 暂时不使用顶点缓冲区，直接在着色器中写死三个顶点的位置。
// 每运行一次顶点着色器，gl_VertexIndex 会指出当前正在处理第几个顶点。
vec3 positions[3] = vec3[](
    vec3(0.0, -0.4, 0.0),
    vec3(0.4, 0.4, 0.0),
    vec3(-0.4, 0.4, 0.0)
);

// 为三个顶点分别准备红、绿、蓝三种颜色，分量范围为 0.0 到 1.0。
vec3 colours[3] = vec3[](
    vec3(1.0, 0.0, 0.0),
    vec3(0.0, 1.0, 0.0),
    vec3(0.0, 0.0, 1.0)
);

void main() {
    // 顶点着色器每次只处理一个顶点。
    // Vulkan 自动提供 gl_VertexIndex；绘制三个顶点时，它分别为 0、1、2。
    // gl_Position 是必须写入的内置输出，表示当前顶点的齐次裁剪空间坐标 (x,y,z,w)。
    gl_Position = vec4(positions[gl_VertexIndex], 1.0);

    // 取出同一编号顶点的颜色。光栅化阶段会在三角形内部自动插值该颜色。
    fragColour = colours[gl_VertexIndex];
}
