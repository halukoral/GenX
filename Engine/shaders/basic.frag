#version 450

layout(binding = 1) uniform sampler2D texSampler;

layout(location = 0) in vec3 fragColor;
layout(location = 1) in vec3 fragNormal;
layout(location = 2) in vec2 fragTexCoord;
layout(location = 3) in vec3 fragWorldPos;

layout(location = 0) out vec4 outColor;

void main() {
    // Texture'dan renk al
    vec4 texColor = texture(texSampler, fragTexCoord);

    // Basit ışıklandırma
    vec3 lightPos = vec3(2.0, 2.0, 2.0);
    vec3 lightColor = vec3(1.0, 1.0, 1.0);

    // Ambient
    vec3 ambient = 0.15 * lightColor;

    // Diffuse
    vec3 norm = normalize(fragNormal);
    vec3 lightDir = normalize(lightPos - fragWorldPos);
    float diff = max(dot(norm, lightDir), 0.0);
    vec3 diffuse = diff * lightColor;

    // Final color = texture * lighting
    vec3 result = (ambient + diffuse) * texColor.rgb;
    outColor = vec4(result, texColor.a);

    // Debug: Texture coordinate'leri görmek için (geçici)
    // outColor = vec4(fragTexCoord, 0.0, 1.0);
}