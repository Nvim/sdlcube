#include "camera.h"

// TODO: hardcoded projection form origin for now
Camera::Camera(float fov, float aspect, float near, float far)
    : fov_{fov}, aspect_{aspect}, near_{near}, far_{far} {

  const float tanHalfFovy = tan(fov_ / 2.f);
  matrix_ = glm::mat4{0.0f};
  matrix_[0][0] = 1.f / (aspect * tanHalfFovy);
  matrix_[1][1] = 1.f / (tanHalfFovy);
  matrix_[2][2] = far / (far - near);
  matrix_[2][3] = 1.f;
  matrix_[3][2] = -(far * near) / (far - near);
}

glm::mat4 &Camera::Matrix() { return matrix_; }
