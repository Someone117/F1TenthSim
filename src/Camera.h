//
// Created by Someo on 3/16/2023.
//

#ifndef VULKAN_CAMERA_H
#define VULKAN_CAMERA_H

#pragma once
#include <glm/glm.hpp>
#include <glm/ext/matrix_transform.hpp>
#include <glm/ext/matrix_clip_space.hpp>
#include <glm/gtx/rotate_vector.hpp>
enum MoveDirection {FORWARD,BACKWARD, LEFT, RIGHT, UP, DOWN};
static glm::vec3 up(0.0f, 0.0f, 1.0f);

class Camera {
private:

    glm::vec3 _position;
    glm::vec3 _rotation;
public:
    Camera();
    Camera(glm::vec3 position, glm::vec3 look);
    void move(float amt, MoveDirection dir);
    Camera rotate(float amt, glm::vec3 axis);
    Camera translate(glm::vec3 translate);

    ~Camera() = default;

    glm::vec3 &getPosition();

    void setPosition(const glm::vec3 &position);

    glm::vec3 &getRotation();

    void setRotation(const glm::vec3 &rotation);

};


#endif //VULKAN_CAMERA_H
