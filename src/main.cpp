#include "Infinite/Infinite.h"
#include "Infinite/backend/Model/Models/Model.h"
#include "Infinite/backend/Rendering/RenderPasses/BasicRenderPass.h"
#include "Infinite/backend/Rendering/RenderPasses/ComputeRenderPass.h"
#include "Infinite/frontend/Camera.h"
#include "Infinite/util/constants.h"

#include <GLFW/glfw3.h>
#include <climits>
#include <cstdint>
#include <glm/fwd.hpp>
#include <iostream>
#include <ostream>
#include <string>

using namespace Infinite;

const char *const MODEL_PATH = R"(../assets/viking_room.obj)";
const char *const MODEL_PATH2 = R"(../assets/untitled.obj)";

const char *const TEXTURE_PATH = R"(../assets/viking_room.png)";
const char *const TEXTURE_PATH2 = R"(../assets/image.jpg)";

Camera camera;

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
      // looking
      camera.mouse((std::floor(xpos - swapChainExtent.width / 2.0f) * spf) *
                       SENSITIVITY,
                   (std::floor((ypos - swapChainExtent.height / 2.0f)) * spf) *
                       SENSITIVITY);

      // movement
      if (glfwGetKey(window, GLFW_KEY_W)) {
        camera.move(spf, FORWARD);
      }
      if (glfwGetKey(window, GLFW_KEY_S)) {
        camera.move(spf, BACKWARD);
      }
      if (glfwGetKey(window, GLFW_KEY_A)) {
        camera.move(spf, LEFT);
      }
      if (glfwGetKey(window, GLFW_KEY_D)) {
        camera.move(spf, RIGHT);
      }
      if (glfwGetKey(window, GLFW_KEY_SPACE)) {
        camera.move(spf, UP);
      }
      if (glfwGetKey(window, GLFW_KEY_LEFT_SHIFT)) {
        camera.move(spf, DOWN);
      }
    }
    renderFrame();
  }
  waitForNextFrame();
}

int main() {
  App vulkanTest("Vulkan Test", 0, 1, 0);

  // ComputeRenderPass computePass{};

  // addRenderPass(&computePass);

  BasicRenderPass mainPass{};

  addRenderPass(&mainPass);

  initInfinite(vulkanTest);

  Model mainModel = createModel("main", MODEL_PATH, TEXTURE_PATH);

  mainPass.addModel(&mainModel);
  // mainPass.addModel(computePass.getModels().front());

  camera = Camera(glm::vec3(0, 0, -0.5));
  camera.setAngles(-27.2313, -1.3192);
  cameras.push_back(&camera);

  try {
    mainLoop();
    cleanUp();
  } catch (const std::exception &e) {
    std::cerr << e.what() << std::endl;
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}