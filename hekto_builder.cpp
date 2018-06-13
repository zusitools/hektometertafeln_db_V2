// Copyright 2018 Zusitools

#include "hekto_builder.hpp"

#include "textur.hpp"

#include <algorithm>
#include <array>
#include <cassert>
#include <cmath>
#include <cstdlib>
#include <functional>
#include <iterator>
#include <utility>


/* Koordinatensystem:
 *  y
 *  ^
 *  |
 *  +-> x
 */

struct TafelParameter final {
  int breite_mm;
  int hoehe_mm;

  int ziffernhoehe_mm;
  int def_ziffernabstand_mm;
  int max_ziffernabstand_mm;

  int zifferndicke_mm;

  const Textur& tex_tafel_vorderseite;
  const Textur& tex_tafel_rueckseite;
  const std::array<Textur, 10>& tex_ziffern;
  const Textur& tex_mast;
  const Textur& tex_transparent;

  inline int XLinks() const {
    return -breite_mm / 2;
  }

  inline int XRechts() const {
    return -breite_mm / 2 + breite_mm;
  }

  inline int YUnten() const {
    return -hoehe_mm / 2;
  }

  inline int YOben() const {
    return -hoehe_mm / 2 + hoehe_mm;
  }
};

SubsetBuilder::SubsetBuilder(uint32_t tagfarbe, uint32_t nachtfarbe)
    : tagfarbe_(tagfarbe), nachtfarbe_(nachtfarbe) { }

void SubsetBuilder::AddMesh(const Mesh& mesh) {
  VertexIndex indexOffset = m_mesh.vertices.size();

  std::copy(std::begin(mesh.vertices), std::end(mesh.vertices), std::back_inserter(m_mesh.vertices));
  std::transform(std::begin(mesh.faces), std::end(mesh.faces), std::back_inserter(m_mesh.faces),
      [indexOffset](const auto& face) -> Face {
        return { indexOffset + face.i1, indexOffset + face.i2, indexOffset + face.i3 };
      });
}

void SubsetBuilder::Write(FILE* fd) {
  if (m_mesh.vertices.size() == 0 || m_mesh.faces.size() == 0) {
    return;
  }

  fprintf(fd,
      "<SubSet Cd=\"%08X\" Ce=\"%08X\">\n"
      "<RenderFlags TexVoreinstellung=\"3\"/>\n"
      "<Textur><Datei Dateiname=\"_Setup\\lib\\milepost\\hektometertafeln_DB_v2\\hektometertafel.dds\"/></Textur>\n"
      "<Textur><Datei Dateiname=\"_Setup\\lib\\milepost\\hektometertafeln_DB_v2\\hektometertafel.dds\"/></Textur>\n",
      tagfarbe_, nachtfarbe_);

  for (const auto& vertex : m_mesh.vertices) {
    fprintf(fd,
        "<Vertex U=\"%f\" V=\"%f\" U2=\"%f\" V2=\"%f\">\n"
        "<p X=\"%f\" Y=\"%f\" Z=\"%f\"/>\n"
        "<n X=\"%f\" Y=\"%f\" Z=\"%f\"/>\n"
        "</Vertex>\n",
        vertex.u1, vertex.v1,
        vertex.u2, vertex.v2,
        vertex.pos_x, vertex.pos_y, vertex.pos_z,
        vertex.nor_x, vertex.nor_y, vertex.nor_z);
  }

  for (const auto& face : m_mesh.faces) {
    fprintf(fd,
        "<Face i=\"%zu;%zu;%zu\"/>\n",
        face.i1, face.i2, face.i3);
  }

  fprintf(fd, "</SubSet>\n");
}


namespace {

/* Abgerundete Ecken. Jede Ecke (lo, lu, ro, ru) besteht aus 2 Vertices,
 * einem oberen und einem unteren.
 *
 *   + v_lo_oben
 *   v
 *   +---...
 *  /
 * + <- v_lo_unten
 * |
 * |
 * .
 *
 * */
struct TafelEcken final {
  VertexIndex v_lo_unten;
  VertexIndex v_lo_oben;
  VertexIndex v_ro_oben;
  VertexIndex v_ro_unten;
  VertexIndex v_ru_oben;
  VertexIndex v_ru_unten;
  VertexIndex v_lu_unten;
  VertexIndex v_lu_oben;
};

constexpr static int kEckenRadius_mm = 20;

TafelEcken BaueTafelEcken(Mesh* mesh, const TafelParameter& tp, const Textur& textur) {
  TafelEcken result;

  auto MakeVertex = [&](int x, int y) -> VertexIndex {
    return mesh->EmplaceVertex(
        0, -x / 1000.0, y / 1000.0,
        -1, 0, 0,
        textur.GetU(x), textur.GetV(y),
        tp.tex_transparent.u_links, tp.tex_transparent.v_unten);
  };

  // Fuege einen zusaetzlichen Vertex im Abstand kEckenOffset_mm von der Ecke ein,
  // sodass jede abgerundete Ecke aus 3 Vertices gebildet wird.
  constexpr static float kEckenOffset_mm = 5.86;

  result.v_lo_unten = MakeVertex(tp.XLinks(),                    tp.YOben() - kEckenRadius_mm);
  const auto v_lo   = MakeVertex(tp.XLinks() + kEckenOffset_mm,  tp.YOben() - kEckenOffset_mm);
  result.v_lo_oben  = MakeVertex(tp.XLinks() + kEckenRadius_mm,  tp.YOben());
  mesh->faces.emplace_back(result.v_lo_unten, v_lo, result.v_lo_oben);

  result.v_ro_oben  = MakeVertex(tp.XRechts() - kEckenRadius_mm, tp.YOben());
  const auto v_ro   = MakeVertex(tp.XRechts() - kEckenOffset_mm, tp.YOben() - kEckenOffset_mm);
  result.v_ro_unten = MakeVertex(tp.XRechts(),                   tp.YOben() - kEckenRadius_mm);
  mesh->faces.emplace_back(result.v_ro_oben, v_ro, result.v_ro_unten);

  result.v_ru_oben  = MakeVertex(tp.XRechts(),                   tp.YUnten() + kEckenRadius_mm);
  const auto v_ru   = MakeVertex(tp.XRechts() - kEckenOffset_mm, tp.YUnten() + kEckenOffset_mm);
  result.v_ru_unten = MakeVertex(tp.XRechts() - kEckenRadius_mm, tp.YUnten());
  mesh->faces.emplace_back(result.v_ru_oben, v_ru, result.v_ru_unten);

  result.v_lu_unten = MakeVertex(tp.XLinks() + kEckenRadius_mm,  tp.YUnten());
  const auto v_lu   = MakeVertex(tp.XLinks() + kEckenOffset_mm,  tp.YUnten() + kEckenOffset_mm);
  result.v_lu_oben  = MakeVertex(tp.XLinks(),                    tp.YUnten() + kEckenRadius_mm);
  mesh->faces.emplace_back(result.v_lu_unten, v_lu, result.v_lu_oben);

  return result;
}

}  // namespace

