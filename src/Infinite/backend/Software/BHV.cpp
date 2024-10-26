/*
 *  CUDA based triangle mesh path tracer using BVH acceleration by Sam lapere,
 * 2016 BVH implementation based on real-time CUDA ray tracer by Thanassis
 * Tsiodras, http://users.softlab.ntua.gr/~ttsiod/cudarenderer-BVH.html
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */
// heavily modified
#include <algorithm>
#include <assert.h>
#include <cfloat>
#include <chrono>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <ctime>
#include <glm/common.hpp>
#include <glm/detail/qualifier.hpp>
#include <glm/ext/vector_float3.hpp>
#include <glm/geometric.hpp>
#include <iostream>
#include <optional>
#include <stdio.h>
#include <string>
#include <sys/types.h>
#include <vector>

#include "BVH.h"

using namespace std;

// report progress during BVH construction
#define PROGRESS_REPORT
#ifdef PROGRESS_REPORT
#define REPORT(x) x
#define REPORTPRM(x) x,
#else
#define REPORT(x)
#define REPORTPRM(x)
#endif

unsigned g_reportCounter = 0;

unsigned g_trianglesNo;
Triangle g_triangles[1290] = {};

// The BVH
BVHNode *g_pSceneBVH = NULL;
std::vector<Infinite::Vertex> vertices;
// the cache-friendly version of the BVH, to be stored in a file
unsigned g_triIndexListNo = 0;
int *g_triIndexList = NULL;
unsigned g_pCFBVH_No = 0;
CacheFriendlyBVHNode *g_pCFBVH = NULL;

// Work item for creation of BVH:
struct BBoxTmp {
  // Bottom point (ie minx,miny,minz)
  glm::vec3 _bottom;
  // Top point (ie maxx,maxy,maxz)
  glm::vec3 _top;
  // Center point, ie 0.5*(top-bottom)
  glm::vec3 _center; // = bbox centroid
  // Triangle
  const Triangle *_pTri; // triangle list
  BBoxTmp()
      : _bottom(FLT_MAX, FLT_MAX, FLT_MAX), _top(-FLT_MAX, -FLT_MAX, -FLT_MAX),
        _pTri(NULL) {}
};

typedef std::vector<BBoxTmp> BBoxEntries; // vector of triangle bounding boxes
                                          // needed during BVH construction

