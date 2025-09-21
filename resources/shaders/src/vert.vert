#version 450 core

layout(location = 0) in vec3 Pos;
layout(location = 1) in vec2 inUv;
layout(location = 0) out vec2 uv;

layout(std140, binding = 0, set = 1) uniform uMatrices {
    mat4 mat_vp;
    mat4 mat_m;
} mvp;

layout(std140, binding = 1, set = 1) uniform uCameraMatrix {
    mat4 mat_cam;
};

layout(std140, binding = 2, set = 1) uniform uInstanceSettings {
    float spread;
    uint dimension;
};

void main()
{
    uv = inUv;
    uint instance = gl_InstanceIndex;
    uint square = dimension * dimension;

    vec4 relative_pos = mvp.mat_m * vec4(Pos, 1.0);
    relative_pos.x += float(instance % dimension) * spread;
    relative_pos.y += int(floor(float(instance / dimension))) % dimension * spread;
    relative_pos.z += floor(float(instance / square)) * spread;

    relative_pos.x -= dimension * 2;
    relative_pos.y -= dimension * 2;
    relative_pos.z -= dimension * 2;
    gl_Position = mvp.mat_vp * relative_pos;
}