namespace {

/**
 * Erstellt ein Viereck aus v1-v4, wobei auf der Kante v3-v4 zusaetzliche Stuetzpunkte
 * gemaess der x-Koordinaten in [intermediate_begin, intermediate_end[
 * (2D-Koordinatensystem, Einheit mm!) eingebaut werden.
 *
 *       z
 *       ^
 *       |
 * y<----+
 *
 * v1------>v2
 * ^ \    / |
 * |   \/   v
 * v4<-----v3
 *       ^intermediate_begin
 *    ^intermediate_end
 *
 * Falls v3 == v4, wird nur ein Dreieck v1-v2-v3 erstellt.
 */
template <typename It>
void MakeQuad(Mesh* mesh, VertexIndex v1, VertexIndex v2, VertexIndex v3,
    const It& intermediate_begin, const It& intermediate_end, VertexIndex v4) {
  if (v3 == v4) {
    mesh->faces.emplace_back(v1, v2, v3);
    assert(intermediate_begin >= intermediate_end);
    return;
  }

  // Kopie statt Referenz, da mesh->vertices beim Einfuegen realloziert werden kann.
  const auto v3_vertex = mesh->vertices[v3];
  const auto v4_vertex = mesh->vertices[v4];

  // Erstelle Dreiecke mit Eckpunkt v2 so lange, bis ein Zwischenpunkt naeher an v4 als an v3 liegt.
  // Ab dort setze seitenwechsel=true und erstelle Dreiecke mit Eckpunkt v1.
  bool seitenwechsel = false;

  auto cur_vertex_idx = v3;
  for (auto it = intermediate_begin; it < intermediate_end; ++it) {
    auto last_vertex_idx = cur_vertex_idx;

    auto cur_vertex = v3_vertex;
    cur_vertex.pos_y = -*it / 1000.0;

    // Berechne t mit v4 = v3 + t(v4-v3)
    const float t = (cur_vertex.pos_y - v3_vertex.pos_y) / (v4_vertex.pos_y - v3_vertex.pos_y);
    cur_vertex.u1 += t * (v4_vertex.u1 - v3_vertex.u1);
    cur_vertex.u2 += t * (v4_vertex.u2 - v3_vertex.u2);

    mesh->vertices.push_back(cur_vertex);
    cur_vertex_idx = mesh->vertices.size() - 1;

    if (!seitenwechsel) {
      mesh->faces.emplace_back(v2, last_vertex_idx, cur_vertex_idx);
      if (t <= -.5) {
        mesh->faces.emplace_back(v2, cur_vertex_idx, v1);
        seitenwechsel = true;
      }
    } else {
      mesh->faces.emplace_back(last_vertex_idx, cur_vertex_idx, v1);
    }
  }

  if (!seitenwechsel) {
    mesh->faces.emplace_back(v2, cur_vertex_idx, v1);
    seitenwechsel = true;
  }

  assert(seitenwechsel);

  mesh->faces.emplace_back(cur_vertex_idx, v4, v1);
}

/**
 *  v3----->v4
 *  ^   /\   |
 *  | /    \ v
 *  v2<-----v1
 */
template <typename It>
void MakeQuad(Mesh* mesh, VertexIndex v1,
    const It& intermediate_begin, const It& intermediate_end,
    VertexIndex v2, VertexIndex v3, VertexIndex v4) {
  MakeQuad(mesh, v3, v4, v1, intermediate_begin, intermediate_end, v2);
}

}  // namespace


Mesh TafelRueckseiteBuilder::Build(const TafelParameter& tp) {
  Mesh result;

  auto ecken = BaueTafelEcken(&result, tp, tp.tex_tafel_rueckseite);

  // Fuellung
  result.faces.emplace_back(ecken.v_lu_oben,  ecken.v_lo_unten, ecken.v_lo_oben);
  result.faces.emplace_back(ecken.v_lo_oben,  ecken.v_ro_oben,  ecken.v_ro_unten);
  result.faces.emplace_back(ecken.v_ro_unten, ecken.v_ru_oben,  ecken.v_ru_unten);
  result.faces.emplace_back(ecken.v_ru_unten, ecken.v_lu_unten, ecken.v_lu_oben);

  result.faces.emplace_back(ecken.v_lu_oben,  ecken.v_lo_oben,  ecken.v_ro_unten);
  result.faces.emplace_back(ecken.v_ro_unten, ecken.v_ru_unten, ecken.v_lu_oben);

  return result;
}


namespace {
  constexpr int kYTafelMitte_mm = 20;  // hier beruehren sich die Meshes von unterer und oberer Zahl

  // Default-Beschnittzugabe der Ziffern nach oben und unten
  constexpr int kYAbstandZiffern_mm = 25;
}  // namespace

