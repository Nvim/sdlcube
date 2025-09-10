#include "transform.h"
#include <glm/ext/matrix_transform.hpp>
#include <glm/gtc/matrix_transform.hpp>

// glm::mat4 &Transform::Matrix() {
//   matrix_ = glm::translate(glm::mat4{1.f}, translation_);
//   matrix_ = glm::rotate(matrix_, rotation_.y, {0.f, 1.f, 0.f});
//   matrix_ = glm::rotate(matrix_, rotation_.x, {1.f, 0.f, 0.f});
//   matrix_ = glm::rotate(matrix_, rotation_.z, {0.f, 0.f, 1.f});
//   matrix_ = glm::scale(matrix_, scale_);
//
//   return matrix_;
// }
glm::mat4 Transform::Matrix() {
  auto matrix = glm::translate(glm::mat4{1.f}, translation_);
  matrix = glm::rotate(matrix, rotation_.y, {0.f, 1.f, 0.f});
  matrix = glm::rotate(matrix, rotation_.x, {1.f, 0.f, 0.f});
  matrix = glm::rotate(matrix, rotation_.z, {0.f, 0.f, 1.f});
  matrix = glm::scale(matrix, scale_);

  return matrix;
}
