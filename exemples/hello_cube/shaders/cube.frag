#version 450

layout(location = 0) out vec4 outColor;

layout(location = 0) in vec2 fragUV;

layout(set = 0, binding = 2) uniform sampler2D textureSampler[];

void main()
{
    if (fragUV.x < 0.0 || fragUV.x > 1.0 || fragUV.y < 0.0 || fragUV.y > 1.0) {
        outColor = vec4(0.4, 0.4, 0.4, 1.0);
    } else {
        outColor = texture(textureSampler[0], fragUV);
    }
}
