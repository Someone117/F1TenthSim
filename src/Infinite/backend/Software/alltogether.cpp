#ifndef TINYOBJLOADER_IMPLEMENTATION
#define TINYOBJLOADER_IMPLEMENTATION
#include <glm/ext/quaternion_trigonometric.hpp>
#include <iostream>
#endif

#ifndef TINYOBJLOADER_USE_MAPBOX_EARCUT
#define TINYOBJLOADER_USE_MAPBOX_EARCUT
#endif

#include "../../Infinite.h"
#include "../../util/constants.h"
#include "alltogether.h"
#include <algorithm>
#include <cstdint>
#include <cstdio>
#include <ctime>
#include <glm/fwd.hpp>
#include <glm/geometric.hpp>
#include <glm/trigonometric.hpp>
#include <stdexcept>
#include <sys/types.h>

// #include <experimental/simd>
// THIS SOURCE FILE:
// Code for the article "How to Build a BVH", part 3: quick BVH builds.
// This version improves BVH construction speed using binned SAH
// construction, heavily based on a 2007 paper by Ingo Wald:
// On fast Construction of SAH-based Bounding Volume Hierarchies
// Feel free to copy this code to your own framework. Absolutely no
// rights are reserved. No responsibility is accepted either.
// For updates, follow me on twitter: @j_bikker.

Tri tri[N];
uint triIdx[N];
BVHNode bvhNode[N * 2 - 1] = {};
uint rootNodeIdx = 0, nodesUsed = 2;

// functions

void IntersectTri(Ray &ray, const Tri &tri) {
  const glm::vec3 edge1 = tri.vertex1 - tri.vertex0;
  const glm::vec3 edge2 = tri.vertex2 - tri.vertex0;
  const glm::vec3 h = cross(ray.D, edge2);
  const float a = dot(edge1, h);
  if (a > -0.0001f && a < 0.0001f)
    return; // ray parallel to triangle
  const float f = 1 / a;
  const glm::vec3 s = ray.O - tri.vertex0;
  const float u = f * dot(s, h);
  if (u < 0 || u > 1)
    return;
  const glm::vec3 q = cross(s, edge1);
  const float v = f * dot(ray.D, q);
  if (v < 0 || u + v > 1)
    return;
  const float t = f * dot(edge2, q);
  if (t > 0.0001f)
    ray.t = std::min(ray.t, t);
}

float IntersectAABB(const Ray &ray, const glm::vec3 bmin,
                    const glm::vec3 bmax) {
  float tx1 = (bmin.x - ray.O.x) * ray.D.x;
  float tx2 = (bmax.x - ray.O.x) * ray.D.x;
  float tmin = glm::min(tx1, tx2);
  float tmax = glm::max(tx1, tx2);
  float ty1 = (bmin.y - ray.O.y) * ray.D.y, ty2 = (bmax.y - ray.O.y) * ray.D.y;
  tmin = glm::max(tmin, glm::min(ty1, ty2)),
  tmax = glm::min(tmax, glm::max(ty1, ty2));
  float tz1 = (bmin.z - ray.O.z) * ray.D.z, tz2 = (bmax.z - ray.O.z) * ray.D.z;
  tmin = glm::max(tmin, glm::min(tz1, tz2)),
  tmax = glm::min(tmax, glm::max(tz1, tz2));
  if (tmax >= tmin && tmin < ray.t && tmax > 0)
    return tmin;
  else
    return 1e30f;
}