Mesh TafelVorderseiteBuilder::Build(const TafelParameter& tp, const std::vector<int>& stuetzpunkte_oben, const std::vector<int>& stuetzpunkte_unten) {
  assert(stuetzpunkte_oben.size() >= 2);
  assert(stuetzpunkte_unten.size() == 2);

  Mesh result;

  auto ecken = BaueTafelEcken(&result, tp, tp.tex_tafel_vorderseite);

  auto MakeVertex = [&](int x, int y) -> VertexIndex {
    return result.EmplaceVertex(
        0, -x / 1000.0, y / 1000.0,
        -1, 0, 0,
        tp.tex_tafel_vorderseite.GetU(x), tp.tex_tafel_vorderseite.GetV(y),
        tp.tex_transparent.u_links, tp.tex_transparent.v_unten);
  };

  auto zahlOben_LinksOben = MakeVertex(stuetzpunkte_oben.front(), tp.YOben() - kEckenRadius_mm);

  // Fuellung links oben
  if (stuetzpunkte_oben.front() > tp.XLinks()) {
    result.faces.emplace_back(ecken.v_lo_unten, ecken.v_lo_oben, zahlOben_LinksOben);
  }

  auto zahl_oben_ro = MakeVertex(stuetzpunkte_oben.back(), tp.YOben() - kEckenRadius_mm);

  // Fuellung rechts oben
  if (stuetzpunkte_oben.back() < tp.XRechts()) {
    result.faces.emplace_back(ecken.v_ro_unten, zahl_oben_ro, ecken.v_ro_oben);
  }

  // Fuellung oben
  MakeQuad(&result, ecken.v_lo_oben, ecken.v_ro_oben, zahl_oben_ro,
      std::upper_bound(std::crbegin(stuetzpunkte_oben), std::crend(stuetzpunkte_oben), stuetzpunkte_oben.back(), std::greater<int>()),
      std::lower_bound(std::crbegin(stuetzpunkte_oben), std::crend(stuetzpunkte_oben), stuetzpunkte_oben.front(), std::greater<int>()),
      zahlOben_LinksOben);

  auto zahl_unten_ru = MakeVertex(stuetzpunkte_unten.back(), kYTafelMitte_mm - tp.ziffernhoehe_mm - 2 * kYAbstandZiffern_mm);
  auto zahl_unten_lu = MakeVertex(stuetzpunkte_unten.front(), kYTafelMitte_mm - tp.ziffernhoehe_mm - 2 * kYAbstandZiffern_mm);

  // Fuellung unten
  result.faces.emplace_back(ecken.v_ru_oben, ecken.v_ru_unten, zahl_unten_ru);
  result.faces.emplace_back(ecken.v_ru_unten, ecken.v_lu_unten, zahl_unten_ru);
  result.faces.emplace_back(ecken.v_lu_unten, zahl_unten_lu, zahl_unten_ru);
  result.faces.emplace_back(ecken.v_lu_unten, ecken.v_lu_oben, zahl_unten_lu);

  auto zahl_unten_lo = MakeVertex(stuetzpunkte_unten.front(), kYTafelMitte_mm);
  auto zahl_oben_lu = MakeVertex(stuetzpunkte_oben.front(), kYTafelMitte_mm);

  // Fuellung links unten
  MakeQuad(&result, zahl_unten_lu, ecken.v_lu_oben,
      stuetzpunkte_oben.front() < stuetzpunkte_unten.front() ? zahl_oben_lu : zahl_unten_lo,
      std::upper_bound(std::cbegin(stuetzpunkte_oben), std::cend(stuetzpunkte_oben), stuetzpunkte_oben.front()),
      std::lower_bound(std::cbegin(stuetzpunkte_oben), std::cend(stuetzpunkte_oben), stuetzpunkte_unten.front()),
      zahl_unten_lo);

  // Fuellung links Mitte
  if (stuetzpunkte_oben.front() > tp.XLinks() || stuetzpunkte_unten.front() > tp.XLinks()) {
    result.faces.emplace_back(ecken.v_lu_oben, ecken.v_lo_unten,
        stuetzpunkte_unten.front() < stuetzpunkte_oben.front() ? zahl_unten_lo : zahl_oben_lu);
  }

  // Fuellung links oben
  MakeQuad(&result, ecken.v_lo_unten, zahlOben_LinksOben, zahl_oben_lu,
      std::upper_bound(std::crbegin(stuetzpunkte_unten), std::crend(stuetzpunkte_unten), stuetzpunkte_oben.front(), std::greater<int>()),
      std::lower_bound(std::crbegin(stuetzpunkte_unten), std::crend(stuetzpunkte_unten), stuetzpunkte_unten.front(), std::greater<int>()),
      stuetzpunkte_unten.front() < stuetzpunkte_oben.front() ? zahl_unten_lo : zahl_oben_lu);

  auto zahl_unten_ro = MakeVertex(stuetzpunkte_unten.back(), kYTafelMitte_mm);
  auto zahl_oben_ru = MakeVertex(stuetzpunkte_oben.back(), kYTafelMitte_mm);

  // Fuellung rechts unten
  MakeQuad(&result, ecken.v_ru_oben, zahl_unten_ru, zahl_unten_ro,
      std::upper_bound(std::cbegin(stuetzpunkte_oben), std::cend(stuetzpunkte_oben), stuetzpunkte_unten.back()),
      std::lower_bound(std::cbegin(stuetzpunkte_oben), std::cend(stuetzpunkte_oben), stuetzpunkte_oben.back()),
      stuetzpunkte_oben.back() > stuetzpunkte_unten.back() ? zahl_oben_ru : zahl_unten_ro);

  // Fuellung rechts Mitte
  if (stuetzpunkte_oben.back() < tp.XRechts() || stuetzpunkte_unten.back() < tp.XRechts()) {
    result.faces.emplace_back(ecken.v_ro_unten, ecken.v_ru_oben,
        stuetzpunkte_unten.back() > stuetzpunkte_oben.back() ? zahl_unten_ro : zahl_oben_ru);
  }

  // Fuellung rechts oben
  MakeQuad(&result, zahl_oben_ro, ecken.v_ro_unten,
      stuetzpunkte_unten.back() > stuetzpunkte_oben.back() ? zahl_unten_ro : zahl_oben_ru,
      std::upper_bound(std::crbegin(stuetzpunkte_unten), std::crend(stuetzpunkte_unten), stuetzpunkte_unten.back(), std::greater<int>()),
      std::lower_bound(std::crbegin(stuetzpunkte_unten), std::crend(stuetzpunkte_unten), stuetzpunkte_oben.back(), std::greater<int>()),
      zahl_oben_ru);

  return result;
}


namespace {

std::vector<int> GetZiffern(int zahl) {
  std::vector<int> result;
  while (zahl >= 10) {
    result.insert(result.begin(), zahl % 10);
    zahl /= 10;
  }
  result.insert(result.begin(), zahl);
  return result;
}

  static constexpr int kKerning_mm[11][11] = {
    //         rechte Ziffer
    // linke
    // Ziffer  0   1   2   3   4   5   6   7   8   9, -1
    /* 0 */ {  0,  0,  5,  5, 15, 10,  0, 10,  0,  0,  0 },
    /* 1 */ { -8, -8, -8, -8, -8, -8, -8, -8, -8, -8,  0 },
    /* 2 */ {  0,  0,  0,  0, 10,  0,  0,  0,  0,  0,  0 },
    /* 3 */ {  0,  0,  0,  0, 10,  0,  0,  0,  0,  0,  0 },
    /* 4 */ {  0,  0,  5,  0,  0,  0,  0, 10,  0,  5,  0 },
    /* 5 */ {  0,  0, 10,  0, 10,  6,  0,  0,  0,  5,  0 },
    /* 6 */ {  6, 10, 10,  8, 20,  5, 15,  5,  0, 10,  0 },
    /* 7 */ { 25,  5, 10, 10, 50, 30, 30,  0, 10,  5, 10 },
    /* 8 */ {  4,  5,  5,  8, 30,  8,  0,  0,  0,  4,  0 },
    /* 9 */ {  0,  0,  0,  6, 20, 10,  8,  0,  0,  0,  0 },
    /*-1 */ {  0,  0,  0,  0, 10,  5,  0,  0,  0,  0,  0 },
  };

