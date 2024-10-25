#include "Infinite/Infinite.h"
#include "Infinite/backend/Model/Models/Model.h"
#include "Infinite/backend/Rendering/RenderPasses/BasicRenderPass.h"
#include "Infinite/frontend/Camera.h"
#include "Infinite/frontend/Car.h"
#include "Infinite/util/constants.h"
#include <GLFW/glfw3.h>
#include <climits>
#include <cstdint>
#include <glm/fwd.hpp>
#include <iostream>
#include <ostream>


#include "Infinite/backend/Software/alltogether.h"


using namespace Infinite;
const float torque = 100.0f;         // Torque applied to the wheels (Nm)
const float deltaTime = 1.0f / 60.0f; // Time step for each update (seconds)
const float brakeTorqueScale = -100.0f;
const float torqueScale = 1.0f;
float speed = 0.0f;
float steeringAngle = 0.0f; // Steering angle in degrees

template <typename T> int sgn(T val) { return (T(0) < val) - (val < T(0)); }

Car car(0.0f, glm::radians(0.0f), 0.4f, 0.1f, 2.0f, 0.1f);

const char *const MODEL_PATH = R"(../assets/track.obj)";
// const char *const MODEL_PATH2 = R"(../assets/untitled.obj)";

const char *const TEXTURE_PATH = R"(../assets/track.png)";
// const char *const TEXTURE_PATH2 = R"(../assets/image.jpg)";

void mainLoop() {

  bool lastCursor = false;
  bool capture_cursor = true;
  // glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

  double xpos = 0.0, ypos = 0.0;
  double currentTime;
  double lastTime = 0.0;
  float spf;
  int f = 0;
  double frameTimes = 0;
  bool firstFrame = true;

#define SENSITIVITY 0.4f

  if (glfwRawMouseMotionSupported()) { // make a setting
    glfwSetInputMode(window, GLFW_RAW_MOUSE_MOTION, GLFW_TRUE);
    std::cout << "Raw input supported, using it" << std::endl;
  }
  glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

  uint32_t screenshotNum = 0;
  while (!glfwWindowShouldClose(window)) {
    currentTime = glfwGetTime();
    spf = currentTime - lastTime;
    lastTime = currentTime;
    frameTimes += spf;
    f++;
    if (f == 1000) {
      printf("%f\n", frameTimes);
      f = 0;
      frameTimes = 0;
    }

    glfwPollEvents();

    // tab out of game
    if (glfwGetKey(window, GLFW_KEY_TAB)) {
      if (!lastCursor) {
        std::cout << "tab" << std::endl;
        capture_cursor = !capture_cursor;
        if (capture_cursor) {
          glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
        } else {
          glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
        }
      }
      lastCursor = true;
    } else {
      lastCursor = false;
    }
    // if not tabbed out, get the cursor and reset pos, else don't move mouse
    if (capture_cursor) {
      if (glfwGetKey(window, GLFW_KEY_ESCAPE)) {
        glfwSetWindowShouldClose(window, true);
      }

      // screenshot
      if (glfwGetKey(window, GLFW_KEY_F2)) {
        std::string name = "screenshot";
        name.append(std::to_string(screenshotNum));
        name.append(".png");
        saveScreenshot(name.c_str(), Infinite::PNG);
      }

      glfwGetCursorPos(window, &xpos,
                       &ypos); // TODO: cursor snaps player when tabbing in
      glfwSetCursorPos(window, swapChainExtent.width / 2.0,
                       swapChainExtent.height / 2.0);

      // // looking
      // if (!firstFrame) {
      //   cameras.mouse(
      //       (std::floor(xpos - swapChainExtent.width / 2.0f) * spf) *
      //           SENSITIVITY,
      //       (std::floor((ypos - swapChainExtent.height / 2.0f)) * spf) *
      //           SENSITIVITY);
      // } else {
      //   firstFrame = false;
      // }
      // // movement
      // if (glfwGetKey(window, GLFW_KEY_W)) {
      //   cameras.move(spf, FORWARD);
      // }
      // if (glfwGetKey(window, GLFW_KEY_S)) {
      //   cameras.move(spf, BACKWARD);
      // }
      // if (glfwGetKey(window, GLFW_KEY_A)) {
      //   cameras.move(spf, LEFT);
      // }
      // if (glfwGetKey(window, GLFW_KEY_D)) {
      //   cameras.move(spf, RIGHT);
      // }
      // if (glfwGetKey(window, GLFW_KEY_SPACE)) {
      //   cameras.move(spf, UP);
      // }
      // if (glfwGetKey(window, GLFW_KEY_LEFT_SHIFT)) {
      //   cameras.move(spf, DOWN);
      // }

      // movement
      speed = 0.0f;
      steeringAngle = 0.0f;
      if (glfwGetKey(window, GLFW_KEY_W)) {
        speed = 1.0f;
      }
      if (glfwGetKey(window, GLFW_KEY_S)) {
        speed = -1.0f;
      }
      if (glfwGetKey(window, GLFW_KEY_A)) {
        steeringAngle = -1.0f;
      }
      if (glfwGetKey(window, GLFW_KEY_D)) {
        steeringAngle = 1.0f;
      }

      float inverseVelocity = std::abs(car.velocity);
      float brakeTorque =
          std::abs(speed) < 0.01f
              ? sgn(car.velocity) * inverseVelocity * brakeTorqueScale
              : speed * torque;

      cameras.move(car.update(steeringAngle * 20.0, brakeTorque, spf / 100.0f),
                   Infinite::FORWARD);
      cameras.setAngles(car.heading + M_PI / 2.0, -M_PI / 2.0);
      
      if(glfwGetKey(window, GLFW_KEY_F)) {
        car.velocity = 0;
      }

      // std::cout << car.velocity << std::endl;
    }
    renderFrame();
  }
  waitForNextFrame();
}

int main() {
  App vulkanTest("Racecar Sim 2", 0, 1, 0);

  BasicRenderPass mainPass{};

  addRenderPass(&mainPass);

  initInfinite(vulkanTest);

  Model mainModel = createModel("main", MODEL_PATH, TEXTURE_PATH);
  mainModel.setScale(glm::vec3(10.0f, 10.0f, 10.0f));

  mainPass.addModel(&mainModel);

  cameras.setAngles(M_PI / 2.0, -M_PI / 2.0);

  init(mainModel);

  castRays(cameras.getPosition());

  try {
    mainLoop();
    cleanUp();
  } catch (const std::exception &e) {
    std::cerr << e.what() << std::endl;
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}