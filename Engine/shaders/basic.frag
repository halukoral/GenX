#version 450

layout(location = 0) in vec3 fragColor;
layout(location = 1) in vec3 fragNormal;
layout(location = 2) in vec2 fragTexCoord;
layout(location = 3) in vec3 fragWorldPos;

layout(location = 0) out vec4 outColor;

void main() {
    // Basit directional lighting
    vec3 lightDir = normalize(vec3(1.0, 1.0, 1.0));
    vec3 normal = normalize(fragNormal);

    // Ambient lighting
    vec3 ambient = 0.3 * fragColor;

    // Diffuse lighting
    float diff = max(dot(normal, lightDir), 0.0);
    vec3 diffuse = diff * fragColor;

    // Simple specular
    vec3 viewDir = normalize(-fragWorldPos);
    vec3 reflectDir = reflect(-lightDir, normal);
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), 32);
    vec3 specular = spec * vec3(0.5, 0.5, 0.5);

    vec3 result = ambient + diffuse + specular;
    outColor = vec4(result, 1.0);
}