  static constexpr int kKerningZiffernhoehe_mm = 310;  // Ziffernhoehe, fuer die das Kerning berechnet wurde

int GetZiffernAbstand(const TafelParameter& tp, int ziffer_links, int ziffer_rechts) {
  assert(ziffer_links != -1 || ziffer_rechts != -1);
  int anzahl_ziffern = (ziffer_links == -1 || ziffer_rechts == -1) ? 1 : 2;
  int result = anzahl_ziffern * tp.def_ziffernabstand_mm
    - kKerning_mm[ziffer_links == -1 ? 10 : ziffer_links][ziffer_rechts == -1 ? 10 : ziffer_rechts]
      * (static_cast<float>(tp.ziffernhoehe_mm)/kKerningZiffernhoehe_mm);
  return std::min(anzahl_ziffern * tp.max_ziffernabstand_mm, result);
}

std::vector<int> GetDefaultAbstaende(const TafelParameter& tp, const std::vector<int>& ziffern) {
  assert(ziffern.size() >= 1);
  std::vector<int> result = { GetZiffernAbstand(tp, -1, ziffern.front()) };
  for (size_t i = 0; i < ziffern.size() - 1; ++i) {
    result.push_back(GetZiffernAbstand(tp, ziffern[i], ziffern[i + 1]));
  }
  result.push_back(GetZiffernAbstand(tp, ziffern.back(), -1));
  assert(result.size() == ziffern.size() + 1);
  return result;
}

// Berechnet Ziffernabstaende so, dass die Ziffern in die angegebene Gesamtbreite passen
// und alle Abstaende >= 0 sind.
std::vector<int> GetAbstaende(const TafelParameter& tp, const std::vector<int>& ziffern, const std::array<Textur, 10>& ziffern_texturen, int gesamtbreite_mm) {
  auto result = GetDefaultAbstaende(tp, ziffern);
  // result kann an dieser Stelle noch negative Werte enthalten,
  // die durch spaeteres Erhoehen der Ziffernabstaende ausgeglichen werden koennen.

  // Berechne Gesamtbreite und Spielraum
  int breite_summe = 0;
  int spielraum_verkleinern = 0;
  for (const auto& ziffer : ziffern) {
    breite_summe += ziffern_texturen[ziffer].breite_mm;
  }
  for (const auto& abstand : result) {
    breite_summe += std::max(0, abstand);
    spielraum_verkleinern += abstand;
  }

  int spielraum = gesamtbreite_mm - breite_summe;
  if (spielraum < 0) {
    // Falls die Default-Abstaende zu gross sind:
    // Quetschen (Abstaende gleichmaessig verkleinern)
    assert(-spielraum <= spielraum_verkleinern);

    size_t j = 0;
    for (int i = 0, end = -spielraum; i < end; ++i) {
      while (result[j % result.size()] <= 0) {
        result[j % result.size()] = 0;  // keine Chance auf negatives Kerning in dieser Situation
        ++j;
      }
      result[j % result.size()] -= 1;
      spielraum += 1;
      ++j;
    }
  } else if (spielraum > 0) {
    if (ziffern.size() > 1) {
      // Falls links und rechts sehr viel Platz ist:
      // Fuege gleichmaessig mehr Abstand zwischen den Ziffern ein, aber:
      //  - beschraenke den Gesamtabstand auf den dreifachen Default-Abstand (+/- Kerning)
      //  - lasse links und rechts mindestens den doppelten Default-Abstand Platz
      for (int i = 0; i <= 2 * tp.def_ziffernabstand_mm; ++i) {
        // Pruefe Abstand links/rechts nur 1x pro Iteration,
        // da sich der Spielraum pro Iteration um max. 2mm verringert.
        if (spielraum <= 4 * tp.def_ziffernabstand_mm) {
          break;
        }

        for (size_t j = 1; j < result.size() - 1; ++j) {
          if (result[j] < tp.max_ziffernabstand_mm) {
            ++result[j];
            --spielraum;
          }
        }
      }

      // Entferne verbleibende negative Abstaende zwischen Ziffern durch Kerning
      for (size_t i = 1; i < result.size() - 1; ++i) {
        if (result[i] < 0) {
          spielraum -= -result[i];
          result[i] = 0;
        }
      }
    }

    // Zentriere die Ziffern
    result.front() += spielraum / 2;
    spielraum -= spielraum / 2;

    result.back() += spielraum;
    spielraum -= spielraum;

  }

  assert(spielraum == 0);
  for (size_t i = 1; i < result.size() - 1; ++i) {
    assert(result[i] >= 0);
    assert(result[i] <= tp.max_ziffernabstand_mm);
  }
  return result;
}

template <typename T>
using Intervall = std::pair<T, T>;

// Berechnet unter Beruecksichtigung des maximalen Ziffernabstandes aus `tp`
// die Intervalle, in denen die Vertices fuer die gegebenen Ziffern mit den gegebenen Abstaenden
// liegen koennen.
std::vector<Intervall<int>> GetStuetzpunktIntervalle(const TafelParameter& tp, const std::vector<int>& ziffern, const std::vector<int>& abstaende) {
  std::vector<Intervall<int>> result;

  // Linken Stuetzpunkt, wenn moeglich, mit der linken Tafelseite zusammenfallen lassen
  int offset = tp.XLinks() + abstaende.front();
  if (offset - tp.max_ziffernabstand_mm <= tp.XLinks()) {
    result.emplace_back(tp.XLinks(), tp.XLinks());
  } else {
    result.emplace_back(offset - tp.max_ziffernabstand_mm, offset);
  }

  for (size_t i = 0; i < ziffern.size() - 1; ++i) {
    offset += tp.tex_ziffern[ziffern[i]].breite_mm;
    result.emplace_back(offset, offset + abstaende[i+1]);
    offset += abstaende[i+1];
  }

  // Rechten Stuetzpunkt, wenn moeglich, mit der rechten Tafelseite zusammenfallen lassen
  offset += tp.tex_ziffern[ziffern.back()].breite_mm;
  if (offset + tp.max_ziffernabstand_mm >= tp.XRechts()) {
    result.emplace_back(tp.XRechts(), tp.XRechts());
  } else {
    result.emplace_back(offset, offset + tp.max_ziffernabstand_mm);
  }

  return result;
}

// Berechnet aus den zwei angegebenen Listen von abgeschlossenen Intervallen
// (die Intervalle in jeder Liste sind paarweise disjunkt)
// zwei Listen von Werten, sodass jeder Wert im zugehoerigen Intervall
// der Ursprungsliste liegt und zusaetzlich moeglichst viele Werte
// in beiden Listen vorkommen.
template <typename T>
std::pair<std::vector<T>, std::vector<T>> GetStuetzpunkte(
    std::vector<Intervall<T>> stuetzpunkt_intervalle_1,
    std::vector<Intervall<T>> stuetzpunkt_intervalle_2) {
  std::vector<T> stuetzpunkte1, stuetzpunkte2;

  auto it1 = std::cbegin(stuetzpunkt_intervalle_1);
  auto end1 = std::cend(stuetzpunkt_intervalle_1);
  auto it2 = std::cbegin(stuetzpunkt_intervalle_2);
  auto end2 = std::cend(stuetzpunkt_intervalle_2);

  while (it1 != end1 || it2 != end2) {
    if (it2 == end2) {
      const T stuetzpunkt = (it1->first + it1->second) / 2;
      assert(stuetzpunkt >= it1->first);
      assert(stuetzpunkt <= it1->second);
      stuetzpunkte1.push_back(stuetzpunkt);
      ++it1;
    } else if (it1 == end1) {
      const T stuetzpunkt = (it2->first + it2->second) / 2;
      assert(stuetzpunkt >= it2->first);
      assert(stuetzpunkt <= it2->second);
      stuetzpunkte2.push_back(stuetzpunkt);
      ++it2;
    } else {
      if (it1->first < it2->first) {
        if (it2->first <= it1->second) {
          const T stuetzpunkt = (it2->first + std::min(it2->second, it1->second)) / 2;
          // Ueberlappung
          assert(stuetzpunkt >= it1->first);
          assert(stuetzpunkt <= it1->second);
          stuetzpunkte1.push_back(stuetzpunkt);
          assert(stuetzpunkt >= it2->first);
          assert(stuetzpunkt <= it2->second);
          stuetzpunkte2.push_back(stuetzpunkt);
          ++it1;
          ++it2;
        } else {
          const T stuetzpunkt = (it1->first + it1->second) / 2;
          // Keine Ueberlappung
          assert(stuetzpunkt >= it1->first);
          assert(stuetzpunkt <= it1->second);
          stuetzpunkte1.push_back(stuetzpunkt);
          ++it1;
        }
      } else {
        if (it1->first <= it2->second) {
          const T stuetzpunkt = (it1->first + std::min(it1->second, it2->second)) / 2;
          // Ueberlappung
          assert(stuetzpunkt >= it1->first);
          assert(stuetzpunkt <= it1->second);
          stuetzpunkte1.push_back(stuetzpunkt);
          assert(stuetzpunkt >= it2->first);
          assert(stuetzpunkt <= it2->second);
          stuetzpunkte2.push_back(stuetzpunkt);
          ++it1;
          ++it2;
        } else {
          const T stuetzpunkt = (it2->first + it2->second) / 2;
          // Keine Ueberlappung
          assert(stuetzpunkt >= it2->first);
          assert(stuetzpunkt <= it2->second);
          stuetzpunkte2.push_back(stuetzpunkt);
          ++it2;
        }
      }
    }
  }

  assert(stuetzpunkte1.size() == stuetzpunkt_intervalle_1.size());
  assert(stuetzpunkte2.size() == stuetzpunkt_intervalle_2.size());

  return { stuetzpunkte1, stuetzpunkte2 };
}

}  // namespace

