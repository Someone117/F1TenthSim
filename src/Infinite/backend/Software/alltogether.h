#include <cmath>
#include <cstdint>
#include <glm/common.hpp>
#include <glm/fwd.hpp>
#include <glm/vec3.hpp>
#include <sys/types.h>
#include "../Model/Models/Model.h"


// triangle count
const uint32_t N = 1290; // hardcoded for the unity vehicle mesh
// bin count
#define BINS 8

template <class T> void swap(T &x, T &y) {
  T t;
  t = x, x = y, y = t;
}

// minimal structs
struct Tri {
  glm::vec3 vertex0, vertex1, vertex2;
  glm::vec3 centroid;
};
struct BVHNode {
  glm::vec3 aabbMin, aabbMax;
  uint leftFirst = 0, triCount = 0;
  inline bool isLeaf() { return triCount > 0; }
};

extern Tri tri[N];
extern uint triIdx[N];
extern BVHNode bvhNode[N * 2 - 1];
extern uint rootNodeIdx, nodesUsed;

struct aabb {
  glm::vec3 bmin = {1e30f, 1e30f, 1e30f}, bmax = {-1e30f, -1e30f, -1e30f};
  inline void grow(glm::vec3 p) {

    bmin = glm::min(bmin, p);
    bmax = glm::max(bmax, p);
  }
  inline void grow(aabb &b) {
    if (b.bmin.x != 1e30f) {
      grow(b.bmin);
      grow(b.bmax);
    }
  }
  inline float area() {
    glm::vec3 e = bmax - bmin; // box extent
    return e.x * e.y + e.y * e.z + e.z * e.x;
  }
};
struct Bin {
  aabb bounds;
  int triCount = 0;
};
struct Ray {
  glm::vec3 O, D;
  float t = 1e30f;
};

void init(Infinite::Model mainModel);
void Subdivide(uint nodeIdx);
float CalculateNodeCost(BVHNode &node);
float FindBestSplitPlane(BVHNode &node, int &axis, float &splitPos);
void UpdateNodeBounds(uint nodeIdx);
void BuildBVH();
void IntersectBVH(Ray &ray);
void castRays(glm::vec3 origin = glm::vec3(0, 0, 0));

void a();