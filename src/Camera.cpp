//
// Created by Someo on 3/16/2023.
//

#include "Camera.h"

Camera Camera::rotate(float amt, glm::vec3 axis) {
    _rotation = glm::rotate(_rotation, amt, axis);
    return *this;
};
Camera Camera::translate(glm::vec3 translate) {
    _position+=translate;
    return *this;
}

Camera::Camera(glm::vec3 position, glm::vec3 look) : _position(position), _rotation(look) {}

glm::vec3 &Camera::getPosition() {
    return _position;
}

void Camera::setPosition(const glm::vec3 &position) {
    _position = position;
}

glm::vec3 &Camera::getRotation() {
    return _rotation;
}

void Camera::setRotation(const glm::vec3 &rotation) {
    _rotation = rotation;
}

Camera::Camera() : _position(0.0f), _rotation(0.0f) {}

void Camera::move(float amt, MoveDirection dir) {
    switch(dir) {
        case FORWARD: {
            _position+= glm::normalize(glm::vec3(_rotation.x, _rotation.y, 0.0f)) * amt;
            break;
        }
        case BACKWARD: {
            _position-= glm::normalize(glm::vec3(_rotation.x, _rotation.y, 0.0f)) * amt;
            break;
        }
        case LEFT: {
            _position-=glm::cross( glm::normalize(glm::vec3(_rotation.x, _rotation.y, 0.0f))* amt, up);
            break;
        }
        case RIGHT: {
            _position+=glm::cross( glm::normalize(glm::vec3(_rotation.x, _rotation.y, 0.0f))* amt, up);
            break;
        }
        case UP: {
            _position.z+=amt;
            break;
        }
        case DOWN: {
            _position.z-=amt;
            break;
        }
    }

}