Ziffern ZiffernBuilder::Build(const TafelParameter& tp, bool ist_negativ, int zahl_oben, int ziffer_unten) {
  assert(zahl_oben >= 0);
  assert(zahl_oben <= 999);
  assert(ziffer_unten >= 0);
  assert(ziffer_unten <= 9);

  const auto& ziffern_oben = GetZiffern(zahl_oben);
  assert(ziffern_oben.size() >= 1);
  assert(ziffern_oben.size() <= 3);

  const std::vector<int> ziffern_unten = { ziffer_unten };
  assert(ziffern_unten.size() == 1);

  // Ziffernabstaende berechnen
  const auto& abstaende_oben = GetAbstaende(tp, ziffern_oben, tp.tex_ziffern, tp.breite_mm);
  assert(abstaende_oben.size() == ziffern_oben.size() + 1);
  const auto& abstaende_unten = GetAbstaende(tp, ziffern_unten, tp.tex_ziffern, tp.breite_mm);
  assert(abstaende_unten.size() == ziffern_unten.size() + 1);

  // Stuetzpunkt-Intervalle berechnen
  const auto& stuetzpunkt_intervalle_oben = GetStuetzpunktIntervalle(tp, ziffern_oben, abstaende_oben);
  const auto& stuetzpunkt_intervalle_unten = GetStuetzpunktIntervalle(tp, ziffern_unten, abstaende_unten);

  // Stuetzpunkte berechnen
  const auto& [ stuetzpunkte_oben, stuetzpunkte_unten ] = GetStuetzpunkte(stuetzpunkt_intervalle_oben, stuetzpunkt_intervalle_unten);

  Mesh result;

  auto MakeVertex = [&tp, &result](int x, int y, float u2, float v2) -> VertexIndex {
    return result.EmplaceVertex(
        0, -x / 1000.0, y / 1000.0,
        -1, 0, 0,
        tp.tex_tafel_vorderseite.GetU(x),
        tp.tex_tafel_vorderseite.GetV(y),
        u2, v2);
  };

  auto MakeZiffer = [&tp, &result, &MakeVertex](int ziffer,
      int y_oben_mm, int y_unten_mm, int abstand_oben_mm, int abstand_unten_mm,
      int x_links_mm, int x_rechts_mm, int abstand_links_mm, int abstand_rechts_mm,
      const std::vector<int> stuetzpunkte, bool istOben) {
    assert(abstand_links_mm >= 0);
    assert(abstand_rechts_mm >= 0);

    const auto& textur = tp.tex_ziffern[ziffer];

    const float tex_per_mm_u = (textur.u_rechts - textur.u_links) / textur.breite_mm;
    const float tex_per_mm_v = (textur.v_unten - textur.v_oben) / textur.hoehe_mm;

    const float u_links = textur.u_links - abstand_links_mm * tex_per_mm_u;
    const float u_rechts = textur.u_rechts + abstand_rechts_mm * tex_per_mm_u;

    const float v_oben = textur.v_oben - abstand_oben_mm * tex_per_mm_v;
    const float v_unten = textur.v_unten + abstand_unten_mm * tex_per_mm_v;

    const auto v1 = MakeVertex(x_links_mm, y_oben_mm, u_links, v_oben);
    const auto v2 = MakeVertex(x_rechts_mm, y_oben_mm, u_rechts, v_oben);
    const auto v3 = MakeVertex(x_rechts_mm, y_unten_mm, u_rechts, v_unten);
    const auto v4 = MakeVertex(x_links_mm, y_unten_mm, u_links, v_unten);

    if (istOben) {
      MakeQuad(&result, v1, v2, v3,
          std::upper_bound(std::crbegin(stuetzpunkte), std::crend(stuetzpunkte), x_rechts_mm, std::greater<int>()),
          std::lower_bound(std::crbegin(stuetzpunkte), std::crend(stuetzpunkte), x_links_mm, std::greater<int>()),
          v4);
    } else {
      MakeQuad(&result, v1,
          std::upper_bound(std::cbegin(stuetzpunkte), std::cend(stuetzpunkte), x_links_mm),
          std::lower_bound(std::cbegin(stuetzpunkte), std::cend(stuetzpunkte), x_rechts_mm),
          v2, v3, v4);
    }
  };

  auto MakeZiffern = [&tp, &MakeZiffer](const std::vector<int>& ziffern, const std::vector<int>& x_abstaende,
      const std::vector<int>& stuetzpunkte, const std::vector<int>& stuetzpunkte2,
      int y_oben_mm, int y_unten_mm, int abstand_oben_mm, int abstand_unten_mm, bool istOben) {
    assert(x_abstaende.size() == ziffern.size() + 1);
    assert(stuetzpunkte.size() == ziffern.size() + 1);

    int offset = tp.XLinks() + x_abstaende.front();

    for (size_t i = 0; i < ziffern.size(); ++i) {
      const int ziffer_breite = tp.tex_ziffern[ziffern[i]].breite_mm;

      assert(offset >= stuetzpunkte[i]);
      assert(offset + ziffer_breite <= stuetzpunkte[i+1]);
      MakeZiffer(ziffern[i], y_oben_mm, y_unten_mm, abstand_oben_mm, abstand_unten_mm,
          stuetzpunkte[i],
          stuetzpunkte[i + 1],
          offset - stuetzpunkte[i],
          stuetzpunkte[i+1] - (offset + ziffer_breite),
          stuetzpunkte2,
          istOben);
      offset += ziffer_breite + x_abstaende[i+1];
    }
  };

  MakeZiffern(ziffern_oben, abstaende_oben, stuetzpunkte_oben, stuetzpunkte_unten,
      tp.YOben() - kEckenRadius_mm, kYTafelMitte_mm,
      (tp.YOben() - kEckenRadius_mm - tp.ziffernhoehe_mm - kYAbstandZiffern_mm) - kYTafelMitte_mm, kYAbstandZiffern_mm,
      true);

  MakeZiffern(ziffern_unten, abstaende_unten, stuetzpunkte_unten, stuetzpunkte_oben,
      kYTafelMitte_mm, kYTafelMitte_mm - tp.ziffernhoehe_mm - 2 * kYAbstandZiffern_mm,
      kYAbstandZiffern_mm, kYAbstandZiffern_mm, false);

  Mesh minuszeichen_mesh;
  if (ist_negativ) {
    const auto y = ((tp.YOben() - kEckenRadius_mm) + kYTafelMitte_mm) / 2;
    const auto y_oben = y + (tp.zifferndicke_mm / 2);
    const auto y_unten = y_oben - tp.zifferndicke_mm;

    const auto breite_regulaer = 2.2 * tp.zifferndicke_mm;
    const auto breite_min = 1.0 * tp.zifferndicke_mm;
    const auto abstand_x = 0.5 * tp.zifferndicke_mm;  // Abstand von Tafelrand und von Ziffern

    const auto x_rechts_regulaer = tp.XLinks() + abstaende_oben.front() - abstand_x;
    const auto x_links = std::max(x_rechts_regulaer - breite_regulaer, tp.XLinks() + abstand_x);
    const auto x_rechts = std::max(x_rechts_regulaer, x_links + breite_min);

    const auto v1 = minuszeichen_mesh.EmplaceVertex(-0.01, -x_rechts / 1000.0, y_unten / 1000.0, -1, 0, 0, 0.103, 0.915, 0, 0);
    const auto v2 = minuszeichen_mesh.EmplaceVertex(-0.01,  -x_links / 1000.0, y_unten / 1000.0, -1, 0, 0, 0.103, 0.915, 0, 0);
    const auto v3 = minuszeichen_mesh.EmplaceVertex(-0.01,  -x_links / 1000.0,  y_oben / 1000.0, -1, 0, 0, 0.103, 0.915, 0, 0);
    const auto v4 = minuszeichen_mesh.EmplaceVertex(-0.01, -x_rechts / 1000.0,  y_oben / 1000.0, -1, 0, 0, 0.103, 0.915, 0, 0);
    minuszeichen_mesh.faces.emplace_back(v1, v2, v3);
    minuszeichen_mesh.faces.emplace_back(v3, v4, v1);
  }

  return { result, minuszeichen_mesh, stuetzpunkte_oben, stuetzpunkte_unten };
}


