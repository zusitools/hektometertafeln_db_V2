// Copyright 2018 Zusitools

#ifndef TEXTUR_HPP_
#define TEXTUR_HPP_

/* Koordinatensystem:
 * 0--1 u
 * |
 * 1
 * v
 */

struct Textur final {
  int breite_mm;  ///< Breite des durch die Textur abgedeckten Bereichs in mm
  float u_links;
  float u_rechts;
  int hoehe_mm;  ///< Hoehe des durch die Textur abgedeckten Bereichs in mm
  float v_oben;
  float v_unten;

  float GetU(int x_mm) const;
  float GetV(int y_mm) const;
};

#endif  // TEXTUR_HPP_
