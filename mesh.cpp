// Copyright 2018 Zusitools

#include "mesh.hpp"

Mesh MeshOps::translate(float dx, float dy, float dz, const Mesh& mesh)  {
  Mesh result(mesh);
  for (auto& vertex : result.vertices) {
    vertex.pos_x += dx;
    vertex.pos_y += dy;
    vertex.pos_z += dz;
  }
  return result;
}

Mesh MeshOps::rotateZ180(const Mesh& mesh) {
  Mesh result(mesh);
  for (auto& vertex : result.vertices) {
    vertex.pos_x *= -1;
    vertex.pos_y *= -1;
    vertex.nor_x *= -1;
    vertex.nor_y *= -1;
    vertex.nor_z *= -1;
  }
  return result;
}
