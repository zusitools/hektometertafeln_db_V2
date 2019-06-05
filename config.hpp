// Copyright 2018 Zusitools

#ifndef CONFIG_HPP_
#define CONFIG_HPP_

#include "hekto_builder.hpp"

struct HektoDllConfig final {
  Beidseitig beidseitig;
  Groesse groesse;
  Rueckstrahlend rueckstrahlend;
  Ankerpunkt ankerpunkt;
  TexturDatei textur;
  bool immer_ohne_mast;
  bool hat_ueberlaenge;
  int basis_km;
  int basis_hm;
};

#endif  // CONFIG_HPP_
