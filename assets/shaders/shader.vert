#version 450

layout(binding = 0) uniform UniformBufferObject {
    mat4 model;
    mat4 view;
    mat4 proj;
} ubo;

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec2 inTexCoord;

// layout(location = 2) in vec4 inColor;

layout(location = 0) out vec2 fragTexCoord;
// layout(location = 1) out vec3 fragColor;


void main() {
    gl_Position = ubo.proj * ubo.view * ubo.model * vec4(inPosition, 1.0);
    // gl_PointSize = 14.0;
    fragTexCoord = inTexCoord;
    // fragColor = inColor.rgb;
}