// recursive building of BVH nodes
// work is the working list (std::vector<>) of triangle bounding boxes
BVHNode *Recurse(BBoxEntries &work, REPORTPRM(float pct = 0.) int depth = 0) {

  REPORT(float pctSpan = 11. / std::pow(3.f, depth);)

  // terminate recursion case:
  // if work set has less then 4 elements (triangle bounding boxes), create a
  // leaf node and create a list of the triangles contained in the node

  if (work.size() < 4) {
    BVHLeaf *leaf = new BVHLeaf;
    for (BBoxEntries::iterator it = work.begin(); it != work.end(); it++)
      leaf->_triangles.push_back(it->_pTri);
    return leaf;
  }

  // else, work size > 4, divide  node further into smaller nodes
  // start by finding the working list's bounding box (top and bottom)

  glm::vec3 bottom(FLT_MAX, FLT_MAX, FLT_MAX);
  glm::vec3 top(-FLT_MAX, -FLT_MAX, -FLT_MAX);

  // loop over all bboxes in current working list, expanding/growing the working
  // list bbox
  for (unsigned i = 0; i < work.size(); i++) { // meer dan 4 bboxen in work
    BBoxTmp &v = work[i];
    bottom = glm::min(bottom, v._bottom);
    top = glm::max(top, v._top);
  }

  // SAH, surface area heuristic calculation
  // find surface area of bounding box by multiplying the dimensions of the
  // working list's bounding box
  float side1 = top.x - bottom.x; // length bbox along X-axis
  float side2 = top.y - bottom.y; // length bbox along Y-axis
  float side3 = top.z - bottom.z; // length bbox along Z-axis

  // the current bbox has a cost of (number of triangles) * surfaceArea of C = N
  // * SA
  float minCost = work.size() * (side1 * side2 + side2 * side3 + side3 * side1);

  float bestSplit = FLT_MAX; // best split along axis, will indicate no split
                             // with better cost found (below)

  int bestAxis = -1;

  // Try all 3 axises X, Y, Z
  for (int j = 0; j < 3; j++) { // 0 = X, 1 = Y, 2 = Z axis

    int axis = j;

    // we will try dividing the triangles based on the current axis,
    // and we will try split values from "start" to "stop", one "step" at a
    // time.
    float start, stop, step;

    // X-axis
    if (axis == 0) {
      start = bottom.x;
      stop = top.x;
    }
    // Y-axis
    else if (axis == 1) {
      start = bottom.y;
      stop = top.y;
    }
    // Z-axis
    else {
      start = bottom.z;
      stop = top.z;
    }

    // In that axis, do the bounding boxes in the work queue "span" across,
    // (meaning distributed over a reasonable distance)? Or are they all already
    // "packed" on the axis? Meaning that they are too close to each other
    if (fabsf(stop - start) < 1e-4)
      // BBox side along this axis too short, we must move to a different axis!
      continue; // go to next axis

    // Binning: Try splitting at a uniform sampling (at equidistantly spaced
    // planes) that gets smaller the deeper we go: size of "sampling grid": 1024
    // (depth 0), 512 (depth 1), etc each bin has size "step"
    step = (stop - start) / (1024. / (depth + 1.));

#ifdef PROGRESS_REPORT
    // Progress report variables...
    float pctStart = pct + j * pctSpan; // j is axis
    float pctStep = pctSpan / ((stop - start - 2 * step) / step);
#endif

    // for each bin (equally spaced bins of size "step"):
    for (float testSplit = start + step; testSplit < stop - step;
         testSplit += step) {

#ifdef PROGRESS_REPORT
      if ((1023 & g_reportCounter++) == 0) {
        std::printf("\b\b\b%02d%%", int(pctStart));
        fflush(stdout);
      }
      pctStart += pctStep;
#endif

      // Create left and right bounding box
      glm::vec3 lbottom(FLT_MAX, FLT_MAX, FLT_MAX);
      glm::vec3 ltop(-FLT_MAX, -FLT_MAX, -FLT_MAX);

      glm::vec3 rbottom(FLT_MAX, FLT_MAX, FLT_MAX);
      glm::vec3 rtop(-FLT_MAX, -FLT_MAX, -FLT_MAX);

      // The number of triangles in the left and right bboxes (needed to
      // calculate SAH cost function)
      int countLeft = 0, countRight = 0;

      // For each test split (or bin), allocate triangles in remaining work list
      // based on their bbox centers this is a fast O(N) pass, no triangle
      // sorting needed (yet)
      for (unsigned i = 0; i < work.size(); i++) {

        BBoxTmp &v = work[i];

        // compute bbox center
        float value;
        if (axis == 0)
          value = v._center.x; // X-axis
        else if (axis == 1)
          value = v._center.y; // Y-axis
        else
          value = v._center.z; // Z-axis

        if (value < testSplit) {
          // if center is smaller then testSplit value, put triangle in Left
          // bbox
          lbottom = glm::min(lbottom, v._bottom);
          ltop = glm::max(ltop, v._top);
          countLeft++;
        } else {
          // else put triangle in right bbox
          rbottom = glm::min(rbottom, v._bottom);
          rtop = glm::max(rtop, v._top);
          countRight++;
        }
      }

      // Now use the Surface Area Heuristic to see if this split has a better
      // "cost"

      // First, check for stupid partitionings, ie bins with 0 or 1 triangles
      // make no sense
      if (countLeft <= 1 || countRight <= 1)
        continue;

      // It's a real partitioning, calculate the surface areas
      float lside1 = ltop.x - lbottom.x;
      float lside2 = ltop.y - lbottom.y;
      float lside3 = ltop.z - lbottom.z;

      float rside1 = rtop.x - rbottom.x;
      float rside2 = rtop.y - rbottom.y;
      float rside3 = rtop.z - rbottom.z;

      // calculate SurfaceArea of Left and Right BBox
      float surfaceLeft = lside1 * lside2 + lside2 * lside3 + lside3 * lside1;
      float surfaceRight = rside1 * rside2 + rside2 * rside3 + rside3 * rside1;

      // calculate total cost by multiplying left and right bbox by number of
      // triangles in each
      float totalCost = surfaceLeft * countLeft + surfaceRight * countRight;

      // keep track of cheapest split found so far
      if (totalCost < minCost) {
        minCost = totalCost;
        bestSplit = testSplit;
        bestAxis = axis;
      }
    } // end of loop over all bins
  } // end of loop over all axises

  // at the end of this loop (which runs for every "bin" or "sample location"),
  // we should have the best splitting plane, best splitting axis and bboxes
  // with minimal traversal cost

  // If we found no split to improve the cost, create a BVH leaf

  if (bestAxis == -1) {

    BVHLeaf *leaf = new BVHLeaf;
    for (BBoxEntries::iterator it = work.begin(); it != work.end(); it++)
      leaf->_triangles.push_back(
          it->_pTri); // put triangles of working list in leaf's triangle list
    return leaf;
  }

  // Otherwise, create BVH inner node with L and R child nodes, split with the
  // optimal value we found above

  BBoxEntries left;
  BBoxEntries right; // BBoxEntries is a vector/list of BBoxTmp
  glm::vec3 lbottom(FLT_MAX, FLT_MAX, FLT_MAX);
  glm::vec3 ltop(-FLT_MAX, -FLT_MAX, -FLT_MAX);
  glm::vec3 rbottom(FLT_MAX, FLT_MAX, FLT_MAX);
  glm::vec3 rtop(-FLT_MAX, -FLT_MAX, -FLT_MAX);

  // distribute the triangles in the left or right child nodes
  // for each triangle in the work set
  for (int i = 0; i < (int)work.size(); i++) {

    // create temporary bbox for triangle
    BBoxTmp &v = work[i];

    // compute bbox center
    float value;
    if (bestAxis == 0)
      value = v._center.x;
    else if (bestAxis == 1)
      value = v._center.y;
    else
      value = v._center.z;

    if (value < bestSplit) { // add temporary bbox v from work list to left
                             // BBoxentries list,
      // becomes new working list of triangles in next step

      left.push_back(v);
      lbottom = glm::min(lbottom, v._bottom);
      ltop = glm::max(ltop, v._top);
    } else {

      // Add triangle bbox v from working list to right BBoxentries,
      // becomes new working list of triangles in next step
      right.push_back(v);
      rbottom = glm::min(rbottom, v._bottom);
      rtop = glm::max(rtop, v._top);
    }
  } // end loop for each triangle in working set

  // create inner node
  BVHInner *inner = new BVHInner;

#ifdef PROGRESS_REPORT
  if ((1023 & g_reportCounter++) == 0) {
    std::printf("\b\b\b%2d%%",
                int(pct + 3.f * pctSpan)); // Update progress indicator
    fflush(stdout);
  }
#endif
  // recursively build the left child
  inner->_left = Recurse(left, REPORTPRM(pct + 3.f * pctSpan) depth + 1);
  inner->_left->_bottom = lbottom;
  inner->_left->_top = ltop;

#ifdef PROGRESS_REPORT
  if ((1023 & g_reportCounter++) == 0) {
    std::printf("\b\b\b%2d%%",
                int(pct + 6.f * pctSpan)); // Update progress indicator
    fflush(stdout);
  }
#endif
  // recursively build the right child
  inner->_right = Recurse(right, REPORTPRM(pct + 6.f * pctSpan) depth + 1);
  inner->_right->_bottom = rbottom;
  inner->_right->_top = rtop;

  return inner;
} // end of Recurse() function, returns the rootnode (when all recursion calls
  // have finished)

