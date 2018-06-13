// Copyright 2018 Zusitools

#ifndef HEKTO_BUILDER_HPP_
#define HEKTO_BUILDER_HPP_

#include "mesh.hpp"

#include <cstdio>
#include <cstdint>
#include <vector>

enum class Hoehe { kHoch, kNiedrig };
enum class Mast { kMitMast, kOhneMast };
enum class Beidseitig { kBeidseitig, kEinseitig };
enum class Groesse { kGross, kKlein };
enum class Rueckstrahlend { kYes, kNo };
enum class Ankerpunkt { kYes, kNo };

struct BauParameter final {
  Hoehe hoehe;
  Mast mast;
  Beidseitig beidseitig;
  Groesse groesse;
  Rueckstrahlend rueckstrahlend;
  Ankerpunkt ankerpunkt;
};

struct TafelParameter;

struct Ziffern final {
  Mesh mesh1;
  Mesh mesh2;
  std::vector<int> stuetzpunkte_oben;
  std::vector<int> stuetzpunkte_unten;
};

class SubsetBuilder final {
 public:
  SubsetBuilder(uint32_t tagfarbe, uint32_t nachtfarbe);
  void AddMesh(const Mesh& mesh);
  void Write(FILE* fd);
 private:
  Mesh m_mesh {};
  uint32_t tagfarbe_ {};
  uint32_t nachtfarbe_ {};
};

class TafelRueckseiteBuilder final {
 public:
  static Mesh Build(const TafelParameter& tp);
};

class TafelVorderseiteBuilder final {
 public:
  static Mesh Build(const TafelParameter& tp, const std::vector<int>& stuetzpunkte_oben, const std::vector<int>& stuetzpunkte_unten);
};

class ZiffernBuilder final {
 public:
  static Ziffern Build(const TafelParameter& tp, bool ist_negativ, int zahl_oben, int ziffer_unten);
};

class MastBuilder final {
 public:
  static Mesh Build(const TafelParameter& tp);
};

class HektoBuilder final {
 public:
  static void Build(FILE* fd, const BauParameter& bauparameter, int hektometer);
};

#endif  // HEKTO_BUILDER_HPP_
