#include <algorithm>
#include <cmath>
#include <glm/common.hpp>
#include <glm/ext/vector_float2.hpp>
#include <glm/trigonometric.hpp>
#include <iostream>

// Car class representing a simple 4-wheel drive car with torque and
// acceleration
class Car {
public:
  float position;
  float heading;  // Heading angle (orientation) in radians
  float velocity; // Linear velocity (m/s)
  float
      wheelbase; // Wheelbase of the car (distance between front and rear axles)
  float wheelRadius;  // Radius of the wheels
  float mass;         // Mass of the car (kg)
  float acceleration; // Linear acceleration (m/s^2)
  float angularVelocity;
  float carWidth;

  Car(float initX, float initHeading, float wheelbaseLength, float wheelR,
      float carMass, float width)
      : position(initX), heading(initHeading), velocity(0.0f),
        wheelbase(wheelbaseLength), wheelRadius(wheelR), mass(carMass),
        acceleration(0.0f), carWidth(width) {}

  // Update car's position based on torque, steering angle, and time step
  inline float update(float steeringAngleDegrees, float torque,
                      float deltaTime) {
    // Convert steering angle to radians
    steeringAngleDegrees = std::clamp(steeringAngleDegrees, -20.0f, 20.0f);
    float steeringAngle = glm::radians(steeringAngleDegrees);

    // Calculate the force exerted by the torque on the wheels (force = torque /
    // wheel radius)
    float force = torque / wheelRadius;

    // Calculate acceleration based on Newton's second law: a = F / m
    acceleration = force / mass;

    // Update velocity: v_new = v_old + a * deltaTime
    velocity += acceleration * deltaTime;

    // 0.4 is a really good max
    velocity = std::clamp(velocity, -0.15f, 0.15f);
    if (std::abs(velocity) < 0.01) {
      velocity = 0.0f;
    }
    // Calculate turning radius
    float turningRadius;
    if (std::abs(steeringAngle) > 0.0001f) {
      turningRadius = wheelbase / std::tan(steeringAngle);
    } else {
      turningRadius = INFINITY; // No turning if steering is zero
    }

    // Calculate angular velocity (if turning)
    float angularVelocity =
        (turningRadius == INFINITY) ? 0.0f : (100 / turningRadius);

    if (velocity == 0) {
      angularVelocity = 0;
    }
    // Update car's heading (orientation)
    heading += angularVelocity * deltaTime;

    // Update car's position
    position += velocity * deltaTime;
    // position.y += velocity * std::sin(heading) * deltaTime;
    return position;
  }

  // Print current position, velocity, and heading of the car
  inline void printState() const {
    std::cout << "Position: " << position << ", Velocity: " << velocity
              << " m/s, Acceleration: " << acceleration
              << " m/s^2, Heading: " << heading * 180.0f / M_PI << " degrees\n";
  }
};