void loadTri(Infinite::Model mainModel) {
  vertices = mainModel.vertices;

  g_trianglesNo = 1290;
  uint32_t index = 0;
  for (uint32_t i = 0; i < mainModel.indices.size(); i += 3) {
    g_triangles[index]._idx1 = mainModel.indices[i];
    g_triangles[index]._idx2 = mainModel.indices[i + 1];
    g_triangles[index]._idx3 = mainModel.indices[i + 2];

    index++;
  }
}

BVHNode *CreateBVH(Infinite::Model mainModel) {
  /* Summary:
  1. Create work BBox
  2. Create BBox for every triangle and compute bounds
  3. Expand bounds work BBox to fit all triangle bboxes
  4. Compute triangle bbox centre and add triangle to working list
  5. Build BVH tree with Recurse()
  6. Return root node
  */
  loadTri(mainModel);

  std::vector<BBoxTmp> work;
  glm::vec3 bottom(FLT_MAX, FLT_MAX, FLT_MAX);
  glm::vec3 top(-FLT_MAX, -FLT_MAX, -FLT_MAX);

  puts("Gathering bounding box info from all triangles...");
  // for each triangle
  for (unsigned j = 0; j < g_trianglesNo; j++) {
    const Triangle &triangle = g_triangles[j];
    // create a new temporary bbox per triangle
    BBoxTmp b;
    b._pTri = &triangle;

    // loop over triangle vertices and pick smallest vertex for bottom of
    // triangle bbox

    b._bottom = vertices[triangle._idx1].pos; // index of vertex
    b._bottom = glm::min(b._bottom, vertices[triangle._idx2].pos);
    b._bottom = glm::min(b._bottom, vertices[triangle._idx3].pos);

    // loop over triangle vertices and pick largest vertex for top of triangle
    // bbox
    b._top = vertices[triangle._idx1].pos;
    b._top = glm::max(b._top, vertices[triangle._idx2].pos);
    b._top = glm::max(b._top, vertices[triangle._idx3].pos);

    // expand working list bbox by largest and smallest triangle bbox bounds
    bottom = glm::min(bottom, b._bottom);
    top = glm::max(top, b._top);

    // compute triangle bbox center: (bbox top + bbox bottom) * 0.5
    b._center = (b._top + b._bottom) * 0.5f;

    // add triangle bbox to working list
    work.push_back(b);
  }

  // ...and pass it to the recursive function that creates the SAH AABB BVH
  // (Surface Area Heuristic, Axis-Aligned Bounding Boxes, Bounding Volume
  // Hierarchy)

  std::printf("Creating Bounding Volume Hierarchy data...    ");
  fflush(stdout);
  BVHNode *root = Recurse(work); // builds BVH and returns root node
  printf("\b\b\b100%%\n");

  root->_bottom =
      bottom; // bottom is bottom of bbox bounding all triangles in the scene
  root->_top = top;

  return root;
}