void IntersectBVH(Ray &ray) {
  BVHNode node = bvhNode[0];
  BVHNode stack[64] = {};
  uint stackPtr = 0;
  while (1) {
    if (node.isLeaf()) {
      for (uint i = 0; i < node.triCount; i++)
        IntersectTri(ray, tri[triIdx[node.leftFirst + i]]);
      if (stackPtr == 0)
        break;
      else
        node = stack[--stackPtr];
      continue;
    }

    BVHNode child1 = bvhNode[node.leftFirst];
    BVHNode child2 = bvhNode[node.leftFirst + 1];
    float dist1 = IntersectAABB(ray, child1.aabbMin, child1.aabbMax);
    float dist2 = IntersectAABB(ray, child2.aabbMin, child2.aabbMax);
    if (dist1 > dist2) {
      swap(dist1, dist2);
      swap(child1, child2);
    }
    if (dist1 == 1e30f) {
      if (stackPtr == 0)
        break;
      else
        node = stack[--stackPtr];
    } else {
      node = child1;
      if (dist2 != 1e30f)
        stack[stackPtr++] = child2;
    }
  }
}

void BuildBVH() {
  // assign all triangles to root node
  BVHNode &root = bvhNode[rootNodeIdx];
  root.leftFirst = 0, root.triCount = N;
  UpdateNodeBounds(rootNodeIdx);
  // subdivide recursively
  Subdivide(rootNodeIdx);
}

void UpdateNodeBounds(uint nodeIdx) {
  BVHNode &node = bvhNode[nodeIdx];
  node.aabbMin = glm::vec3(1e30f);
  node.aabbMax = glm::vec3(-1e30f);
  for (uint first = node.leftFirst, i = 0; i < node.triCount; i++) {
    uint leafTriIdx = triIdx[first + i];
    Tri &leafTri = tri[leafTriIdx];
    node.aabbMin = glm::min(node.aabbMin, leafTri.vertex0);
    node.aabbMin = glm::min(node.aabbMin, leafTri.vertex1);
    node.aabbMin = glm::min(node.aabbMin, leafTri.vertex2);
    node.aabbMax = glm::max(node.aabbMax, leafTri.vertex0);
    node.aabbMax = glm::max(node.aabbMax, leafTri.vertex1);
    node.aabbMax = glm::max(node.aabbMax, leafTri.vertex2);
  }
}

float FindBestSplitPlane(BVHNode &node, int &axis, float &splitPos) {
  float bestCost = 1e30f;
  for (int a = 0; a < 3; a++) {
    float boundsMin = 1e30f, boundsMax = -1e30f;
    for (uint i = 0; i < node.triCount; i++) {
      Tri &triangle = tri[triIdx[node.leftFirst + i]];
      boundsMin = glm::min(boundsMin, triangle.centroid[a]);
      boundsMax = glm::max(boundsMax, triangle.centroid[a]);
    }
    if (boundsMin == boundsMax)
      continue;
    // populate the bins
    Bin bin[BINS];
    float scale = BINS / (boundsMax - boundsMin);
    for (uint i = 0; i < node.triCount; i++) {
      Tri &triangle = tri[triIdx[node.leftFirst + i]];
      int binIdx =
          glm::min(BINS - 1, (int)((triangle.centroid[a] - boundsMin) * scale));
      bin[binIdx].triCount++;
      bin[binIdx].bounds.grow(triangle.vertex0);
      bin[binIdx].bounds.grow(triangle.vertex1);
      bin[binIdx].bounds.grow(triangle.vertex2);
    }
    // gather data for the 7 planes between the 8 bins
    float leftArea[BINS - 1], rightArea[BINS - 1];
    int leftCount[BINS - 1], rightCount[BINS - 1];
    aabb leftBox, rightBox;
    int leftSum = 0, rightSum = 0;
    for (int i = 0; i < BINS - 1; i++) {
      leftSum += bin[i].triCount;
      leftCount[i] = leftSum;
      leftBox.grow(bin[i].bounds);
      leftArea[i] = leftBox.area();
      rightSum += bin[BINS - 1 - i].triCount;
      rightCount[BINS - 2 - i] = rightSum;
      rightBox.grow(bin[BINS - 1 - i].bounds);
      rightArea[BINS - 2 - i] = rightBox.area();
    }
    // calculate SAH cost for the 7 planes
    scale = (boundsMax - boundsMin) / BINS;
    for (int i = 0; i < BINS - 1; i++) {
      float planeCost =
          leftCount[i] * leftArea[i] + rightCount[i] * rightArea[i];
      if (planeCost < bestCost)
        axis = a, splitPos = boundsMin + scale * (i + 1), bestCost = planeCost;
    }
  }
  return bestCost;
}

