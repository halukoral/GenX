#version 450

layout(location = 0) in vec3 fragColor;
layout(location = 1) in vec2 fragTexCoord;

layout(location = 0) out vec4 outColor;

void main() {
    // Simple gradient based on texture coordinates for now
    vec3 color = mix(fragColor, vec3(fragTexCoord, 0.5), 0.5);
    outColor = vec4(color, 1.0);
}