// the following functions are required to create the cache-friendly BVH
// recursively count bboxes
int CountBoxes(BVHNode *root) {
  if (!root->IsLeaf()) {
    BVHInner *p = dynamic_cast<BVHInner *>(root);
    return 1 + CountBoxes(p->_left) + CountBoxes(p->_right);
  } else
    return 1;
}

// recursively count triangles
unsigned CountTriangles(BVHNode *root) {
  if (!root->IsLeaf()) {
    BVHInner *p = dynamic_cast<BVHInner *>(root);
    return CountTriangles(p->_left) + CountTriangles(p->_right);
  } else {
    BVHLeaf *p = dynamic_cast<BVHLeaf *>(root);
    return (unsigned)p->_triangles.size();
  }
}

// recursively count depth
void CountDepth(BVHNode *root, int depth, int &maxDepth) {
  if (maxDepth < depth)
    maxDepth = depth;
  if (!root->IsLeaf()) {
    BVHInner *p = dynamic_cast<BVHInner *>(root);
    CountDepth(p->_left, depth + 1, maxDepth);
    CountDepth(p->_right, depth + 1, maxDepth);
  }
}

// Writes in the g_pCFBVH and g_triIndexListNo arrays,
// creating a cache-friendly version of the BVH
void PopulateCacheFriendlyBVH(const Triangle *pFirstTriangle, BVHNode *root,
                              unsigned &idxBoxes, unsigned &idxTriList) {
  unsigned currIdxBoxes = idxBoxes;
  g_pCFBVH[currIdxBoxes]._bottom = root->_bottom;
  g_pCFBVH[currIdxBoxes]._top = root->_top;

  // DEPTH FIRST APPROACH (left first until complete)
  if (!root->IsLeaf()) { // inner node
    BVHInner *p = dynamic_cast<BVHInner *>(root);
    // recursively populate left and right
    int idxLeft = ++idxBoxes;
    PopulateCacheFriendlyBVH(pFirstTriangle, p->_left, idxBoxes, idxTriList);
    int idxRight = ++idxBoxes;
    PopulateCacheFriendlyBVH(pFirstTriangle, p->_right, idxBoxes, idxTriList);
    g_pCFBVH[currIdxBoxes].u.inner._idxLeft = idxLeft;
    g_pCFBVH[currIdxBoxes].u.inner._idxRight = idxRight;
    // g_pCFBVH[currIdxBoxes].u.leaf._count = 0;
    // g_pCFBVH[currIdxBoxes].u.leaf._startIndexInTriIndexList = 0;
  } else { // leaf
    BVHLeaf *p = dynamic_cast<BVHLeaf *>(root);
    unsigned count = (unsigned)p->_triangles.size();
    g_pCFBVH[currIdxBoxes].u.leaf._count =
        0x80000000 | count; // highest bit set indicates a leaf node (inner node
                            // if highest bit is 0)
    g_pCFBVH[currIdxBoxes].u.leaf._startIndexInTriIndexList = idxTriList;

    for (std::list<const Triangle *>::iterator it = p->_triangles.begin();
         it != p->_triangles.end(); it++) {
      g_triIndexList[idxTriList++] = *it - pFirstTriangle;
    }
  }
}

