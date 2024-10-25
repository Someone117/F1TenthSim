#include "Camera.h"
#include <glm/fwd.hpp>

namespace Infinite {
Camera cameras = Camera(glm::vec3(10, 9, -0.5));

void Camera::move(float amt, MoveDirection direction) {
  glm::vec3 forward =
      glm::vec3(sin(xAngle), cos(xAngle), 0.0f) * glm::vec3(1.0f, 1.0f, 0.0f);
  forward.z = 0;
  glm::vec3 left =
      glm::normalize(glm::cross(forward, glm::vec3(0, 0, 1))) * amt;
  forward = glm::normalize(forward) * amt;

  if (direction == FORWARD)
    position -= forward;
  else if (direction == BACKWARD)
    position += forward;
  else if (direction == LEFT)
    position += left;
  else if (direction == RIGHT)
    position -= left;
  else if (direction == UP)
    position -= Up * amt;
  else if (direction == DOWN)
    position += Up * amt;
}

void Camera::mouse(float xpos, float ypos) {
  xAngle += xpos;
  // make sure that when pitch is out of bounds, screen doesn't get flipped
  yAngle = glm::clamp(yAngle + ypos, (float)-M_PI, 0.0f);
}

glm::mat4 Camera::updateCameraVectors() {
  // FPS camera:  RotationX(pitch) * RotationY(yaw)
  auto qPitch = glm::angleAxis(yAngle, glm::vec3(1, 0, 0));
  auto qYaw = glm::angleAxis(xAngle, glm::vec3(0, 0, 1));
  angles = qPitch * qYaw;
  angles = glm::normalize(angles);
  return glm::mat4_cast(angles);
}

glm::mat4 Camera::getViewMatrix() {
  return glm::translate(updateCameraVectors(), position);
}

void Camera::translate(glm::vec3 distance) { position += distance; }
bool Camera::getIsOrthographic() const { return isOrthographic; }
void Camera::setIsOrthographic(bool isOrthographic_) {
  isOrthographic = isOrthographic_;
}

} // namespace Infinite
