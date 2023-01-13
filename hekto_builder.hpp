// Copyright 2018 Zusitools

#ifndef HEKTO_BUILDER_HPP_
#define HEKTO_BUILDER_HPP_

#include "mesh.hpp"

#include <cstdio>
#include <cstdint>
#include <cmath>
#include <optional>
#include <vector>

enum class Hoehe { kHoch, kNiedrig };
enum class Mast { kMitMast, kOhneMast };
enum class Beidseitig { kBeidseitig, kEinseitig };
enum class Groesse { kGross, kKlein };
enum class Rueckstrahlend { kYes, kNo };
enum class Ankerpunkt { kYes, kNo };
enum class TexturDatei { kStandard = 0, kTunnel = 1, kVerwittert1 = 2, kVerwittert2 = 3 };

struct BauParameter final {
  Hoehe hoehe;
  Mast mast;
  Beidseitig beidseitig;
  Groesse groesse;
  Rueckstrahlend rueckstrahlend;
  Ankerpunkt ankerpunkt;
  TexturDatei textur;
};

struct TafelParameter;

struct Kilometrierung {
  // Invariante: -oo < km < oo
  // Invariante: 0 <= |hm| <= 9
  // Invariante: km < 0 ==> hm < 0
  // Invariante: km > 0 ==> hm > 0
  const int km;
  const int hm;

  static Kilometrierung fromMeter(int wert_m) {
    // Runde auf Hektometer
    int wert_m_int = std::abs(wert_m) + 50;
    int vorzeichen = (wert_m < 0 ? -1 : 1);
    return {
      vorzeichen * (wert_m_int / 1000),
      vorzeichen * ((wert_m_int % 1000) / 100)
    };
  }

  int toHektometer() const {
    return 10 * km + hm;
  }

  bool istNegativ() const {
    return hm < 0;
  }
};

constexpr int kMaxUeberlaenge = 39;

//       +-*--*-+
//       |      |
//       |      |
//  +-*--+      +-*-+
//  |               |
//  |               |
//  +-*--*--*---*-*-+

struct Ziffern final {
  Mesh mesh1;
  Mesh mesh2;
  std::vector<int> stuetzpunkte_oben;
  std::vector<int> stuetzpunkte_unten;
};

class SubsetBuilder final {
 public:
  SubsetBuilder(uint32_t tagfarbe, uint32_t nachtfarbe, size_t textur_idx);
  void AddMesh(const Mesh& mesh);
  void Write(FILE* fd);
 private:
  Mesh m_mesh {};
  uint32_t tagfarbe_ {};
  uint32_t nachtfarbe_ {};
  size_t textur_idx_ {};
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
  static Ziffern Build(const TafelParameter& tp, bool ist_negativ, int zahl_oben, int ziffer_unten, std::optional<int> ueberlaenge);
};

class MastBuilder final {
 public:
  static Mesh Build(const TafelParameter& tp);
};

class HektoBuilder final {
 public:
  static void Build(FILE* fd, const BauParameter& bauparameter, Kilometrierung kilometrierung, std::optional<int> ueberlaenge_hm);
};

#endif  // HEKTO_BUILDER_HPP_