#define BVH_STACK_SIZE 32

void CreateCFBVH() {
  if (g_pSceneBVH == NULL) {
    puts("Internal bug in CreateCFBVH, please report it...");
    fflush(stdout);
    exit(1);
  }

  unsigned idxTriList = 0;
  unsigned idxBoxes = 0;

  g_triIndexListNo = CountTriangles(g_pSceneBVH);
  g_triIndexList = new int[g_triIndexListNo];

  g_pCFBVH_No = CountBoxes(g_pSceneBVH);
  g_pCFBVH = new CacheFriendlyBVHNode[g_pCFBVH_No];
  // array

  PopulateCacheFriendlyBVH(&g_triangles[0], g_pSceneBVH, idxBoxes, idxTriList);

  if ((idxBoxes != g_pCFBVH_No - 1) || (idxTriList != g_triIndexListNo)) {
    puts("Internal bug in CreateCFBVH, please report it...");
    fflush(stdout);
    exit(1);
  }

  int maxDepth = 0;
  CountDepth(g_pSceneBVH, 0, maxDepth);
  if (maxDepth >= BVH_STACK_SIZE) {
    printf("Current Max Depth: %u, is larger than max BVH_STACK_SIZE %u\n", maxDepth, BVH_STACK_SIZE);
    puts("Recompile with BVH_STACK_SIZE set to more than that...");
    fflush(stdout);
    exit(1);
  }
}

// bool IntersectAABB(const Ray &ray, const glm::vec3 bmin, const glm::vec3
// bmax) {
//   float tx1 = (bmin.x - ray.O.x) / ray.D.x, tx2 = (bmax.x - ray.O.x) /
//   ray.D.x; float tmin = min(tx1, tx2), tmax = max(tx1, tx2); float ty1 =
//   (bmin.y - ray.O.y) / ray.D.y, ty2 = (bmax.y - ray.O.y) / ray.D.y; tmin =
//   max(tmin, min(ty1, ty2)), tmax = min(tmax, max(ty1, ty2)); float tz1 =
//   (bmin.z - ray.O.z) / ray.D.z, tz2 = (bmax.z - ray.O.z) / ray.D.z; tmin =
//   max(tmin, min(tz1, tz2)), tmax = min(tmax, max(tz1, tz2)); return tmax >=
//   tmin && tmin < ray.t && tmax > 0;
// }

