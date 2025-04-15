#version 450
#include "common.glsl"

layout(location = 0) in vec3 input_position;
layout(location = 1) in vec3 input_color;

layout(location = 0) out vec4 vertex_color;

void main() 
{
    gl_Position = vec4(input_position, 1.0);
    vertex_color = vec4(input_color, 1.0);
}