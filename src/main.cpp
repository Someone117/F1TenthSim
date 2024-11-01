#include "Infinite/Infinite.h"
#include "Infinite/backend/Model/Models/Model.h"
#include "Infinite/backend/Rendering/RenderPasses/BasicRenderPass.h"
#include "Infinite/backend/Settings.h"
#include "Infinite/frontend/Camera.h"
#include "Infinite/frontend/Car.h"
#include "Infinite/util/constants.h"
#include <GLFW/glfw3.h>
#include <atomic>
#include <climits>
#include <cstdint>
#include <glm/fwd.hpp>
#include <iostream>
#include <ostream>
#include <vector>

#include "Infinite/backend/Software/BVH.h"
#include "stlastar.h"

// This code currently does not work, but the idea is to eventually get it to
// work, currently it is much faster than the python version without much
// optimization. Also this is probably the slowest it will get A* wise

using namespace Infinite;
const float torque = 0.010f;          // Torque applied to the wheels (Nm)
const float deltaTime = 1.0f / 60.0f; // Time step for each update (seconds)
const float brakeTorqueScale = -0.2f;
float speed = 0.0f;
float steeringAngle = 0.0f; // Steering angle in degrees

template <typename T> int sgn(T val) { return (T(0) < val) - (val < T(0)); }

Car car(0.0f, glm::radians(0.0f), 0.4f, 0.1f, 2.0f, 0.1f);

const char *const MODEL_PATH = R"(../assets/track.obj)";
// const char *const MODEL_PATH2 = R"(../assets/untitled.obj)";

const char *const TEXTURE_PATH = R"(../assets/track.png)";
// const char *const TEXTURE_PATH2 = R"(../assets/image.jpg)";

const int MAP_WIDTH = 200;
const int MAP_HEIGHT = 200;

class MapSearchNode {
public:
  glm::vec2 pos;

  MapSearchNode() { pos = {0, 0}; }
  MapSearchNode(int px, int py) {
    pos.x = px;
    pos.y = py;
  }

  float GoalDistanceEstimate(MapSearchNode &nodeGoal);
  bool IsGoal(MapSearchNode &nodeGoal);
  bool GetSuccessors(AStarSearch<MapSearchNode> *astarsearch,
                     MapSearchNode *parent_node);
  float GetCost(MapSearchNode &successor);
  bool IsSameState(MapSearchNode &rhs);
  size_t Hash();

  void PrintNodeInfo();
};

std::vector<std::vector<int>> occupancy_grid(MAP_WIDTH,
                                             std::vector<int>(MAP_HEIGHT, 0));

int GetMap(int x, int y) {
  if (x < 0 || x >= MAP_WIDTH || y < 0 || y >= MAP_HEIGHT) {
    return 9;
  }
  return occupancy_grid[x][y];
}

bool MapSearchNode::IsSameState(MapSearchNode &rhs) {

  // same state in a maze search is simply when (x,y) are the same
  if ((pos.x == rhs.pos.x) && (pos.y == rhs.pos.y)) {
    return true;
  } else {
    return false;
  }
}

size_t MapSearchNode::Hash() {
  size_t h1 = std::hash<float>{}(pos.x);
  size_t h2 = std::hash<float>{}(pos.y);
  return h1 ^ (h2 << 1);
}

void MapSearchNode::PrintNodeInfo() {
  const int strSize = 100;
  char str[strSize];
  snprintf(str, strSize, "Node position : (%d,%d)\n", (int)pos.x, (int)pos.y);

  std::cout << str;
}

// Here's the heuristic function that estimates the distance from a Node
// to the Goal.

float MapSearchNode::GoalDistanceEstimate(MapSearchNode &nodeGoal) {
  return std::abs(pos.x - nodeGoal.pos.x) + std::abs(pos.y - nodeGoal.pos.y);
}

bool MapSearchNode::IsGoal(MapSearchNode &nodeGoal) {

  if ((pos.x == nodeGoal.pos.x) && (pos.y == nodeGoal.pos.y)) {
    return true;
  }

  return false;
}