// bool IntersectAABB(const Ray &ray, const glm::vec3 bmin, const glm::vec3
// bmax) {
//   float tx1 = (bmin.x - ray.O.x) * ray.inv.x;
//   float tx2 = (bmax.x - ray.O.x) * ray.inv.x;
//   float tmin = min(tx1, tx2);
//   float tmax = max(tx1, tx2);
//   std::cout << tmax << " " << tmin << std::endl;
//   float ty1 = (bmin.y - ray.O.y) * ray.inv.y;
//   float ty2 = (bmax.y - ray.O.y) * ray.inv.y;
//   tmin = max(tmin, min(ty1, ty2));
//   tmax = min(tmax, max(ty1, ty2));
//   std::cout << tmax << " " << tmin << std::endl;
//   float tz1 = (bmin.z - ray.O.z) * ray.inv.z;
//   float tz2 = (bmax.z - ray.O.z) * ray.inv.z;
//   tmin = max(tmin, min(tz1, tz2));
//   tmax = min(tmax, max(tz1, tz2));
//   std::cout << tmax << " " << tmin << std::endl;
//   return tmax >= tmin && tmin < ray.t && tmax > 0;
// }

// this was hell to debug
bool IntersectAABB(const Ray &ray, const glm::vec3 bmin, const glm::vec3 bmax) {
  float tmin = (bmin.x - ray.O.x) * ray.inv.x;
  float tmax = (bmax.x - ray.O.x) * ray.inv.x;

  if (ray.inv.x < 0.0f) {
    std::swap(tmin, tmax);
  }

  float tymin = (bmin.y - ray.O.y) * ray.inv.y;
  float tymax = (bmax.y - ray.O.y) * ray.inv.y;

  if (ray.inv.y < 0.0f) {
    std::swap(tymin, tymax);
  }

  if (tmin > tymax || tymin > tmax) {
    return false;
  }

  tmin = std::max(tmin, tymin);
  tmax = std::min(tmax, tymax);

  float tzmin = (bmin.z - ray.O.z) * ray.inv.z;
  float tzmax = (bmax.z - ray.O.z) * ray.inv.z;

  if (ray.inv.z < 0.0f) {
    std::swap(tzmin, tzmax);
  }

  if (tmin > tzmax || tzmin > tmax) {
    return false;
  }

  tmin = std::max(tmin, tzmin);
  tmax = std::min(tmax, tzmax);

  return tmax >= tmin && tmin < ray.t && tmax > 0;
}

// Optional float to store the intersection distance 't' if there's a hit
std::optional<float> rayTriangleIntersection(Ray &ray, Triangle &triangle) {
  const float EPSILON = 1e-8;

  glm::vec3 v0 = vertices[triangle._idx1].pos;
  glm::vec3 v1 = vertices[triangle._idx2].pos;
  glm::vec3 v2 = vertices[triangle._idx3].pos;
  // Edges of the triangle
  glm::vec3 edge1 = v1 - v0;
  glm::vec3 edge2 = v2 - v0;

  // Begin calculating determinant - also used to calculate u parameter
  glm::vec3 h = glm::cross(ray.D, edge2);
  float a = glm::dot(edge1, h);

  // If the determinant is near zero, the ray is parallel to the triangle
  if (a > -EPSILON && a < EPSILON)
    return std::nullopt;

  float f = 1.0 / a;
  glm::vec3 s = ray.O - v0;
  float u = f * glm::dot(s, h);

  // Check if intersection lies outside the triangle
  if (u < 0.0 || u > 1.0)
    return std::nullopt;

  glm::vec3 q = glm::cross(s, edge1);
  float v = f * glm::dot(ray.D, q);

  // Check if intersection lies outside the triangle
  if (v < 0.0 || u + v > 1.0)
    return std::nullopt;

  // Calculate t to determine the distance of the intersection
  float t = f * glm::dot(edge2, q);

  // Ray intersection
  if (t > EPSILON)
    return t;

  // This means there is a line intersection, but not a ray intersection
  // basically there is an intersection with -1*ray
  return std::nullopt;
}

