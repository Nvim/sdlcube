#version 450 core

layout(location = 0) in vec3 Pos;
layout(location = 0) out vec3 uv;

layout(std140, binding=0, set=1) uniform uMatrices {
  mat4 mat_vp;
  mat4 mat_m;
  mat4 mat_cam;
} mvp;

layout(std140, binding=1, set=1) uniform uCameraMatrix {
  mat4 mat_cam;
};

void main()
{
  uv = Pos;
	vec4 res = mvp.mat_vp * mat_cam * vec4(Pos, 1.0).xyzw;
  gl_Position = res.xyww;
}
