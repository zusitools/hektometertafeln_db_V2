// Copyright 2021 Zusitools

#include "dll.hpp"

#include <cassert>
#include <iostream>

int
main([[maybe_unused]] int argc, char** argv)
{
  assert(argc > 1);

  std::cout << Init(argv[1]) << '\n';
  std::cout << dllVersion() << '\n';
  std::cout << Autor() << '\n';
  std::cout << Bezeichnung() << '\n';
  std::cout << AbstandTafeln() << '\n';

  auto erzeugen = [](int meter) {
    const char* datei;
    int result = Erzeugen(meter, 0, &datei);
    std::cout << "Ergebnis: " << result << '\n';
    if (datei != nullptr) {
      std::cout << "Erzeugte Datei: \"" << datei << "\"\n";
    }
  };
  erzeugen(138500);
  erzeugen(40700);
  erzeugen(2900);
}
