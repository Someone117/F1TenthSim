#ifndef VULKAN_CAMERA_H
#define VULKAN_CAMERA_H

#include <glm/fwd.hpp>
#include <vector>
#pragma once

#include <cmath>
#include <glm/ext/matrix_clip_space.hpp>
#include <glm/ext/matrix_transform.hpp>
#include <glm/ext/quaternion_trigonometric.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtx/rotate_vector.hpp>

namespace Infinite {
enum MoveDirection { FORWARD, BACKWARD, LEFT, RIGHT, UP, DOWN };
const glm::vec3 worldUp(0.0f, 0.0f, 1.0f);
class Camera {
public:
  explicit Camera(glm::vec3 pos = glm::vec3(-1.0f, 0.9f, -0.05f),
                  glm::vec3 up = worldUp, float yaw = M_PI, float pitch = 0.0f,
                  bool _isOrthographic = false)
      : position(pos), Up(up), xAngle(yaw), yAngle(pitch),
        isOrthographic(_isOrthographic) {
    updateCameraVectors();
  }

  // returns the view matrix calculated using Euler Angles and the LookAt Matrix
  glm::mat4 getViewMatrix();

  // processes input received from any keyboard-like input system. Accepts input
  // parameter in the form of camera defined ENUM (to abstract it from windowing
  // systems)
  void move(float amt, MoveDirection direction);

  // processes input received from a mouse input system. Expects the offset
  // value in both the x and y direction.
  void mouse(float xpos, float ypos);

  void translate(glm::vec3 distance);

  bool getIsOrthographic() const;
  void setIsOrthographic(bool isOrthographic_);
  glm::vec3 getPosition() { return position; }
  glm::vec3 getUp() { return Up; }
  glm::vec2 getAngles() { return glm::vec2(xAngle, yAngle); }
  void setUp(glm::vec3 up) { Up = up; }
  void setAngles(float xAngle_, float yAngle_) { 
    xAngle = xAngle_;
    yAngle = yAngle_;
  };
  inline void setPositon(glm::vec3 pos) {
    position = pos;
  }

private:
  // camera Attributes
  glm::vec3 position;
  glm::vec3 Up;
  // euler Angles
  float xAngle;
  float yAngle;
  glm::quat angles;

  bool isOrthographic;

  // calculates the front vector from the Camera's (updated) Euler Angles
  glm::mat4 updateCameraVectors();
};

extern Camera cameras;
} // namespace Infinite
#endif // VULKAN_CAMERA_H