Mesh MastBuilder::Build(const TafelParameter& tp) {
  Mesh result;

  constexpr float z_top = .1;
  constexpr float z_bottom = -3.4;

  constexpr float radius = 0.0375;  // Halbe Mastdicke

  const float u_links = tp.tex_mast.u_links;
  const float u_rechts = tp.tex_mast.u_rechts;

  const float v_top = tp.tex_mast.v_oben;
  const float v_bottom = (z_top - z_bottom) / (2 * radius) * (u_rechts - u_links);

  const float u_transp = tp.tex_transparent.u_links;
  const float v_transp = tp.tex_transparent.v_unten;

  /* Draufsicht (Zusi-Koordinaten):
   *
   *      x
   *      ^
   *      |
   *    l-1-r
   *    | | |
   * y<-4-+ 2
   *    |   |
   *    r-3-l
   */

  // 1
  {
    const auto v1 = result.EmplaceVertex(radius,  radius, z_bottom,  1,  0, 0, u_rechts, v_bottom, u_transp, v_transp);
    const auto v2 = result.EmplaceVertex(radius, -radius, z_bottom,  1,  0, 0, u_links,  v_bottom, u_transp, v_transp);
    const auto v3 = result.EmplaceVertex(radius, -radius, z_top,     1,  0, 0, u_links,  v_top,    u_transp, v_transp);
    const auto v4 = result.EmplaceVertex(radius,  radius, z_top,     1,  0, 0, u_rechts, v_top,    u_transp, v_transp);
    result.faces.emplace_back(v1, v2, v3);
    result.faces.emplace_back(v3, v4, v1);
  }

  // 2
  {
    const auto v1 = result.EmplaceVertex( radius, -radius, z_bottom,  0, -1, 0, u_links,  v_bottom, u_transp, v_transp);
    const auto v2 = result.EmplaceVertex(-radius, -radius, z_bottom,  0, -1, 0, u_rechts, v_bottom, u_transp, v_transp);
    const auto v3 = result.EmplaceVertex(-radius, -radius, z_top,     0, -1, 0, u_rechts, v_top,    u_transp, v_transp);
    const auto v4 = result.EmplaceVertex( radius, -radius, z_top,     0, -1, 0, u_links,  v_top,    u_transp, v_transp);
    result.faces.emplace_back(v1, v2, v3);
    result.faces.emplace_back(v3, v4, v1);
  }

  // 3
  {
    const auto v1 = result.EmplaceVertex(-radius, -radius, z_bottom, -1,  0, 0, u_rechts, v_bottom, u_transp, v_transp);
    const auto v2 = result.EmplaceVertex(-radius,  radius, z_bottom, -1,  0, 0, u_links,  v_bottom, u_transp, v_transp);
    const auto v3 = result.EmplaceVertex(-radius,  radius, z_top,    -1,  0, 0, u_links,  v_top,    u_transp, v_transp);
    const auto v4 = result.EmplaceVertex(-radius, -radius, z_top,    -1,  0, 0, u_rechts, v_top,    u_transp, v_transp);
    result.faces.emplace_back(v1, v2, v3);
    result.faces.emplace_back(v3, v4, v1);
  }

  // 4
  {
    const auto v1 = result.EmplaceVertex(-radius,  radius, z_bottom,  0,  1, 0, u_links,  v_bottom, u_transp, v_transp);
    const auto v2 = result.EmplaceVertex( radius,  radius, z_bottom,  0,  1, 0, u_rechts, v_bottom, u_transp, v_transp);
    const auto v3 = result.EmplaceVertex( radius,  radius, z_top,     0,  1, 0, u_rechts, v_top,    u_transp, v_transp);
    const auto v4 = result.EmplaceVertex(-radius,  radius, z_top,     0,  1, 0, u_links,  v_top,    u_transp, v_transp);
    result.faces.emplace_back(v1, v2, v3);
    result.faces.emplace_back(v3, v4, v1);
  }

  // Deckel
  {
    const float v_oben = 0;
    const float v_unten = u_rechts - u_links;

    const auto v1 = result.EmplaceVertex( radius, -radius, z_top,     0,  0, 1, u_rechts, v_unten,  u_transp, v_transp);
    const auto v2 = result.EmplaceVertex(-radius, -radius, z_top,     0,  0, 1, u_rechts, v_oben,   u_transp, v_transp);
    const auto v3 = result.EmplaceVertex(-radius,  radius, z_top,     0,  0, 1, u_links,  v_oben,   u_transp, v_transp);
    const auto v4 = result.EmplaceVertex( radius,  radius, z_top,     0,  0, 1, u_links,  v_unten,  u_transp, v_transp);
    result.faces.emplace_back(v1, v2, v3);
    result.faces.emplace_back(v3, v4, v1);
  }

  return result;
}


