#ifndef STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_IMPLEMENTATION
#endif

#ifndef GLFW_INCLUDE_VULKAN
#define GLFW_INCLUDE_VULKAN
#endif

#ifndef VMA_IMPLEMENTATION
#define VMA_IMPLEMENTATION
#endif

#ifndef GLM_FORCE_RADIANS
#define GLM_FORCE_RADIANS
#endif

#ifndef GLM_FORCE_DEPTH_ZERO_TO_ONE
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#endif

#ifndef GLM_ENABLE_EXPERIMENTAL
#define GLM_ENABLE_EXPERIMENTAL
#endif

#define MAX_MODELS 1

#include "Infinite/Infinite.h"
#include "Infinite/backend/Model/Models/Model.h"
#include "Infinite/backend/Rendering/Engine.h"
#include "Infinite/backend/Rendering/RenderPasses/BasicRenderPass.h"

// #include
// <BulletCollision/CollisionDispatch/btDefaultCollisionConfiguration.h>

/*
Ended before:
Compute Shaders
*/

using namespace Infinite;

const char *const MODEL_PATH = R"(../assets/viking_room.obj)";
const char *const TEXTURE_PATH = R"(../assets/viking_room.png)";

Camera camera;

void mainLoop() {

  bool lastCursor = false;
  bool capture_cursor = true;
  glfwSetInputMode(Engine::getEngine().getWindow(), GLFW_CURSOR,
                   GLFW_CURSOR_DISABLED);

  double xpos = 0.0, ypos = 0.0;
  double currentTime;
  double lastTime = 0.0;
  float spf;
  int f = 0;
  double frameTimes = 0;

  if (glfwRawMouseMotionSupported()) // make a setting
    glfwSetInputMode(Engine::getEngine().getWindow(), GLFW_RAW_MOUSE_MOTION,
                     GLFW_TRUE);

  while (!glfwWindowShouldClose(Engine::getEngine().getWindow())) {
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
    // do we exit
    if (glfwGetKey(Engine::getEngine().getWindow(), GLFW_KEY_ESCAPE)) {
      glfwSetWindowShouldClose(Engine::getEngine().getWindow(), true);
    }
    // tab out of game
    if (glfwGetKey(Engine::getEngine().getWindow(), GLFW_KEY_TAB)) {
      if (!lastCursor) {
        capture_cursor = !capture_cursor;
        if (capture_cursor) {
          glfwSetInputMode(Engine::getEngine().getWindow(), GLFW_CURSOR,
                           GLFW_CURSOR_DISABLED);
        } else {
          glfwSetInputMode(Engine::getEngine().getWindow(), GLFW_CURSOR,
                           GLFW_CURSOR_NORMAL);
        }
      }
      lastCursor = true;
    } else {
      lastCursor = false;
    }
    // if not tabbed out, get the cursor and reset pos, else don't move mouse
    if (capture_cursor) {
      glfwGetCursorPos(Engine::getEngine().getWindow(), &xpos, &ypos);
      glfwSetCursorPos(Engine::getEngine().getWindow(),
                       Engine::getEngine().getWindowWidth() / 2.0,
                       Engine::getEngine().getWindowHeight() / 2.0);
      // looking
      camera.mouse(
          (std::floor(xpos - Engine::getEngine().getWindowWidth() / 2.0f) *
           spf),
          (std::floor((ypos - Engine::getEngine().getWindowHeight() / 2.0f)) *
           spf));

      // movement
      if (glfwGetKey(Engine::getEngine().getWindow(), GLFW_KEY_W)) {
        camera.move(spf, FORWARD);
      }
      if (glfwGetKey(Engine::getEngine().getWindow(), GLFW_KEY_S)) {
        camera.move(spf, BACKWARD);
      }
      if (glfwGetKey(Engine::getEngine().getWindow(), GLFW_KEY_A)) {
        camera.move(spf, LEFT);
      }
      if (glfwGetKey(Engine::getEngine().getWindow(), GLFW_KEY_D)) {
        camera.move(spf, RIGHT);
      }
      if (glfwGetKey(Engine::getEngine().getWindow(), GLFW_KEY_SPACE)) {
        camera.move(spf, UP);
      }
      if (glfwGetKey(Engine::getEngine().getWindow(), GLFW_KEY_LEFT_SHIFT)) {
        camera.move(spf, DOWN);
      }
    } else {
      xpos = Engine::getEngine().getWindowWidth() / 2.0;
      ypos = Engine::getEngine().getWindowHeight() / 2.0;
    }
    renderFrame();
  }
  waitForNextFrame();
}

int main() {
  //    Engine::getEngine().settings.mipLevels =
  //    static_cast<uint32_t>(std::floor(std::log2(std::max(texWidth,
  //    texHeight)))) + 1;
  App vulkanTest("Vulkan Test", 0, 1, 0);

  BasicRenderPass mainPass{};

  addRenderPass(&mainPass);

  Engine::getEngine().setApp(vulkanTest);

  initInfinite();

  Model mainModel = createModel("main", MODEL_PATH, TEXTURE_PATH);

  mainPass.addModel(&mainModel);

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