// This generates the successors to the given Node. It uses a helper function
// called AddSuccessor to give the successors to the AStar class. The A*
// specific initialisation is done for each node internally, so here you just
// set the state information that is specific to the application
bool MapSearchNode::GetSuccessors(AStarSearch<MapSearchNode> *astarsearch,
                                  MapSearchNode *parent_node) {

  int parent_x = -1;
  int parent_y = -1;

  if (parent_node) {
    parent_x = parent_node->pos.x;
    parent_y = parent_node->pos.y;
  }

  MapSearchNode NewNode;

  // push each possible move except allowing the search to go backwards
  float search = GetMap(pos.x - 1, pos.y);
  float current = GetMap(pos.x, pos.y);
  if ((search < 9) && !((parent_x == pos.x - 1) && (parent_y == pos.y))) {
    NewNode = MapSearchNode(pos.x - 1, pos.y);
    astarsearch->AddSuccessor(NewNode);
  }

  // search = GetMap(pos.x, pos.y - 1);
  // if ((search < 9) && (current == 0 && search != 0) &&
  //     !((parent_x == pos.x) && (parent_y == pos.y - 1))) {
  //   NewNode = MapSearchNode(pos.x, pos.y - 1);
  //   astarsearch->AddSuccessor(NewNode);
  // }

  search = GetMap(pos.x + 1, pos.y);
  if ((search < 9) && !((parent_x == pos.x + 1) && (parent_y == pos.y))) {
    NewNode = MapSearchNode(pos.x + 1, pos.y);
    astarsearch->AddSuccessor(NewNode);
  }

  search = GetMap(pos.x, pos.y + 1);
  if ((search < 9) && !((parent_x == pos.x) && (parent_y == pos.y + 1))) {
    NewNode = MapSearchNode(pos.x, pos.y + 1);
    astarsearch->AddSuccessor(NewNode);
  }

  search = GetMap(pos.x + 1, pos.y + 1);
  if ((search < 9) && !((parent_x == pos.x) && (parent_y == pos.y + 1))) {
    NewNode = MapSearchNode(pos.x, pos.y + 1);
    astarsearch->AddSuccessor(NewNode);
  }

  search = GetMap(pos.x - 1, pos.y + 1);
  if ((search < 9) && !((parent_x == pos.x) && (parent_y == pos.y + 1))) {
    NewNode = MapSearchNode(pos.x, pos.y + 1);
    astarsearch->AddSuccessor(NewNode);
  }

  search = GetMap(pos.x - 1, pos.y - 1);
  if ((search < 9) && !((parent_x == pos.x) && (parent_y == pos.y + 1))) {
    NewNode = MapSearchNode(pos.x, pos.y + 1);
    astarsearch->AddSuccessor(NewNode);
  }

  search = GetMap(pos.x + 1, pos.y - 1);
  if ((search < 9) && !((parent_x == pos.x) && (parent_y == pos.y + 1))) {
    NewNode = MapSearchNode(pos.x, pos.y + 1);
    astarsearch->AddSuccessor(NewNode);
  }

  return true;
}

// given this node, what does it cost to move to successor. In the case
// of our map the answer is the map terrain value at this node since that is
// conceptually where we're moving

float MapSearchNode::GetCost(MapSearchNode &successor) {
  return (float)GetMap(pos.x, pos.x);
}

std::vector<glm::vec2> astar(glm::vec2 start, glm::vec2 end) {
  std::vector<glm::vec2> solution = {};

  // Our sample problem defines the world as a 2d array representing a terrain
  // Each element contains an integer from 0 to 5 which indicates the cost
  // of travel across the terrain. Zero means the least possible difficulty
  // in travelling (think ice rink if you can skate) whilst 5 represents the
  // most difficult. 9 indicates that we cannot pass.

  // Create an instance of the search class...

  AStarSearch<MapSearchNode> astarsearch;

  unsigned int SearchCount = 0;

  const unsigned int NumSearches = 1;

  while (SearchCount < NumSearches) {

    // Create a start state
    MapSearchNode nodeStart;
    nodeStart.pos = start;

    // Define the goal state
    MapSearchNode nodeEnd;
    nodeEnd.pos = end;
    // Set Start and goal states

    astarsearch.SetStartAndGoalStates(nodeStart, nodeEnd);

    unsigned int SearchState;
    unsigned int SearchSteps = 0;

    do {
      SearchState = astarsearch.SearchStep();

      SearchSteps++;

#if DEBUG_LISTS

      cout << "Steps:" << SearchSteps << "\n";

      int len = 0;

      cout << "Open:\n";
      MapSearchNode *p = astarsearch.GetOpenListStart();
      while (p) {
        len++;
#if !DEBUG_LIST_LENGTHS_ONLY
        ((MapSearchNode *)p)->PrintNodeInfo();
#endif
        p = astarsearch.GetOpenListNext();
      }

      cout << "Open list has " << len << " nodes\n";

      len = 0;

      cout << "Closed:\n";
      p = astarsearch.GetClosedListStart();
      while (p) {
        len++;
#if !DEBUG_LIST_LENGTHS_ONLY
        p->PrintNodeInfo();
#endif
        p = astarsearch.GetClosedListNext();
      }

      cout << "Closed list has " << len << " nodes\n";
#endif

    } while (SearchState == AStarSearch<MapSearchNode>::SEARCH_STATE_SEARCHING);

    if (SearchState == AStarSearch<MapSearchNode>::SEARCH_STATE_SUCCEEDED) {
      // std::cout << "Search found goal state\n";

      MapSearchNode *node = astarsearch.GetSolutionStart();

#if DISPLAY_SOLUTION
      cout << "Displaying solution\n";
#endif
      int steps = 0;

      // node->PrintNodeInfo();
      solution.push_back(std::move(node->pos));
      for (;;) {
        node = astarsearch.GetSolutionNext();

        if (!node) {
          break;
        }
        solution.push_back(std::move(node->pos));
        // node->PrintNodeInfo();
        steps++;
      };

      // std::cout << "Solution steps " << steps << std::endl;

      // Once you're done with the solution you can free the nodes up
      astarsearch.FreeSolutionNodes();

    } else if (SearchState == AStarSearch<MapSearchNode>::SEARCH_STATE_FAILED) {
      // std::cout << "Search terminated. Did not find goal state\n";
    }

    // Display the number of loops the search went through
    // std::cout << "SearchSteps : " << SearchSteps << "\n";

    SearchCount++;

    astarsearch.EnsureMemoryFreed();
  }

  return solution;
}

