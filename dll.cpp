// Copyright 2018 Zusitools

#include "dll.hpp"

#include "config.hpp"
#include "gui.hpp"
#include "hekto_builder.hpp"

#include <shlwapi.h>

#include <cassert>
#include <cstdio>
#include <string>
#include <utility>

// Globale Variablen
DWORD g_zusi_datenpfad_laenge;
char g_zielverzeichnis[MAX_PATH];  // wird von Zusi beim Initialisieren gesetzt; ohne abschliessenden Slash/Backslash
char g_outDatei[MAX_PATH];  // zwecks Rueckgabe an Zusi

HektoDllConfig g_config = {
  Beidseitig::kEinseitig,
  Groesse::kGross,
  Rueckstrahlend::kNo,
  Ankerpunkt::kNo,
  /* immer_ohne_mast */ false,
  /* hat_ueberlaenge */ false,
  /* basis_km */ 0,
  /* basis_hm */ 0,
};

enum class Standort : std::uint8_t {
  kEigenerStandort = 0,
  kMontageAmAnkerpunkt = 1,
};

DLL_EXPORT uint32_t Init(const char* zielverzeichnis) {
  HKEY key;
  g_zusi_datenpfad_laenge = MAX_PATH;
  if (!SUCCEEDED(RegOpenKeyEx(HKEY_LOCAL_MACHINE, "Software\\Zusi3", 0, KEY_READ | KEY_WOW64_32KEY, &key)) ||
      !SUCCEEDED(RegGetValue(key, nullptr, "DatenVerzeichnis", RRF_RT_REG_SZ, nullptr, (LPBYTE)g_zielverzeichnis, &g_zusi_datenpfad_laenge))) {
    return 0;
  }

  g_zusi_datenpfad_laenge -= 1;  // pcbData ist inklusive Nullterminator

  if (!PathAppend(g_zielverzeichnis, zielverzeichnis)) {
    return 0;
  }

  if (!PathAppend(g_zielverzeichnis, "Hektometertafeln")) {
    return 0;
  }

  // Der an Zusi zurueckgegebene Pfad soll keinen fuehrenden Backslash enthalten
  if (g_zielverzeichnis[g_zusi_datenpfad_laenge - 1] != '\\') {
    g_zusi_datenpfad_laenge += 1;
  }

  return 1;
}

DLL_EXPORT const char* dllVersion() {
  return "0.0.8";
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

const char* GetDateiname(const BauParameter& bau_parameter, Kilometrierung kilometrierung, int ueberlaenge_hm) {
  CreateDirectory(g_zielverzeichnis, nullptr);
  snprintf(g_outDatei, sizeof(g_outDatei)/sizeof(g_outDatei[0]),
      "%s\\Hekto%s%s%s_%d_%d%s.ls3", g_zielverzeichnis,
      (bau_parameter.mast == Mast::kMitMast ? "_Mast" : ""),
      (bau_parameter.beidseitig == Beidseitig::kBeidseitig ? "_beids" : ""),
      (bau_parameter.groesse == Groesse::kKlein ? "_klein" : ""),
      kilometrierung.km,
      std::abs(kilometrierung.hm),
      ueberlaenge_hm == 0 ? "" : (std::string("_") + std::to_string(ueberlaenge_hm)).c_str());

  return g_outDatei;
}

DLL_EXPORT uint8_t Erzeugen(float wert_m, uint8_t modus, const char** datei) {
  *datei = nullptr;

  Kilometrierung km_basis =
    g_config.hat_ueberlaenge ? Kilometrierung { g_config.basis_km, g_config.basis_hm } : Kilometrierung::fromMeter(wert_m);
  Kilometrierung km_tatsaechlich =
    g_config.hat_ueberlaenge ? Kilometrierung::fromMeter(wert_m) : km_basis;
  const auto ueberlaenge_hm = km_tatsaechlich.toHektometer() - km_basis.toHektometer();

  if ((ueberlaenge_hm < 0) || (ueberlaenge_hm > 99)) {
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

  *datei = GetDateiname(bauparameter, km_basis, ueberlaenge_hm) + g_zusi_datenpfad_laenge;

  FILE* fd = fopen(g_outDatei, "w");
  assert(fd != nullptr);
  HektoBuilder::Build(fd, bauparameter, km_basis, ueberlaenge_hm);
  fclose(fd);

  return 1;
}
