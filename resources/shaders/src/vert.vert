#version 450 core

layout(location = 0) in vec3 Pos;
layout(location = 1) in vec3 inCol;
layout(location = 0) out vec3 col;

layout(std140, binding=0, set=3) uniform uTransform {
  // mat4 mat_vp;
  // mat4 mat_m;
  mat4 tr;
} transform;

void main()
{
  col = inCol;

  // const vec4 world_pos = mat_m * vec4(Pos, 1.0);
  const vec4 world_pos = vec4(Pos, 1.0);
	gl_Position = transform.tr * world_pos;
}
