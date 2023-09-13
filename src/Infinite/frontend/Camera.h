
#ifndef VULKAN_CAMERA_H
#define VULKAN_CAMERA_H

#pragma once
#include <glm/glm.hpp>
#include <glm/ext/matrix_transform.hpp>
#include <glm/ext/matrix_clip_space.hpp>
#include <glm/gtx/rotate_vector.hpp>
#include <glm/ext/quaternion_trigonometric.hpp>
#include <glm/gtc/quaternion.hpp>
#include <cmath>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

namespace Infinite {
    enum MoveDirection {
        FORWARD, BACKWARD, LEFT, RIGHT, UP, DOWN
    };
    const glm::vec3 worldUp(0.0f, 0.0f, 1.0f);

    class Camera {
    public:
        explicit Camera(glm::vec3 pos = glm::vec3(0.0f, 0.0f, -1.0f), glm::vec3 up = worldUp, float yaw = M_PI,
                        float pitch = 0.0f) : position(pos), Up(up), xAngle(yaw), yAngle(pitch) {
            updateCameraVectors();
        }

        // returns the view matrix calculated using Euler Angles and the LookAt Matrix
        glm::mat4 getViewMatrix();

        // processes input received from any keyboard-like input system. Accepts input parameter in the form of camera defined ENUM (to abstract it from windowing systems)
        void move(float amt, MoveDirection direction);

        // processes input received from a mouse input system. Expects the offset value in both the x and y direction.
        void mouse(float xpos, float ypos);

        void translate(glm::vec3 distance);

    private:
        // camera Attributes
        glm::vec3 position;
        glm::vec3 Up;
        // euler Angles
        float xAngle;
        float yAngle;
        glm::quat angles;

        // calculates the front vector from the Camera's (updated) Euler Angles
        glm::mat4 updateCameraVectors();
    };
}
#endif //VULKAN_CAMERA_H
