#version 450

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec2 inUV;

layout(set = 0, binding = 0) uniform CameraUniformBufferObject {
    mat4 viewMatrix;
    mat4 projectionMatrix;
    vec3 cameraPosition;
    float time;
} cameraUBO;

layout(location = 0) out vec2 fragUV;

void main()
{
    vec4 worldPos = vec4(inPosition, 1.0);

    vec4 clipPos = cameraUBO.projectionMatrix * cameraUBO.viewMatrix * worldPos;

    gl_Position = clipPos;

    fragUV = inUV;
}