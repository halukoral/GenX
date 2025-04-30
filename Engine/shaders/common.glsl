#extension GL_KHR_vulkan_glsl: enable

layout(set = 0, binding = 0) uniform UniformBufferObject 
{
    mat4 model;
    mat4 view;
    mat4 projection;
} camera;