#version 450

vec2 positions[3] = vec2[](
    vec2(-0.5, 0.5),
    vec2(0.5, 0.5),
    vec2(0.0, -0.5)
);


vec3 colors[3] = vec3[](
    vec3(1.0, 0.0, 0.0),
    vec3(0.0, 1.0, 0.0),
    vec3(0.0, 0.0, 1.0)
);

layout(set = 0, binding = 0) uniform CameraUniformBufferObject {
    mat4 viewMatrix;
    mat4 projectionMatrix;
    vec3 cameraPosition;
    float time;
} cameraUBO;

layout(location = 0) out vec3 fragColor;

void main()
{
    vec4 worldPos = vec4(positions[gl_VertexIndex], 0.0, 1.0);

    vec4 clipPos = cameraUBO.projectionMatrix * cameraUBO.viewMatrix * worldPos;

    gl_Position = clipPos;

    vec3 cameraDir = normalize(cameraUBO.cameraPosition - vec3(worldPos.xy, 0.0));
    fragColor = colors[gl_VertexIndex] * (0.5 + 0.5 * sin(cameraUBO.time + dot(cameraDir, vec3(0.0, 0.0, 1.0))));
}