float CalculateNodeCost(BVHNode &node) {
  glm::vec3 e = node.aabbMax - node.aabbMin; // extent of the node
  float surfaceArea = e.x * e.y + e.y * e.z + e.z * e.x;
  return node.triCount * surfaceArea;
}

void a() {

}

void Subdivide(uint nodeIdx) {
  // terminate recursion
  BVHNode &node = bvhNode[nodeIdx];
  // determine split axis using SAH
  int axis;
  float splitPos;
  float splitCost = FindBestSplitPlane(node, axis, splitPos);
  float nosplitCost = CalculateNodeCost(node);
  a();
  std::cout << splitCost << " " << nosplitCost << std::endl;
  if (splitCost >= nosplitCost)
    return;
  // in-place partition
  int i = node.leftFirst;
  int j = i + node.triCount - 1;
  while (i <= j) {
    if (tri[triIdx[i]].centroid[axis] < splitPos)
      i++;
    else
      swap(triIdx[i], triIdx[j--]);
  }
  // abort split if one of the sides is empty
  int leftCount = i - node.leftFirst;
  if (leftCount == 0 || leftCount == node.triCount)
    return;
  // create child nodes
  int leftChildIdx = nodesUsed++;
  int rightChildIdx = nodesUsed++;
  bvhNode[leftChildIdx].leftFirst = node.leftFirst;
  bvhNode[leftChildIdx].triCount = leftCount;
  bvhNode[rightChildIdx].leftFirst = i;
  bvhNode[rightChildIdx].triCount = node.triCount - leftCount;
  node.leftFirst = leftChildIdx;
  node.triCount = 0;
  UpdateNodeBounds(leftChildIdx);
  UpdateNodeBounds(rightChildIdx);
  // recurse
  Subdivide(leftChildIdx);
  Subdivide(rightChildIdx);
}

void init(Infinite::Model mainModel) {
  uint32_t index = 0;
  for (uint32_t i = 0; i < mainModel.indices.size(); i += 3) {
    tri[index] = {mainModel.vertices[mainModel.indices[i]].pos,
                  mainModel.vertices[mainModel.indices[i + 1]].pos,
                  mainModel.vertices[mainModel.indices[i + 2]].pos,
                  glm::vec3(0.0, 0.0, 0.0)};
    tri[i].centroid =
        (tri[i].vertex0 + tri[i].vertex1 + tri[i].vertex2) * 0.3333f;
    triIdx[index] = index;

    index++;
  }
  BuildBVH();
}

void castRays(glm::vec3 origin) {
  Ray rays[720];

  // #pragma omp parallel for

  origin = {10.0f, 9.0f, 0.5f};

  // r.D = tri[0].centroid - r.O;
  // std::cout << tri[1].centroid.z << std::endl;

  // #pragma omp parallel for
  // for (uint32_t i = 0; i < N; i++) {

  //   IntersectTri(r, tri[i]);
  //   if (r.t != 1e30f)
  //     break;
  // }
  // IntersectBVH(r);

  // for (uint32_t i = 0; i < 720; i++) {
  //   rays[i].O = origin;
  //   // Calculate the current angle in radians
  //   float angle = glm::radians(i * 0.5f);

  //   // Calculate the vector direction (keeping the Z the same as origin)
  //   glm::vec3 direction(glm::cos(angle), glm::sin(angle), origin.z);
  //   // Normalize the vector
  //   rays[i].D = glm::normalize(direction);
  //   for (uint32_t j = 0; j < N; j++) {

  //     IntersectTri(rays[i], tri[j]);
  //     if (rays[i].t != 1e30f)
  //       break;
  //   }
  // }
  // for (uint32_t i = 0; i < 720; i++) {
  //   // if(tri[i].centroid.z != 0.0f)
  //   std::cout << rays[i].t << std::endl;
  // }
}