#version 450 core

layout (location = 0 ) out vec4 FragColor;
layout (location = 0 ) in vec3 col;
void main()
{
  // FragColor = vec4(0.2f, 0.4f, 0.4f, 1.0f);
  FragColor = vec4(col, 1.0f);
}
