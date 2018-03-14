// Copyright 2018 Zusitools

#include "dll.hpp"

#include "config.hpp"
#include "gui.hpp"
#include "hekto_builder.hpp"

#include <shlwapi.h>

#include <cassert>
#include <cstdio>
#include <utility>

// Globale Variablen
char g_zielverzeichnis[MAX_PATH];  // wird von Zusi beim Initialisieren gesetzt; ohne abschliessenden Slash/Backslash
char g_outDatei[MAX_PATH];  // zwecks Rueckgabe an Zusi

HektoDllConfig g_config = {
  Beidseitig::kEinseitig,
  Groesse::kGross,
  Rueckstrahlend::kNo,
  Ankerpunkt::kNo,
  /* immer_ohne_mast */ false
};

enum class Standort : std::uint8_t {
  kEigenerStandort = 0,
  kMontageAmAnkerpunkt = 1,
};

DLL_EXPORT uint32_t Init(const char* zielverzeichnis) {
  HKEY key;
  DWORD len = MAX_PATH;
  if (!SUCCEEDED(RegOpenKeyEx(HKEY_LOCAL_MACHINE, "Software\\Zusi3", 0, KEY_READ | KEY_WOW64_32KEY, &key)) ||
      !SUCCEEDED(RegGetValue(key, nullptr, "DatenVerzeichnis", RRF_RT_REG_SZ, nullptr, (LPBYTE)g_zielverzeichnis, &len))) {
    return 0;
  }

  if (!PathAppend(g_zielverzeichnis, zielverzeichnis)) {
    return 0;
  }

  if (!PathAppend(g_zielverzeichnis, "Hektometertafeln")) {
    return 0;
  }

  return 1;
}

DLL_EXPORT const char* dllVersion() {
  return "0.0.4";
}

DLL_EXPORT const char* Autor() {
  return "Zusitools";
}

DLL_EXPORT const char* Bezeichnung() {
  return "Hektometertafeln (DB) V2";
}

DLL_EXPORT float AbstandTafeln() {
  return 200.0;
}

DLL_EXPORT float AbstandGleis(uint8_t modus) {
  if (static_cast<Standort>(modus) == Standort::kEigenerStandort) {
    return 3.0;
  } else {
    return 4.0;
  }
}

DLL_EXPORT const char* Gruppe() {
  return "Hektometertafeln";
}

DLL_EXPORT void Config(HWND appHandle) {
  ShowGui(appHandle, &g_config);
}

const char* GetDateiname(const BauParameter& bau_parameter, int zahl_oben, int ziffer_unten) {
  CreateDirectory(g_zielverzeichnis, nullptr);
  snprintf(g_outDatei, sizeof(g_outDatei)/sizeof(g_outDatei[0]),
      "%s\\Hekto%s%s%s_%d_%d.ls3", g_zielverzeichnis,
      (bau_parameter.mast == Mast::kMitMast ? "_Mast" : ""),
      (bau_parameter.beidseitig == Beidseitig::kBeidseitig ? "_beids" : ""),
      (bau_parameter.groesse == Groesse::kKlein ? "_klein" : ""),
      zahl_oben,
      ziffer_unten);

  return g_outDatei;
}

std::pair<int, int> BerechneZiffern(float wert_m) {
  assert(wert_m >= 0);
  // Runde auf Hektometer
  // TODO(me): negative Kilometerangaben
  int wert_m_int = wert_m + 50;
  return std::make_pair(wert_m_int / 1000, (wert_m_int % 1000) / 100);
}

DLL_EXPORT uint8_t Erzeugen(float wert_m, uint8_t modus, const char** datei) {
  const auto& [ zahl_oben, ziffer_unten ] = BerechneZiffern(wert_m);

  if (zahl_oben < 0 || zahl_oben > 999 || ziffer_unten < 0 || ziffer_unten > 9) {
    return 0;
  }

  auto standort = static_cast<Standort>(modus);
  BauParameter bauparameter = {
    standort == Standort::kEigenerStandort ? Hoehe::kHoch : Hoehe::kNiedrig,
    (standort == Standort::kMontageAmAnkerpunkt || g_config.immer_ohne_mast) ? Mast::kOhneMast : Mast::kMitMast,
    g_config.beidseitig,
    g_config.groesse,
    g_config.rueckstrahlend,
    g_config.ankerpunkt
  };

  *datei = GetDateiname(bauparameter, zahl_oben, ziffer_unten);

  FILE* fd = fopen(g_outDatei, "w");
  assert(fd != nullptr);
  HektoBuilder::Build(fd, bauparameter, zahl_oben, ziffer_unten);
  fclose(fd);

  return 1;
}
