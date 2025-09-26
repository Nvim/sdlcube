#version 450 core

layout(location = 0) out vec4 FragColor;
// layout(location = 0) in vec2 uv;

layout(set = 2, binding = 0) uniform sampler2D tex;

void main()
{
    // FragColor = texture(tex, uv);
    FragColor = vec4(0.1, 0.5, 0.6, 1.0);
}
