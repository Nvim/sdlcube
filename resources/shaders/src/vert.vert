#version 450 core

layout(location = 0) in vec3 Pos;

layout(std140, binding=0, set=3) uniform uTransform {
  mat4 tr;
};

void main()
{
  gl_Position = vec4(Pos, 1.0) * tr;
}
