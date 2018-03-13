// Copyright 2018 Zusitools

#ifndef MESH_HPP_
#define MESH_HPP_

#include <cstddef>
#include <vector>
#include <utility>

/* Koordinatensystem (Zusi)
 *     z
 *     ^
 *     |
 *  y<-+
 *
 * Die x-Achse zeigt in den Bildschirm hinein.
 */
struct Vertex final {
  float pos_x, pos_y, pos_z;
  float nor_x, nor_y, nor_z;
  float u1, v1;
  float u2, v2;

  inline Vertex(
      float pos_x, float pos_y, float pos_z,
      float nor_x, float nor_y, float nor_z,
      float u1, float v1, float u2, float v2)
    : pos_x(pos_x), pos_y(pos_y), pos_z(pos_z),
    nor_x(nor_x), nor_y(nor_y), nor_z(nor_z),
    u1(u1), v1(v1), u2(u2), v2(v2) {}
};

using VertexIndex = size_t;

/* Winding Order: clockwise */
struct Face final {
  VertexIndex i1, i2, i3;

  inline Face(VertexIndex i1, VertexIndex i2, VertexIndex i3)
    : i1(i1), i2(i2), i3(i3) {}
};

struct Mesh final {
  std::vector<Vertex> vertices;
  std::vector<Face> faces;

  Mesh() = default;

  Mesh(const Mesh&) = default;
  Mesh(Mesh&&) = default;

  Mesh& operator=(const Mesh&) = default;
  Mesh& operator=(Mesh&&) = default;

  template <typename... Args>
  VertexIndex EmplaceVertex(Args&&... args) {
    vertices.emplace_back(std::forward<Args>(args)...);
    return vertices.size() - 1;
  }
};

namespace MeshOps {
  Mesh translate(float dx, float dy, float dz, const Mesh& mesh);
  Mesh rotateZ180(const Mesh& mesh);
}

#endif  // MESH_HPP_
