@echo off
REM 将人类可读的 GLSL 顶点着色器编译成 Vulkan 使用的 SPIR-V 二进制。
C:/VulkanSDK/1.4.357.0/Bin/glslangValidator.exe -V shader.vert -o shader.vert.spv
REM 同样编译片段着色器；修改 GLSL 后需要重新执行本脚本生成新的 .spv。
C:/VulkanSDK/1.4.357.0/Bin/glslangValidator.exe -V shader.frag -o shader.frag.spv
pause
