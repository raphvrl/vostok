#version 450

layout(location = 0) in vec2 inPosition;
layout(location = 1) in vec3 inColor;

layout(set = 0, binding = 0) uniform CameraUniformBufferObject {
    mat4 viewMatrix;
    mat4 projectionMatrix;
    vec3 cameraPosition;
    float time;
} cameraUBO;

layout(location = 0) out vec3 fragColor;

void main()
{
    vec4 worldPos = vec4(inPosition, 0.0, 1.0);

    vec4 clipPos = cameraUBO.projectionMatrix * cameraUBO.viewMatrix * worldPos;

    gl_Position = clipPos;

    vec3 cameraDir = normalize(cameraUBO.cameraPosition - vec3(worldPos.xy, 0.0));
    fragColor = inColor * (0.5 + 0.5 * sin(cameraUBO.time + dot(cameraDir, vec3(0.0, 0.0, 1.0))));
}