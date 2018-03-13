// Copyright 2018 Zusitools

#include "textur.hpp"

float Textur::GetU(int x_mm) const {
  const float faktor_u = x_mm / static_cast<float>(breite_mm);
  return u_links + faktor_u * (u_rechts - u_links);
}

float Textur::GetV(int y_mm) const {
  const float faktor_v = -y_mm / static_cast<float>(hoehe_mm);
  return v_oben + faktor_v * (v_unten - v_oben);
}