void Intersect(Ray *ray, uint32_t index) {
  //<<Follow ray through BVH nodes to find primitive intersections>>
  // int toVisitOffset = 0, currentNodeIndex = 0;
  // BVHNode *nodesToVisit[64];
  CacheFriendlyBVHNode *node = &g_pCFBVH[index];
  //<<Check ray against BVH node>>

  // std::cout << "(" << node->_bottom.x << ", " << node->_bottom.y << ", "
  //           << node->_bottom.z << ") (" << node->_top.x << ", " <<
  //           node->_top.y
  //           << ", " << node->_top.z << ")" << std::endl;
  if (IntersectAABB(*ray, node->_bottom, node->_top)) {
    // is a leaf
    if ((node->u.leaf._count & 0x80000000) != 0) {
      // intersect triangles
      for (uint32_t i = 0; i < (node->u.leaf._count & ~0x80000000); i++) {
        // std::cout << "TriIndex: " << node->u.leaf._startIndexInTriIndexList +
        // i
        //           << std::endl;
        std::optional<float> value = rayTriangleIntersection(
            *ray,
            g_triangles[g_triIndexList[node->u.leaf._startIndexInTriIndexList +
                                       i]]);
        if (value.has_value()) {
          ray->t = glm::min(value.value(), ray->t);
          // std::cout << index << " -> Ray Hit" << " Distance: " << ray.t
          // <<std::endl;
          return;
        }
      }
      // std::cout << index << " -> Ray Miss" << std::endl;
      return;
    } else {
      //<<Put far BVH node on nodesToVisit stack, advance to near node>>
      // std::cout << index << " -> " << node->u.inner._idxLeft << ", "
      //           << node->u.inner._idxRight << std::endl;
      Intersect(ray, node->u.inner._idxLeft);
      Intersect(ray, node->u.inner._idxRight);
      return;
    }
  } else {
    // std::cout << index << " -> AABB Miss" << std::endl;
    return;
  }
}

// void traversal(Ray ray, uint32_t index) {
//   CacheFriendlyBVHNode *node = &g_pCFBVH[index];
//   std::cout << index << " AABB? "
//             << IntersectAABB(ray, node->_bottom, node->_top) << std::endl;
//   if ((node->u.leaf._count & 0x80000000) != 0) {
//     for (uint32_t i = 0; i < (node->u.leaf._count & ~0x80000000); i++) {
//       std::cout << "TriIndex: " << node->u.leaf._startIndexInTriIndexList + i
//                 << " " << (node->u.leaf._count & ~0x80000000) << std::endl;
//       std::optional<float> value = rayTriangleIntersection(
//           ray,
//           g_triangles[g_triIndexList[node->u.leaf._startIndexInTriIndexList +
//                                      i]]);
//       if (value.has_value()) {
//         ray.t = glm::min(value.value(), ray.t);
//         std::cout << index << " -> Ray Hit" << std::endl;
//       } else {
//         std::cout << index << " -> Ray Miss" << std::endl;
//       }
//     }
//     return;
//   } else {
//     std::cout << index << " -> " << node->u.inner._idxLeft << ", "
//               << node->u.inner._idxRight << std::endl;
//     traversal(ray, node->u.inner._idxLeft);
//     traversal(ray, node->u.inner._idxRight);
//   }
// }

std::vector<Ray> generateRaysAroundPoint(const glm::vec3 &origin) {
  float angleStep =
      0.5f * glm::pi<float>() / 180.0f; // Convert degrees to radians
  std::vector<Ray> rays;
  float angle = 0.0f;
  for (int i = 0; i < 720; ++i) {
    float x = std::cos(angle);
    float y = std::sin(angle);
    glm::vec3 direction =
        glm::vec3(x, y, 0.0f); // Assuming 2D rays in the XY plane
    Ray r = {origin, direction, INFINITY, 1.0f / direction};
    rays.push_back(r);
    angle += angleStep;
  }

  return rays;
}

