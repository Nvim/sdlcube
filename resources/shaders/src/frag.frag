#version 450 core

layout (location = 0 ) in vec3 col;
layout (location = 0 ) out vec4 FragColor;

void main()
{
  FragColor = vec4(col, 1.0f);
}
