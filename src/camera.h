#pragma once

#include <glm/glm.hpp>

class Camera {
public:
  Camera(float fov, float aspect, float near, float far);
  glm::mat4 &Matrix();

private:
  glm::mat4 matrix_;
  float fov_;
  float aspect_;
  float near_;
  float far_;
};
