// Copyright 2018 Zusitools

#ifndef CONFIG_HPP_
#define CONFIG_HPP_

#include "hekto_builder.hpp"

struct HektoDllConfig final {
  Beidseitig beidseitig;
  Groesse groesse;
  Rueckstrahlend rueckstrahlend;
  Ankerpunkt ankerpunkt;
  bool immer_ohne_mast;
};

#endif  // CONFIG_HPP_