// The gateway - creates the "pure" BVH, and then copies the results in the
// cache-friendly one
void UpdateBoundingVolumeHierarchy(const char *filename,
                                   Infinite::Model mainModel) {
  if (!g_pSceneBVH) {
    std::string BVHcacheFilename(filename);
    BVHcacheFilename += ".bvh";
    FILE *fp = fopen(BVHcacheFilename.c_str(), "rb");
    if (!fp) {
      // No cached BVH data - we need to calculate them
      g_pSceneBVH = CreateBVH(mainModel);
      // Now that the BVH has been created, copy its data into a more
      // cache-friendly format (CacheFriendlyBVHNode occupies exactly 32 bytes,
      // i.e. a cache-line)
      CreateCFBVH();

      // Now store the results, if possible...
      fp = fopen(BVHcacheFilename.c_str(), "wb");
      if (!fp)
        return;
      if (1 != fwrite(&g_pCFBVH_No, sizeof(unsigned), 1, fp))
        return;
      if (1 != fwrite(&g_triIndexListNo, sizeof(unsigned), 1, fp))
        return;
      if (g_pCFBVH_No !=
          fwrite(g_pCFBVH, sizeof(CacheFriendlyBVHNode), g_pCFBVH_No, fp))
        return;
      if (g_triIndexListNo !=
          fwrite(g_triIndexList, sizeof(int), g_triIndexListNo, fp))
        return;
      fclose(fp);
    } else { // BVH has been built already and stored in a file, read the
             // file
      puts("Cache exists, reading the pre-calculated BVH data...");
      if (1 != fread(&g_pCFBVH_No, sizeof(unsigned), 1, fp))
        return;
      if (1 != fread(&g_triIndexListNo, sizeof(unsigned), 1, fp))
        return;
      g_pCFBVH = new CacheFriendlyBVHNode[g_pCFBVH_No];
      g_triIndexList = new int[g_triIndexListNo];
      if (g_pCFBVH_No !=
          fread(g_pCFBVH, sizeof(CacheFriendlyBVHNode), g_pCFBVH_No, fp))
        return;
      if (g_triIndexListNo !=
          fread(g_triIndexList, sizeof(int), g_triIndexListNo, fp))
        return;
      fclose(fp);
      loadTri(mainModel);
    }
  }
}

std::vector<Ray> LIDAR;

bool update() {
  bool ahhh = false;
  LIDAR = generateRaysAroundPoint(glm::vec3(Infinite::cameras.getPosition().x, Infinite::cameras.getPosition().y, 0.05f));
  // std::cout << Infinite::cameras.getPosition().z << std::endl;

#pragma omp parallel for
  for (uint32_t i = 0; i < LIDAR.size(); i++) {
    Intersect(&LIDAR[i], 0);
    if (LIDAR[i].t < 0.04) {
      ahhh = true;
    }
  }

  // std::cout << LIDAR[180].t << std::endl;

  return ahhh;
}

void destroyBVH() {
      delete g_triIndexList;
      delete g_pCFBVH;
  // for(uint32_t i = 0; i < 1290; i++) {
  // }
}

// std::cout << "(" << vertices[g_triangles[i]._idx1].pos.x << ", "
//                 << vertices[g_triangles[i]._idx1].pos.y << ", "
//                 << vertices[g_triangles[i]._idx1].pos.z << ") ("
//                 << vertices[g_triangles[i]._idx2].pos.x << ", "
//                 << vertices[g_triangles[i]._idx2].pos.y << ", "
//                 << vertices[g_triangles[i]._idx2].pos.z << ") ("
//                 << vertices[g_triangles[i]._idx3].pos.x << ", "
//                 << vertices[g_triangles[i]._idx3].pos.y << ", "
//                 << vertices[g_triangles[i]._idx3].pos.z << std::endl;