namespace {

/**
 * Hilfsmethode zum Erstellen einer Zifferntextur.
 * Alle Pixelkoordinaten sind bezogen auf ein 256x256-Bild.
 * Koordinatensystem wie in Gimp:
 * +->x
 * |
 * v
 * y
 */
static constexpr Textur MakeTexturGimp(
    int breite_mm, float x_links_px, float breite_px,
    int hoehe_mm, float y_unten_px, float hoehe_px) {
  return { breite_mm, x_links_px / 256.0f, (x_links_px + breite_px) / 256.0f,
    hoehe_mm, (y_unten_px) / 256.0f, (y_unten_px + hoehe_px) / 256.0f };
}

static const Textur kTransparentTextur = MakeTexturGimp(0, 4, 0, 0, 252, 0);

static const Textur kTafelVorderseiteTexturGross           = MakeTexturGimp(720 / 2,  59,  57, 800 / 2, 65, 64);
static const Textur kTafelVorderseiteTexturGrossGespiegelt = MakeTexturGimp(720 / 2,  58, -57, 800 / 2, 65, 64);
static const Textur kTafelRueckseiteTexturGross            = MakeTexturGimp(720 / 2, 174,  55, 800 / 2, 65, 64);
static const Textur kTafelRueckseiteTexturGrossGespiegelt  = MakeTexturGimp(720 / 2, 174, -55, 800 / 2, 65, 64);

static const Textur kTafelVorderseiteTexturKlein           = MakeTexturGimp(480 / 2,  59,  57, 610 / 2, 65, 64);
static const Textur kTafelVorderseiteTexturKleinGespiegelt = MakeTexturGimp(480 / 2,  58, -57, 610 / 2, 65, 64);
static const Textur kTafelRueckseiteTexturKlein            = MakeTexturGimp(480 / 2, 174,  52, 610 / 2, 65, 64);
static const Textur kTafelRueckseiteTexturKleinGespiegelt  = MakeTexturGimp(480 / 2, 174, -52, 610 / 2, 65, 64);

static const Textur kMastTextur = MakeTexturGimp(22, 233, 22, 256, 0, 256);

/**
 * Hilfsmethode zum Erstellen einer Zifferntextur.
 * Alle Pixelkoordinaten sind bezogen auf ein 256x256-Bild.
 * Koordinatensystem wie in Inkscape:
 * y
 * ^
 * |
 * +->x
 */
static constexpr Textur MakeTexturInkscape(
    int breite_mm, float x_links_px, float breite_px,
    int hoehe_mm, float y_unten_px, float hoehe_px) {
  return { breite_mm, x_links_px / 256.0f, (x_links_px + breite_px) / 256.0f,
    hoehe_mm, (256 - y_unten_px - hoehe_px) / 256.0f, (256 - y_unten_px) / 256.0f };
}

// breite_mm: Reine Ziffernbreite in mm
// abstand_x_mm: Beschnittzugabe links und rechts in mm
static constexpr Textur MakeZiffernTextur(
    int breite_mm, float x_links_px, float breite_px, float abstand_x_mm,
    int hoehe_mm, float y_unten_px, float hoehe_px) {
  return MakeTexturInkscape(
      breite_mm + 2 * abstand_x_mm,
      x_links_px - abstand_x_mm / breite_mm * breite_px,
      breite_px + 2 * abstand_x_mm / breite_mm * breite_px,
      hoehe_mm, y_unten_px, hoehe_px);
}

// Berechne Beschnittzugabe links und rechts, sodass die Tafel mit den breitesten Ziffern
// ohne Fehler erzeugt werden kann.
// Grosse Ausfuehrung:
//  - zweistellig (Tafelbreite 480): Zahl 40 -> 221+231 -> 28mm uebrig -> 28/4 mm = 1.15px Abstand
//  - dreistellig (Tafelbreite 720): Zahl 400 -> 221+231+231 -> 37mm uebrig -> 37/6 mm = 0.98px Abstand
// Kleine Ausfuehrung:
//  - zweistellig (Tafelbreite 320): Zahl 40 -> 150+157 -> 13mm uebrig -> 13/4 mm = 0.72px Abstand
//  - dreistellig (Tafelbreite 480): Zahl 400 -> 150+157+157 -> 16mm uebrig -> 16/6 mm = 0.48px Abstand
static constexpr float kAbstandXGross_mm = 37/6.0f;
static constexpr float kAbstandXKlein_mm = 16/6.0f;

static constexpr float AbstandX(bool gross) {
  return gross ? kAbstandXGross_mm : kAbstandXKlein_mm;
}

static constexpr std::array<Textur, 10> MakeZiffernTexturen(bool gross) {
  return {{
    MakeZiffernTextur(gross ? 231 : 157,  19.000,           38.095, AbstandX(gross), gross ? 310 : 210,      68,  51),
    MakeZiffernTextur(gross ?  89 :  60,  15.000,           14.695, AbstandX(gross), gross ? 310 : 210,       7,  51),
    MakeZiffernTextur(gross ? 190 : 129,  45.689,           31.383, AbstandX(gross), gross ? 310 : 210,       7,  51),
    MakeZiffernTextur(gross ? 173 : 117, 128.139,           28.371, AbstandX(gross), gross ? 310 : 210,      68,  51),
    MakeZiffernTextur(gross ? 221 : 150, 175.570,           36.430, AbstandX(gross), gross ? 310 : 210,      68,  51),

    MakeZiffernTextur(gross ? 200 : 135,  76.168,           32.898, AbstandX(gross), gross ? 310 : 210,      68,  51),
    MakeZiffernTextur(gross ? 201 : 136,  93.066,           33.030, AbstandX(gross), gross ? 310 : 210,       7,  51),
    MakeZiffernTextur(gross ? 169 : 114, 142.090,           27.766, AbstandX(gross), gross ? 310 : 210,       7,  51),
    MakeZiffernTextur(gross ? 183 : 124, 185.850,           30.150, AbstandX(gross), gross ? 310 : 210,       7,  51),
    // Ziffer 9 ist eine gedrehte Ziffer 6
    MakeZiffernTextur(gross ? 201 : 136,  93.066 + 33.030, -33.030, AbstandX(gross), gross ? 310 : 210,  7 + 51, -51),
  }};
}

static constexpr std::array<Textur, 10> kZiffernTexturenGross = MakeZiffernTexturen(true);
static constexpr std::array<Textur, 10> kZiffernTexturenKlein = MakeZiffernTexturen(false);

// Z-Verschiebung fuer die hohe Variante der Tafel
static constexpr float kZVerschiebungHoch = 2.4f;

}  // namespace