// Optimized LIDAR to local coordinates function (1 & 4)
std::vector<std::array<float, 2>>
lidar_to_local(float robot_x, float robot_y, float robot_theta,
               std::vector<Ray> &lidar_samples) {
  const int num_samples = 720;
  std::vector<float> lidar_angles(num_samples);
  std::vector<std::array<float, 2>> result;

  for (int i = 0; i < num_samples; ++i) {
    lidar_angles[i] = M_PI * (180.0 - 360.0 * i / (num_samples - 1)) / 180.0;
  }

  // Use parallel loop to speed up processing of each sample
  for (int i = 0; i < num_samples; ++i) {
    // if (lidar_samples[i].t > 5) { // Skip invalid points
    float x_local = lidar_samples[i].t * cos(lidar_angles[i]);
    float y_local = lidar_samples[i].t * sin(lidar_angles[i]);
    result.push_back({x_local, y_local});
    // }
  }

  return result;
}

// Optimized concentric buffer function (2 & 3)
void add_concentric_plus_buffers(std::vector<std::vector<int>> &array, int r1,
                                 int r2) {
  const int rows = array.size();
  const int cols = array[0].size();

  std::vector<std::pair<int, int>> directions = {
      {1, 0}, {-1, 0}, {0, 1}, {0, -1}};
#pragma omp parallel for // Parallelize loop if OpenMP is available
  for (int i = r1; i < rows - r1; ++i) {
    for (int j = r1; j < cols - r1; ++j) {
      if (array[i][j] == 9) {
        // Add radius r1 buffer
        for (const auto &[dx, dy] : directions) {
          for (int r = 1; r <= r1; ++r) {
            if (i + dx * r >= 0 && i + dx * r < MAP_WIDTH && j + dy * r >= 0 &&
                j + dy * r < MAP_HEIGHT)
              array[i + dx * r][j + dy * r] = 9;
          }
        }
        // Add radius r2 buffer
        for (const auto &[dx, dy] : directions) {
          for (int r = r1 + 1; r <= r2; ++r) {
            if (i + dx * r >= 0 && i + dx * r < MAP_WIDTH && j + dy * r >= 0 &&
                j + dy * r < MAP_HEIGHT)
              array[i + dx * r][j + dy * r] = 1;
          }
        }
      }
    }
  }
}

std::atomic<int> counter{0};
std::atomic<bool> isDriving{false};
std::array<float, 2> position{0.0f, 0.0f};
std::atomic<float> angle{0.0f};
std::array<float, 2> velocity{0.0f, 0.0f};