void HektoBuilder::Build(FILE* fd, const BauParameter& bauparameter, int hektometer) {
  const bool ist_negativ = hektometer < 0;
  const int zahl_oben = std::abs(hektometer) / 10;
  const int ziffer_unten = std::abs(hektometer) % 10;

  assert(zahl_oben >= 0);
  assert(zahl_oben <= 999);
  assert(ziffer_unten >= 0);
  assert(ziffer_unten <= 9);

  fprintf(fd,
      "\xef\xbb\xbf"
      "<?xml version=\"1.0\" encoding=\"utf-8\"?>\n"
      "<Zusi>\n"
      "<Info DateiTyp=\"Landschaft\" Version=\"A.1\" MinVersion=\"A.1\">\n"
      "<AutorEintrag AutorID=\"-1\" AutorName=\"Zusi-generiert\"/>\n"
      "</Info>\n"
      "<Landschaft>\n");

  const uint32_t grundfarbe = 0xFFFFFF;
  const uint32_t nachtfarbe = 0x646464;

  SubsetBuilder subset_unbeleuchtet(grundfarbe | 0xFF000000, 0xFF000000);
  SubsetBuilder subset_beleuchtet((grundfarbe - nachtfarbe) | 0xFF000000, nachtfarbe | 0xFF000000);
  auto& subset_evtl_beleuchtet = (bauparameter.rueckstrahlend == Rueckstrahlend::kYes ? subset_beleuchtet : subset_unbeleuchtet);

  // Die Spiegelung der Vorder- und Rueckseitentextur ist abhaengig vom dargestellten Wert
  std::srand(1000 * zahl_oben + 100 * ziffer_unten);

  const float mm_per_px = bauparameter.groesse == Groesse::kKlein ? 210.0f / 51.0f : 310.0f / 51.0f;

  const bool breit = (ist_negativ && (zahl_oben >= 10)) || (zahl_oben >= 100);  // TODO auch bei Ueberlaenge

  TafelParameter tp = bauparameter.groesse == Groesse::kKlein ?
    TafelParameter {
      /* breite */ breit ? 480 : 320,
      /* hoehe */ 610,

      /* ziffernhoehe_mm */ 210,
      /* def_ziffernabstand_mm */ static_cast<int>(16 - 2 * kAbstandXKlein_mm),
      /* max_ziffernabstand_mm */ static_cast<int>(15/*px*/ * mm_per_px - 2 * kAbstandXKlein_mm),
      /* zifferndicke_mm */ 27,

      /* tex_tafel_vorderseite */ rand() % 2 == 0 ? kTafelVorderseiteTexturKlein : kTafelVorderseiteTexturKleinGespiegelt,
      /* tex_tafel_rueckseite */ rand() % 2 == 0 ? kTafelRueckseiteTexturKlein : kTafelRueckseiteTexturKleinGespiegelt,
      /* tex_ziffern */ kZiffernTexturenKlein,
      /* tex_mast */ kMastTextur,
      /* tex_transparent */ kTransparentTextur
    } : TafelParameter {
      /* breite */ breit ? 720 : 480,
      /* hoehe */ 800,

      /* ziffernhoehe_mm */ 310,
      /* def_ziffernabstand_mm */ static_cast<int>(24 - 2 * kAbstandXGross_mm),
      /* max_ziffernabstand_mm */ static_cast<int>(15/*px*/ * mm_per_px - 2 * kAbstandXGross_mm),
      /* zifferndicke_mm */ 40,

      /* tex_tafel_vorderseite */ rand() % 2 == 0 ? kTafelVorderseiteTexturGross : kTafelVorderseiteTexturGrossGespiegelt,
      /* tex_tafel_rueckseite */ rand() % 2 == 0 ? kTafelRueckseiteTexturGross : kTafelRueckseiteTexturGrossGespiegelt,
      /* tex_ziffern */ kZiffernTexturenGross,
      /* tex_mast */ kMastTextur,
      /* tex_transparent */ kTransparentTextur
    };

  const char* nbue_dateiname = bauparameter.groesse == Groesse::kKlein ?
    "_Setup\\lib\\milepost\\hektometertafeln_DB\\NBUe_Signal_klein.ls3" :
    "_Setup\\lib\\milepost\\hektometertafeln_DB\\NBUe_Signal.ls3";

  auto MakeAnkerpunkt = [&](float x, float z, bool rueckseite) {
    fprintf(fd, "<Ankerpunkt>\n");
    if (x != 0 || z != 0) {
      fprintf(fd, "<p");
      if (x != 0) {
        fprintf(fd, " X=\"%f\"", x);
      }
      if (z != 0) {
        fprintf(fd, " Z=\"%f\"", z);
      }
      fprintf(fd, "/>\n");
    }
    if (rueckseite) {
      fprintf(fd, "<phi Z=\"3.141592\"/>");
    }
    fprintf(fd, "<Datei Dateiname=\"%s\"/>\n</Ankerpunkt>\n", nbue_dateiname);
  };

  const float x_verschiebung = bauparameter.mast == Mast::kMitMast ? .038f : 0.0f;
  const float z_verschiebung = bauparameter.hoehe == Hoehe::kHoch ? kZVerschiebungHoch : 0.0f;
  // Die generierte Tafel ist in Y- und Z-Richtung zentriert
  // Verschiebe sie so, dass die Oberkante bei z=0 liegt
  const float z_verschiebung_tafel = z_verschiebung + (bauparameter.groesse == Groesse::kKlein ? -.61 / 2 : -.80 / 2);

  const auto& ziffern = ZiffernBuilder::Build(tp, ist_negativ, zahl_oben, ziffer_unten);
  const auto& mesh_vorderseite = TafelVorderseiteBuilder::Build(tp, ziffern.stuetzpunkte_oben, ziffern.stuetzpunkte_unten);
  const auto& mesh_rueckseite = TafelRueckseiteBuilder::Build(tp);

  subset_evtl_beleuchtet.AddMesh(MeshOps::translate(-x_verschiebung, 0, z_verschiebung_tafel, ziffern.mesh1));
  subset_evtl_beleuchtet.AddMesh(MeshOps::translate(-x_verschiebung, 0, z_verschiebung_tafel, ziffern.mesh2)); // TODO: sep. Subset
  subset_evtl_beleuchtet.AddMesh(MeshOps::translate(-x_verschiebung, 0, z_verschiebung_tafel, mesh_vorderseite));

  if (bauparameter.beidseitig == Beidseitig::kBeidseitig) {
    subset_evtl_beleuchtet.AddMesh(MeshOps::translate(x_verschiebung, 0, z_verschiebung_tafel, MeshOps::rotateZ180(ziffern.mesh1)));
    subset_evtl_beleuchtet.AddMesh(MeshOps::translate(x_verschiebung, 0, z_verschiebung_tafel, MeshOps::rotateZ180(ziffern.mesh2))); // TODO: sep. Subset
    subset_evtl_beleuchtet.AddMesh(MeshOps::translate(x_verschiebung, 0, z_verschiebung_tafel, MeshOps::rotateZ180(mesh_vorderseite)));
  }

  if (bauparameter.mast == Mast::kMitMast) {
    subset_unbeleuchtet.AddMesh(MeshOps::translate(-x_verschiebung, 0, z_verschiebung_tafel, MeshOps::rotateZ180(mesh_rueckseite)));
    if (bauparameter.beidseitig == Beidseitig::kBeidseitig) {
      subset_unbeleuchtet.AddMesh(MeshOps::translate(x_verschiebung, 0, z_verschiebung_tafel, mesh_rueckseite));
    }
    subset_unbeleuchtet.AddMesh(MeshOps::translate(0, 0, z_verschiebung, MastBuilder::Build(tp)));
  } else if (bauparameter.beidseitig == Beidseitig::kEinseitig) {
    subset_unbeleuchtet.AddMesh(MeshOps::translate(x_verschiebung, 0, z_verschiebung_tafel, MeshOps::rotateZ180(mesh_rueckseite)));
  }

  if (bauparameter.ankerpunkt == Ankerpunkt::kYes) {
    if (bauparameter.beidseitig == Beidseitig::kBeidseitig) {
      MakeAnkerpunkt(-x_verschiebung - 0.01, z_verschiebung, false);
      MakeAnkerpunkt(x_verschiebung + 0.01, z_verschiebung, true);
    } else {
      MakeAnkerpunkt(-x_verschiebung, z_verschiebung, false);
    }
  }

  subset_beleuchtet.Write(fd);
  subset_unbeleuchtet.Write(fd);

  fprintf(fd,
      "</Landschaft>\n"
      "</Zusi>\n");
}