void update2(double deltaTime) {
  angle = angle + car.angularVelocity * deltaTime;
  auto samples = lidar_to_local(position[0], position[1], angle,
                                LIDAR); // Placeholder lidar samples

  // std::array<std::array<float, 2>, 2> rotation_matrix = {
  //     {{std::cos(angle), -std::sin(angle)},
  //      {std::sin(angle), std::cos(angle)}}};
  // auto acc_local =
  //     std::array<float, 2>{rc::physics::get_linear_acceleration()[0],
  //                          rc::physics::get_linear_acceleration()[2]};
  // std::array<float, 2> acc_global{rotation_matrix[0][0] * acc_local[0] +
  //                                     rotation_matrix[0][1] * acc_local[1],
  //                                 rotation_matrix[1][0] * acc_local[0] +
  //                                     rotation_matrix[1][1] * acc_local[1]};
  // velocity[0] += acc_global[0] * deltaTime;
  // velocity[1] += acc_global[1] * deltaTime;
  // position[0] += velocity[0] * deltaTime;
  // position[1] += velocity[1] * deltaTime;

  for (auto &row : occupancy_grid) {
    std::fill(row.begin(), row.end(), 0);
  }
  // Initialize occupancy grid
  // FLIP CORDS!!!!!
  for (const auto &[x, y] : samples) {
    int y_coord = static_cast<int>(x * 50);
    int x_coord = static_cast<int>(y * 50) + MAP_WIDTH / 2;

    if (x_coord >= 0 && x_coord < MAP_WIDTH && y_coord >= 0 &&
        y_coord < MAP_HEIGHT) {
      occupancy_grid[x_coord][y_coord] = 9;
    }
  }

  int buffer_size = 1;
  int buffer_size_2 = 5;
  // this could be more efficient
  add_concentric_plus_buffers(occupancy_grid, buffer_size, buffer_size_2);

  glm::vec2 start = {MAP_WIDTH / 2, 0};
  glm::vec2 end = {MAP_WIDTH / 2, MAP_HEIGHT};
  std::vector<glm::vec2> path = astar(start, end);

  int car_size = 15;
  float heading = M_PI / 2.0;
  if (path.size() > car_size) {
    heading = atan2(path[car_size].x, path[car_size].y - MAP_WIDTH / 2.0f);
  } else if (path.size() > 0) {
    heading = atan2(path[path.size() - 1][0],
                    path[path.size() - 1][1] - MAP_WIDTH / 2.0f);
  }

  angle = heading - M_PI / 2.0;

  float drive = 0.1f;
  float multiplier = 4.0f;
  float clamped_angle =
      std::max(std::min(angle * multiplier, 1.0f), -1.0f); // Clamp angle
  speed = drive;
  steeringAngle = clamped_angle;
}

void mainLoop() {

  bool lastCursor = false;
  bool capture_cursor = true;
  // glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

  double xpos = 0.0, ypos = 0.0;
  double currentTime;
  double lastTime = 0.0;
  float spf;
  bool firstFrame = true;
  double frameTimes = 0;
  uint32_t f = 0;

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
    // Print the number of seconds for 1000 frames
    if (f == 1000) {
      printf("%f\n", frameTimes);
      f = 0;
      frameTimes = 0;
    }
    glfwPollEvents();

    // tab out of game
    if (glfwGetKey(window, GLFW_KEY_TAB)) {
      if (!lastCursor) {
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
    // if not tabbed out, get the cursor and reset pos, else don't move
    // mouse
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

      // get LIDAR data and tells us when we hit walls or choose to reset
      if (update() || glfwGetKey(window, GLFW_KEY_X)) {
        Infinite::cameras.setPositon({-1.0f, 0.9f, -0.05f});
        car.velocity = 0;
        car.position = 0;
        car.angularVelocity = 0;
        car.acceleration = 0;
        std::cout << "AHHH" << std::endl;
        continue;
      }
      speed = 0.0f;
      steeringAngle = 0.0f;
      // autonomous driving
      update2(spf);

      // movement
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
      // "drive" the car/camera
      float inverseVelocity = std::abs(car.velocity);
      float brakeTorque =
          std::abs(speed) < 0.1f
              ? sgn(car.velocity) * inverseVelocity * brakeTorqueScale
              : speed * torque;

      float carPos = car.update(steeringAngle * 0.20f, brakeTorque, spf);
      cameras.move(carPos, Infinite::FORWARD);
      cameras.setAngles(car.heading + M_PI / 2.0, -M_PI / 2.0);

      if (glfwGetKey(window, GLFW_KEY_F)) {
        car.velocity = 0;
        car.position = 0;
        car.angularVelocity = 0;
        car.acceleration = 0;
      }
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

  mainPass.addModel(&mainModel);

  cameras.setAngles(M_PI / 2.0, -M_PI / 2.0);

  UpdateBoundingVolumeHierarchy("../assets/bvh", mainModel);

  try {
    mainLoop();
    cleanUp();
    destroyBVH();
  } catch (const std::exception &e) {
    std::cerr << e.what() << std::endl;
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}